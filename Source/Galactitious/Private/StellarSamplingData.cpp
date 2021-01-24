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
	// Quantile function for picking star luminosity Ls for particle:
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
	// XXX Arbitrary: use constant temperature for the most luminous class
	const float MaxTemperature = StellarClasses.Last().MinTemperature;

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
	// Temperature of stars
	FRichCurve TemperatureSamplingCurve;
	float TotalProbability = 0.0f;
	for (int32 i = 0; i < StellarClasses.Num(); ++i)
	{
		const FStellarClass& StellarClass = StellarClasses[i];

		// Probability is tweaked such that stars with luminosity > Lf
		// can be represented by a single particle without changing overall Luminosity too much.
		const float Luminosity = StellarClass.MinLuminosity;
		const float Probability = StellarClass.Fraction * Luminosity / AverageLuminosity;

		FKeyHandle Handle;
		Handle = LogLuminositySamplingCurve.AddKey(TotalProbability, FMath::Loge(Luminosity));
		LogLuminositySamplingCurve.SetKeyInterpMode(Handle, RCIM_Linear);
		Handle = TemperatureSamplingCurve.AddKey(TotalProbability, StellarClass.MinTemperature);
		TemperatureSamplingCurve.SetKeyInterpMode(Handle, RCIM_Linear);

		TotalProbability += Probability;
	}
	// Final point
	{
		const float Luminosity = MaxLuminosity;

		FKeyHandle Handle;
		Handle = LogLuminositySamplingCurve.AddKey(TotalProbability, FMath::Loge(Luminosity));
		LogLuminositySamplingCurve.SetKeyInterpMode(Handle, RCIM_Linear);
		Handle = TemperatureSamplingCurve.AddKey(TotalProbability, MaxTemperature);
		TemperatureSamplingCurve.SetKeyInterpMode(Handle, RCIM_Linear);
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
		for (FRichCurveKey& Key : TemperatureSamplingCurve.Keys)
		{
			Key.Time /= TotalProbability;
			Key.ArriveTangent *= TotalProbability;
			Key.LeaveTangent *= TotalProbability;
		}
	}

	UGalaxyNiagaraFunctionLibrary::ApplyParameterCollectionUpdate(
		NiagaraParameters, [=](UNiagaraParameterCollectionInstance* UpdatedParameters) {
			const bool bOverride = false;
			UGalaxyNiagaraFunctionLibrary::SetCurveParameter(
				UpdatedParameters, TEXT("LuminositySamplingCurve"), LogLuminositySamplingCurve, bOverride);
			UGalaxyNiagaraFunctionLibrary::SetFloatParameter(UpdatedParameters, TEXT("AverageLuminosity"), AverageLuminosity, bOverride);
			UGalaxyNiagaraFunctionLibrary::SetCurveParameter(
				UpdatedParameters, TEXT("TemperatureSamplingCurve"), TemperatureSamplingCurve, bOverride);
		});
}

void UGalaxyShapeSettings::UpdateNiagaraParameters()
{
	if (!ensureMsgf(NiagaraParameters != nullptr, TEXT("Niagara parameter collection not set")))
	{
		return;
	}

	UGalaxyNiagaraFunctionLibrary::ApplyParameterCollectionUpdate(
		NiagaraParameters, [=](UNiagaraParameterCollectionInstance* UpdatedParameters) {
			const bool bOverride = false;
			UGalaxyNiagaraFunctionLibrary::SetFloatParameter(UpdatedParameters, TEXT("Radius"), Radius, bOverride);
			UGalaxyNiagaraFunctionLibrary::SetFloatParameter(UpdatedParameters, TEXT("Velocity"), Velocity, bOverride);
			UGalaxyNiagaraFunctionLibrary::SetFloatParameter(UpdatedParameters, TEXT("Perturbation"), Perturbation, bOverride);
			UGalaxyNiagaraFunctionLibrary::SetFloatParameter(UpdatedParameters, TEXT("WindingFrequency"), WindingFrequency, bOverride);
			UGalaxyNiagaraFunctionLibrary::SetCurveParameter(
				UpdatedParameters, TEXT("ThicknessCurve"), ThicknessCurve->FloatCurve, bOverride);

			FRichCurve RadialDensityNormalizedCurve, RadialSamplingCurve;
			UProbabilityCurveFunctionLibrary::ComputeQuantileRichCurve(
				RadialDensityCurve->FloatCurve, RadialDensityNormalizedCurve, RadialSamplingCurve);
			UGalaxyNiagaraFunctionLibrary::SetCurveParameter(
				UpdatedParameters, TEXT("RadialDensityCurve"), RadialDensityNormalizedCurve, bOverride);
			UGalaxyNiagaraFunctionLibrary::SetCurveParameter(
				UpdatedParameters, TEXT("RadialSamplingCurve"), RadialSamplingCurve, bOverride);
		});
}
