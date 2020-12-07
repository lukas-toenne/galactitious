// Fill out your copyright notice in the Description page of Project Settings.


#include "StellarSamplingData.h"

#if WITH_EDITOR
#include "GalactitiousEditor/TextureBaker.h"
#endif

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

UStellarSamplingData::UStellarSamplingData()
{
}

#if WITH_EDITOR
void UStellarSamplingData::BuildFromStellarClasses()
{
	if (!ensureMsgf(SamplingTexture != nullptr, TEXT("Sampling texture not set")))
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
		"CreateLuminositySamplingTextures",
		[&StellarClasses](const FName& Key, const FStellarClass& Value) { StellarClasses.Add(Value); });

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

	AverageLuminosity = 0.0f;
	for (int32 i = 0; i < StellarClasses.Num(); ++i)
	{
		const float LuminosityStart = StellarClasses[i].MinLuminosity;
		const float LuminosityEnd = i < StellarClasses.Num() - 1 ? StellarClasses[i + 1].MinLuminosity : MaxLuminosity;
		AverageLuminosity += 0.5f * (LuminosityStart + LuminosityEnd) * StellarClasses[i].Fraction;
	}

	// Stores logarithmic Luminosity ln(L) for more sensible values.
	// Luminosity varies by many orders of magnitude.
	FInterpCurveFloat LogLuminositySamplingCurve;
	// Number of stars represented by one particle.
	FInterpCurveFloat LogStarCountSamplingCurve;
	float TotalProbability = 0.0f;
	for (const FStellarClass& StellarClass : StellarClasses)
	{
		// Probability is tweaked such that stars with luminosity > Lf
		// can be represented by a single particle without changing overall Luminosity too much.
		const float Probability = StellarClass.MinLuminosity < AverageLuminosity
									  ? StellarClass.Fraction * StellarClass.MinLuminosity / AverageLuminosity
									  : StellarClass.Fraction;
		const float StarCount = StellarClass.MinLuminosity < AverageLuminosity ? AverageLuminosity / StellarClass.MinLuminosity : 1.0f;

		LogLuminositySamplingCurve.AddPoint(TotalProbability, FMath::Loge(StellarClass.MinLuminosity));
		LogStarCountSamplingCurve.AddPoint(TotalProbability, FMath::Loge(StarCount));

		TotalProbability += Probability;
	}
	// Final point
	{
		const float StarCount = MaxLuminosity < AverageLuminosity ? AverageLuminosity / MaxLuminosity : 1.0f;

		LogLuminositySamplingCurve.AddPoint(TotalProbability, FMath::Loge(MaxLuminosity));
		LogStarCountSamplingCurve.AddPoint(TotalProbability, FMath::Loge(StarCount));
	}
	// Normalize probability
	if (!FMath::IsNearlyZero(TotalProbability))
	{
		for (FInterpCurvePointFloat& CurvePoint : LogLuminositySamplingCurve.Points)
		{
			CurvePoint.InVal /= TotalProbability;
		}
		for (FInterpCurvePointFloat& CurvePoint : LogStarCountSamplingCurve.Points)
		{
			CurvePoint.InVal /= TotalProbability;
		}
	}

	SamplingTexture = FTextureBaker::BakeTexture<FVector4_16>(
		SamplingTexture->GetPathName(), 512, PF_A32B32G32R32F, TSF_RGBA16F, TMGS_NoMipmaps,
		[LogLuminositySamplingCurve, LogStarCountSamplingCurve](float X) -> FVector4_16 {
			FVector4_16 Result;
			Result.X = LogLuminositySamplingCurve.Eval(X);
			Result.Y = LogStarCountSamplingCurve.Eval(X);
			Result.Z = 0.0f;
			Result.W = 1.0f;
			return Result;
		});
}
#endif
