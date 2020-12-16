// Fill out your copyright notice in the Description page of Project Settings.

#include "StellarSamplingData.h"

#include "GalaxyNiagaraFunctionLibrary.h"
#include "NiagaraParameterCollection.h"
#include "ProbabilityCurveFunctionLibrary.h"

namespace
{
	bool NormalizeFractions(TArray<FStellarClass>& StellarClasses)
	{
		float Total = 0.0f;
		for (const FStellarClass& StellarClass : StellarClasses)
		{
			ensureMsgf(StellarClass.Fraction > 0.0f, TEXT("Stellar class fraction is negative or zero"));
			Total += StellarClass.Fraction;
		}
		if (!ensureMsgf(!FMath::IsNearlyZero(Total), TEXT("Stellar class fractions total is too small")))
		{
			return false;
		}

		for (FStellarClass& StellarClass : StellarClasses)
		{
			StellarClass.Fraction /= Total;
		}

		return true;
	}

// Utility macros to perform additional update hacks made necessary due to Niagara bugs
#define NIAGARA_UPDATE_HACK_BEGIN(_Collection, _UpdatedParameters)                                   \
	{                                                                                                \
		/** Restart any systems using this collection. */                                            \
		FNiagaraSystemUpdateContext _UpdateContext(_Collection, true);                               \
		/** XXX BUG in UE 4.26: Parameter overrides do not work, have to modify the default instance \
		/*  https://issues.unrealengine.com/issue/UE-97301                                           \
		 */                                                                                          \
		UNiagaraParameterCollectionInstance* _UpdatedParameters = _Collection->GetDefaultInstance();

#if WITH_EDITOR
#define NIAGARA_UPDATE_HACK_END(_Collection)                             \
	/** Push the change to anyone already bound. */                      \
	_Collection->GetDefaultInstance()->GetParameterStore().Tick();       \
	/** XXX this is necessary to update editor windows using old data */ \
	_Collection->OnChangedDelegate.Broadcast();                          \
	}
#else
#define NIAGARA_UPDATE_HACK_END()                   \
	/** Push the change to anyone already bound. */ \
	_Collection->GetDefaultInstance()->GetParameterStore().Tick();  \
	}
#endif
} // namespace

void UStarSettings::UpdateNiagaraParameters()
{
	if (!ensureMsgf(NiagaraParameters != nullptr, TEXT("Niagara parameter collection not set")))
	{
		return;
	}
	if (!ensureMsgf(StellarClassesTable != nullptr, TEXT("Stellar classes table is not set")))
	{
		return;
	}
	if (!ensureMsgf(StellarClassesTable->GetRowMap().Num() > 0, TEXT("Stellar classes table is empty")))
	{
		return;
	}

	// Compute average star luminosity (expectation value): E(L) = sum(Ls_k * P_k)
	// Fractional target luminosity a single particle shall represent Lf = E(L) * N / M, N: stars in galaxy, M: particle count

	// ### Probability function for particle luminosity:
	// Sort stars into bins by luminosity
	// Number of stars in each bin: N_i = P_i * N, P_i: relative frequency of stars of this class
	// Total luminosity of each bin: L_i = Ls_i * N_i = Ls_i * P_i * N
	// Number of particles in the bin:
	//   M_i = / Ls_i < Lf: L_i / Lf    = P_i * N * Ls_i / Lf
	//         \ Ls_i > Lf: L_i / Ls_i  = P_i * N
	// Probability of picking star luminosity Ls for particle:
	//   P(Ls_i <= Ls < Ls_(i+1)) = M_i / M
	//                            = / Ls_i < Lf: P_i * Ls_i / E(L)  [lower probability]
	//                              \ Ls_i > Lf: P_i * Lf / E(L)    [higher probability]
	// Integrated of picking star luminosity Ls for particle:
	//   P(Ls_i <= Ls) = sum(P_k * Ls_k, k=1..i)

	// ### Define particle luminosity:
	// Select star luminosity represented by particle Ls = sample(T_L)
	// Compute fractional star count Nf = Lf / Ls
	// Compute clamped star count for particle, for high Ls we want to represent at least 1 star: Np = max(Nf, 1)
	// Compute final particle luminosity Lp = Np * Ls
	// Assign particle luminosity value Lp, star count Np

	// Local mutable copy of stellar classes
	TArray<FStellarClass> StellarClasses;
	StellarClasses.Reserve(StellarClassesTable->GetRowMap().Num());
	StellarClassesTable->ForeachRow<FStellarClass>(
		"CreateLuminositySampling", [&StellarClasses](const FName& Key, const FStellarClass& Value) { StellarClasses.Add(Value); });

	// Sort by luminosity in increasing order
	StellarClasses.Sort(
		[](const FStellarClass& ClassA, const FStellarClass& ClassB) { return ClassA.MinLuminosity < ClassB.MinLuminosity; });

	// Normalize to ensure fractions add up to 1
	if (!NormalizeFractions(StellarClasses))
	{
		return;
	}

	// XXX Arbitrary: double min. luminosity of the last class as max. luminosity
	const float MaxLuminosity = 2.0f * StellarClasses.Last().MinLuminosity;

	// Average luminosity of i-th class
	auto AverageClassLuminosity = [StellarClasses, MaxLuminosity](int32 i) -> float {
		const float MinClassLuminosity = StellarClasses[i].MinLuminosity;
		const float MaxClassLuminosity = i < StellarClasses.Num() - 1 ? StellarClasses[i + 1].MinLuminosity : MaxLuminosity;
		return 0.5f * (MinClassLuminosity + MaxClassLuminosity) * StellarClasses[i].Fraction;
	};

	float AverageLuminosity = 0.0f;
	for (int32 i = 0; i < StellarClasses.Num(); ++i)
	{
		AverageLuminosity += AverageClassLuminosity(i);
	}

	// Stores logarithmic Luminosity ln(L) for more sensible values.
	// Luminosity varies by many orders of magnitude.
	FRichCurve LogLuminositySamplingCurve;
	// Number of stars represented by one particle.
	FRichCurve LogStarCountSamplingCurve;
	float TotalProbability = 0.0f;
	for (int32 i = 0; i < StellarClasses.Num(); ++i)
	{
		const FStellarClass& StellarClass = StellarClasses[i];
		const float Luminosity = StellarClass.MinLuminosity;

		// Probability is tweaked such that stars with luminosity > Lf
		// can be represented by a single particle without changing overall Luminosity too much.
		const float Probability =
			Luminosity < AverageLuminosity ? StellarClass.Fraction * Luminosity / AverageLuminosity : StellarClass.Fraction;
		const float StarCount = Luminosity < AverageLuminosity ? AverageLuminosity / Luminosity : 1.0f;

		FKeyHandle Handle;
		Handle = LogLuminositySamplingCurve.AddKey(TotalProbability, FMath::Loge(StellarClass.MinLuminosity));
		LogLuminositySamplingCurve.SetKeyInterpMode(Handle, RCIM_Linear);
		Handle = LogStarCountSamplingCurve.AddKey(TotalProbability, FMath::Loge(StarCount));
		LogStarCountSamplingCurve.SetKeyInterpMode(Handle, RCIM_Linear);

		TotalProbability += Probability;
	}
	// Final point
	{
		const float StarCount = MaxLuminosity < AverageLuminosity ? AverageLuminosity / MaxLuminosity : 1.0f;

		FKeyHandle Handle;
		Handle = LogLuminositySamplingCurve.AddKey(TotalProbability, FMath::Loge(MaxLuminosity));
		LogLuminositySamplingCurve.SetKeyInterpMode(Handle, RCIM_Linear);
		Handle = LogStarCountSamplingCurve.AddKey(TotalProbability, FMath::Loge(StarCount));
		LogStarCountSamplingCurve.SetKeyInterpMode(Handle, RCIM_Linear);
	}
	// Normalize probability
	if (!FMath::IsNearlyZero(TotalProbability))
	{
		for (FRichCurveKey& Key : LogLuminositySamplingCurve.Keys)
		{
			Key.Time /= TotalProbability;
			Key.ArriveTangent *= TotalProbability;
			Key.LeaveTangent *= TotalProbability;
		}
		for (FRichCurveKey& Key : LogStarCountSamplingCurve.Keys)
		{
			Key.Time /= TotalProbability;
			Key.ArriveTangent *= TotalProbability;
			Key.LeaveTangent *= TotalProbability;
		}
	}

	NIAGARA_UPDATE_HACK_BEGIN(NiagaraParameters->Collection, UpdatedParameters)
	const bool bOverride = false;
	UGalaxyNiagaraFunctionLibrary::SetCurveParameter(UpdatedParameters, TEXT("LuminositySamplingCurve"), LogLuminositySamplingCurve, bOverride);
	UGalaxyNiagaraFunctionLibrary::SetCurveParameter(UpdatedParameters, TEXT("StarCountSamplingCurve"), LogStarCountSamplingCurve, bOverride);
	UGalaxyNiagaraFunctionLibrary::SetFloatParameter(UpdatedParameters, TEXT("AverageLuminosity"), AverageLuminosity, bOverride);
	NIAGARA_UPDATE_HACK_END(NiagaraParameters->Collection)
}

void UGalaxyShapeSettings::UpdateNiagaraParameters()
{
	if (!ensureMsgf(NiagaraParameters != nullptr, TEXT("Niagara parameter collection not set")))
	{
		return;
	}

	NIAGARA_UPDATE_HACK_BEGIN(NiagaraParameters->Collection, UpdatedParameters)
	const bool bOverride = false;
	UGalaxyNiagaraFunctionLibrary::SetFloatParameter(UpdatedParameters, TEXT("Radius"), Radius, bOverride);
	UGalaxyNiagaraFunctionLibrary::SetFloatParameter(UpdatedParameters, TEXT("Velocity"), Velocity, bOverride);
	UGalaxyNiagaraFunctionLibrary::SetFloatParameter(UpdatedParameters, TEXT("Perturbation"), Perturbation, bOverride);
	UGalaxyNiagaraFunctionLibrary::SetFloatParameter(UpdatedParameters, TEXT("WindingFrequency"), WindingFrequency, bOverride);
	UGalaxyNiagaraFunctionLibrary::SetCurveParameter(UpdatedParameters, TEXT("ThicknessCurve"), ThicknessCurve->FloatCurve, bOverride);

	FRichCurve RadialSamplingCurve;
	UProbabilityCurveFunctionLibrary::ComputeQuantileRichCurve(RadialDensityCurve->FloatCurve, RadialSamplingCurve);
	UGalaxyNiagaraFunctionLibrary::SetCurveParameter(UpdatedParameters, TEXT("RadialSamplingCurve"), RadialSamplingCurve, bOverride);
	NIAGARA_UPDATE_HACK_END(NiagaraParameters->Collection)
}
