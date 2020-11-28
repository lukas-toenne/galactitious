// Fill out your copyright notice in the Description page of Project Settings.


#include "ProbabilityCurveFunctionLibrary.h"

#include "DrawDebugHelpers.h"

UTexture2D* UProbabilityCurveFunctionLibrary::CurveToTexture2D(const FInterpCurveFloat& Curve, int32 Resolution)
{
	if (!ensureMsgf(Resolution >= 1, TEXT("Sampling resolution must be at least 1")))
	{
		return nullptr;
	}
	if (!ensure(Curve.Points.Num() > 0))
	{
		return nullptr;
	}

	const float MinTime = Curve.Points[0].InVal;
	const float MaxTime = Curve.Points[Curve.Points.Num() - 1].InVal;
	const float DeltaTime = (MaxTime - MinTime) / (float)(Resolution - 1);

	int32 Width = Resolution;
	int32 Height = 1;
	// Grayscale would suffice for storing a curve, but cannot be exported by UImageWriteBlueprintLibrary::ExportToDisk.
	// Use full RGBA instead.
	EPixelFormat PixelFormat = PF_A32B32G32R32F;

	UTexture2D* NewTexture = UTexture2D::CreateTransient(Width, Height, PixelFormat);
	if (NewTexture)
	{
		NewTexture->MipGenSettings = TMGS_NoMipmaps;
		NewTexture->AddressX = TextureAddress::TA_Clamp;
		NewTexture->AddressY = TextureAddress::TA_Clamp;

		{
			FVector4* MipData = static_cast<FVector4*>(NewTexture->PlatformData->Mips[0].BulkData.Lock(LOCK_READ_WRITE));
			float Time = MinTime;
			for (int i = 0; i < Resolution; ++i)
			{
				float Value = Curve.Eval(Time);
				(MipData++)->Set(Value, Value, Value, 1.0f);
				Time += DeltaTime;
			}

			NewTexture->PlatformData->Mips[0].BulkData.Unlock();
		}

		NewTexture->UpdateResource();
	}

	return NewTexture;
}

bool UProbabilityCurveFunctionLibrary::GetAssetFilename(const UObject* Asset, FString& Filename)
{
	Filename.Empty();
	if (const UPackage* Package = Asset->GetPackage())
	{
		// This is a package in memory that has not yet been saved. Determine the extension and convert to a filename
		const FString* PackageExtension =
			Package->ContainsMap() ? &FPackageName::GetMapPackageExtension() : &FPackageName::GetAssetPackageExtension();
		return FPackageName::TryConvertLongPackageNameToFilename(Package->GetName(), Filename, *PackageExtension);
	}
	return false;
}

namespace
{
	EInterpCurveMode ConvertInterpolationMode(ERichCurveInterpMode Mode)
	{
		switch (Mode)
		{
		case RCIM_None:
			return CIM_Unknown;
		case RCIM_Constant:
			return CIM_Constant;
		case RCIM_Linear:
			return CIM_Linear;
		case RCIM_Cubic:
			return CIM_CurveBreak;
		}
		return CIM_Unknown;
	}
}

void UProbabilityCurveFunctionLibrary::ConvertCurveAsset(UCurveFloat* CurveAsset, FInterpCurveFloat& Curve)
{
	Curve.bIsLooped = false;
	Curve.LoopKeyOffset = 0.0f;

	if (!ensure(CurveAsset != nullptr))
	{
		Curve.Points.Empty();
		return;
	}

	const FRichCurve& FloatCurve = CurveAsset->FloatCurve;
	const int32 NumKeys = FloatCurve.GetNumKeys();
	if (NumKeys == 0)
	{
		Curve.Points.Empty();
		return;
	}

	Curve.Points.SetNum(NumKeys);

	for (auto KeyIter = FloatCurve.GetKeyIterator(); KeyIter; ++KeyIter)
	{
		FInterpCurvePointFloat& Point = Curve.Points[KeyIter.GetIndex()];

		Point.InVal = KeyIter->Time;
		Point.OutVal = KeyIter->Value;
		Point.ArriveTangent = KeyIter->ArriveTangent;
		Point.LeaveTangent = KeyIter->LeaveTangent;
		Point.InterpMode = ConvertInterpolationMode(KeyIter->InterpMode);
	}
}

namespace
{
	/**
	 * Find extrema of the cubic hermite spline through points p0 and p1 with derivatives m0 and m1.
	 * Returns the number N of possible extrema. Only the first N values of OutExtrema are valid.
	 */
	int FindExtrema(float p0, float p1, float m0, float m1, float OutExtrema[2])
	{
		const float a = 6.0f * (p0 - p1) + 3.0f * (m0 + m1);
		const float b = 6.0f * (p1 - p0) - 4.0f * m0 - 2.0f * m1;
		const float c = m0;

		// Quadratic formula
		const bool aIsZero = FMath::IsNearlyZero(a);
		const bool bIsZero = FMath::IsNearlyZero(b);
		if (aIsZero)
		{
			if (bIsZero)
			{
				// Curve is constant: f(t) == c
				return 0;
			}
			else
			{
				OutExtrema[0] = -c / b;
				return 1;
			}
		}
		else
		{
			const float q = b * b - 4.0f * a * c;
			if (q < .0f)
			{
				// No real extrema
				return 0;
			}
			else if (FMath::IsNearlyZero(q))
			{
				// One extremum
				OutExtrema[0] = - b / (2.0f * a);
				return 1;
			}
			else
			{
				const float sq = FMath::Sqrt(q);
				// Keep extrema sorted
				if (a >= .0f)
				{
					OutExtrema[0] = (-b - sq) / (2.0f * a);
					OutExtrema[1] = (-b + sq) / (2.0f * a);
				}
				else
				{
					OutExtrema[0] = (-b + sq) / (2.0f * a);
					OutExtrema[1] = (-b - sq) / (2.0f * a);
				}
				return 2;
			}
		}
	}
} // namespace

void UProbabilityCurveFunctionLibrary::IntegrateCurve(const FInterpCurveFloat& Curve, float Offset, FInterpCurveFloat& IntegratedCurve)
{
	IntegratedCurve.bIsLooped = false;
	IntegratedCurve.LoopKeyOffset = 0.0f;
	IntegratedCurve.Points.Empty();
	if (Curve.Points.Num() == 0)
	{
		return;
	}

	// Reserve up to 3 points per segment to account for additional extrema points where needed.
	// See cubic spline integration case below.
	IntegratedCurve.Points.Reserve((Curve.Points.Num() - 1) * 3 + 1);

	float Value = Offset;
	// First point
	{
		const FInterpCurvePointFloat& Point = Curve.Points[0];
		IntegratedCurve.Points.Add(FInterpCurvePointFloat(Point.InVal, Value, Point.OutVal, .0f, EInterpCurveMode::CIM_Unknown));
	}

	for (int i = 0; i < Curve.Points.Num() - 1; ++i)
	{
		const FInterpCurvePointFloat& Point = Curve.Points[i];
		const FInterpCurvePointFloat& NextPoint = Curve.Points[i + 1];

		if (Point.InterpMode == EInterpCurveMode::CIM_Constant)
		{
			const float Time0 = Point.InVal;
			const float Time1 = NextPoint.InVal;
			const float DeltaTime = Time1 - Time0;
			const float Slope0 = Point.OutVal;
			FInterpCurvePointFloat& CurrentPoint = IntegratedCurve.Points.Last();

			Value += Slope0 * DeltaTime;

			CurrentPoint.LeaveTangent = Slope0;
			CurrentPoint.InterpMode = EInterpCurveMode::CIM_Linear;
			IntegratedCurve.Points.Add(FInterpCurvePointFloat(Time1, Value, Slope0, .0f, EInterpCurveMode::CIM_Unknown));
			break;
		}
		else if (Point.InterpMode == EInterpCurveMode::CIM_Linear)
		{
			const float Time0 = Point.InVal;
			const float Time1 = NextPoint.InVal;
			const float DeltaTime = Time1 - Time0;
			const float Slope0 = Point.OutVal;
			const float Slope1 = NextPoint.OutVal;
			FInterpCurvePointFloat& CurrentPoint = IntegratedCurve.Points.Last();

			Value += (Slope0 + Slope1) * DeltaTime * 0.5f;

			CurrentPoint.LeaveTangent = Slope0;
			CurrentPoint.InterpMode = EInterpCurveMode::CIM_CurveBreak;
			IntegratedCurve.Points.Add(FInterpCurvePointFloat(Time1, Value, Slope1, .0f, EInterpCurveMode::CIM_Unknown));
		}
		else if (Point.IsCurveKey())
		{
			// Cannot go above cubic splines, so use linear interpolation between extrema to minimize errors.

			const float TotalDeltaTime = NextPoint.InVal - Point.InVal;
			float CurrentTime = Point.InVal;
			float CurrentSlope = Point.OutVal;

			float Extrema[2];
			int NumExtrema = FindExtrema(
				Point.OutVal, NextPoint.OutVal, Point.LeaveTangent * TotalDeltaTime, NextPoint.ArriveTangent * TotalDeltaTime,
				Extrema);
			for (int a = 0; a < NumExtrema; ++a)
			{
				if (Extrema[a] <= .0f || Extrema[a] >= 1.0f)
				{
					continue;
				}

				const float NextTime = CurrentTime + Extrema[a] * TotalDeltaTime;
				const float NextSlope = Curve.Eval(NextTime);
				FInterpCurvePointFloat& CurrentPoint = IntegratedCurve.Points.Last();

				Value += (CurrentSlope + NextSlope) * (NextTime - CurrentTime) * 0.5f;

				CurrentPoint.LeaveTangent = CurrentSlope;
				CurrentPoint.InterpMode = EInterpCurveMode::CIM_CurveBreak;
				IntegratedCurve.Points.Add(FInterpCurvePointFloat(NextTime, Value, NextSlope, .0f, EInterpCurveMode::CIM_Unknown));

				CurrentTime = NextTime;
				CurrentSlope = NextSlope;
			}

			// Last point of segment
			const float NextTime = NextPoint.InVal;
			const float NextSlope = NextPoint.OutVal;
			FInterpCurvePointFloat& CurrentPoint = IntegratedCurve.Points.Last();

			Value += (CurrentSlope + NextSlope) * (NextTime - CurrentTime) * 0.5f;

			CurrentPoint.LeaveTangent = CurrentSlope;
			CurrentPoint.InterpMode = EInterpCurveMode::CIM_CurveBreak;
			IntegratedCurve.Points.Add(FInterpCurvePointFloat(NextTime, Value, NextSlope, .0f, EInterpCurveMode::CIM_Unknown));
		}
		else
		{
			const float Time1 = NextPoint.InVal;
			FInterpCurvePointFloat& CurrentPoint = IntegratedCurve.Points.Last();

			// Unknown
			CurrentPoint.LeaveTangent = .0f;
			CurrentPoint.InterpMode = EInterpCurveMode::CIM_Unknown;
			IntegratedCurve.Points.Add(FInterpCurvePointFloat(Time1, Value, .0f, .0f, EInterpCurveMode::CIM_Unknown));
		}
	}

	// Last point
	{
		const FInterpCurvePointFloat& Point = Curve.Points[Curve.Points.Num() - 1];
		FInterpCurvePointFloat& CurrentPoint = IntegratedCurve.Points.Last();

		CurrentPoint.LeaveTangent = Point.OutVal;
		CurrentPoint.InterpMode = EInterpCurveMode::CIM_CurveBreak;
	}
}

void UProbabilityCurveFunctionLibrary::NormalizeCurve(const FInterpCurveFloat& Curve, FInterpCurveFloat& NormalizedCurve)
{
	NormalizedCurve.bIsLooped = Curve.bIsLooped;
	NormalizedCurve.LoopKeyOffset = Curve.LoopKeyOffset;

	if (Curve.Points.Num() == 0)
	{
		NormalizedCurve.Points.Empty();
		return;
	}

	NormalizedCurve.Points.SetNum(Curve.Points.Num());

	float MinValue, MaxValue;
	Curve.CalcBounds(MinValue, MaxValue, 0.0f);
	const float InvScale = FMath::IsNearlyZero(MaxValue - MinValue) ? 0.0f : 1.0f / (MaxValue - MinValue);

	for (int i = 0; i < Curve.Points.Num(); ++i)
	{
		const FInterpCurvePointFloat& Point = Curve.Points[i];
		FInterpCurvePointFloat& NormalizedPoint = NormalizedCurve.Points[i];

		NormalizedPoint.InVal = Point.InVal;
		NormalizedPoint.OutVal = (Point.OutVal - MinValue) * InvScale;
		NormalizedPoint.ArriveTangent = Point.ArriveTangent * InvScale;
		NormalizedPoint.LeaveTangent = Point.LeaveTangent * InvScale;
		NormalizedPoint.InterpMode = Point.InterpMode;
	}
}

void UProbabilityCurveFunctionLibrary::InvertCurve(const FInterpCurveFloat& Curve, FInterpCurveFloat& InvertedCurve)
{
	const float MonotoneEpsilon = 1.0e-9f;

	InvertedCurve.bIsLooped = Curve.bIsLooped;
	InvertedCurve.LoopKeyOffset = Curve.LoopKeyOffset;
	InvertedCurve.Points.Empty();

	float LastValue = 0.0f;
	for (int i = 0; i < Curve.Points.Num(); ++i)
	{
		const FInterpCurvePointFloat& Point = Curve.Points[i];
		if (!ensureMsgf(i == 0 || Point.OutVal >= LastValue, TEXT("Curve values must increase monotonically to be invertible")))
		{
			break;
		}

		FInterpCurvePointFloat& InvPoint = InvertedCurve.Points.Emplace_GetRef();
		InvPoint.InVal = Point.OutVal;
		InvPoint.OutVal = Point.InVal;
		const bool bArriveZero = FMath::IsNearlyZero(Point.ArriveTangent, MonotoneEpsilon);
		const bool bLeaveZero = FMath::IsNearlyZero(Point.LeaveTangent, MonotoneEpsilon);
		InvPoint.ArriveTangent = bArriveZero ? 0.0f : 1.0f / Point.ArriveTangent;
		InvPoint.LeaveTangent = bLeaveZero ? 0.0f : 1.0f / Point.LeaveTangent;
		InvPoint.InterpMode = Point.InterpMode;

		const FInterpCurvePointFloat& NextPoint = (i < Curve.Points.Num() - 1 ? Curve.Points[i + 1] : Point);
		const bool bRequireJumpPoint =
			Point.InterpMode == EInterpCurveMode::CIM_Constant ||
			(i < Curve.Points.Num() - 1 && NextPoint.OutVal < Point.OutVal + MonotoneEpsilon);
		if (bRequireJumpPoint)
		{
			FInterpCurvePointFloat& JumpPoint = InvertedCurve.Points.Emplace_GetRef();
			JumpPoint.InVal = Point.OutVal + MonotoneEpsilon;
			JumpPoint.OutVal = NextPoint.InVal;

			JumpPoint.LeaveTangent = InvPoint.LeaveTangent;
			InvPoint.LeaveTangent = 0.0f;
			JumpPoint.ArriveTangent = 0.0f;

			JumpPoint.InterpMode = InvPoint.InterpMode;
			InvPoint.InterpMode = EInterpCurveMode::CIM_Constant;
		}
	}
}

void UProbabilityCurveFunctionLibrary::DrawDebugCurve(
	const UObject* WorldContextObject, const FInterpCurveFloat& Curve, const FTransform& Transform,
	const FDrawDebugCurveSettings& Settings, bool bPersistentLines, float LifeTime, uint8 DepthPriority)
{
	if (!ensure(Settings.CurveResolution >= 1))
	{
		return;
	}

	if (UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::LogAndReturnNull))
	{
		// Draw control points
		for (int32 i = 0; i < Curve.Points.Num(); ++i)
		{
			const FInterpCurvePointFloat& Point = Curve.Points[i];
			const FVector Position = Transform.TransformPosition(FVector(Point.InVal, .0f, Point.OutVal));

			DrawDebugPoint(World, Position, Settings.PointSize, Settings.PointColor, bPersistentLines, LifeTime, DepthPriority);
		}

		// Draw tangents
		for (int32 i = 0; i < Curve.Points.Num(); ++i)
		{
			const FInterpCurvePointFloat& Point = Curve.Points[i];
			FVector Position = Transform.TransformPosition(FVector(Point.InVal, .0f, Point.OutVal));

			if (i > 0)
			{
				const FInterpCurvePointFloat& PrevPoint = Curve.Points[i - 1];
				const float ArriveTime = (Point.InVal - PrevPoint.InVal) / 3.0f;
				const float ArriveInVal = (Point.InVal * 2.0f + PrevPoint.InVal) / 3.0f;

				FVector ArrivePos;
				if (PrevPoint.InterpMode == EInterpCurveMode::CIM_Constant)
				{
					ArrivePos = Transform.TransformPosition(FVector(Point.InVal, .0f, PrevPoint.OutVal));
				}
				else if (PrevPoint.InterpMode == EInterpCurveMode::CIM_Linear)
				{
					ArrivePos = Transform.TransformPosition(FVector(ArriveInVal, .0f, (Point.OutVal * 2.0f + PrevPoint.OutVal) / 3.0f));
				}
				else if (PrevPoint.IsCurveKey())
				{
					ArrivePos = Transform.TransformPosition(FVector(ArriveInVal, .0f, Point.OutVal - Point.ArriveTangent * ArriveTime));
				}
				else
				{
					ArrivePos = Position;
				}

				DrawDebugLine(World, ArrivePos, Position, Settings.TangentColor, bPersistentLines, LifeTime, DepthPriority, Settings.TangentThickness);
			}

			if (i < Curve.Points.Num() - 1)
			{
				const FInterpCurvePointFloat& NextPoint = Curve.Points[i + 1];
				const float LeaveTime = (NextPoint.InVal - Point.InVal) / 3.0f;
				const float LeaveInVal = (Point.InVal * 2.0f + NextPoint.InVal) / 3.0f;

				FVector LeavePos;
				if (Point.InterpMode == EInterpCurveMode::CIM_Constant)
				{
					LeavePos = Transform.TransformPosition(FVector(Point.InVal, .0f, Point.OutVal));
				}
				else if (Point.InterpMode == EInterpCurveMode::CIM_Linear)
				{
					LeavePos = Transform.TransformPosition(FVector(LeaveInVal, .0f, (Point.OutVal * 2.0f + NextPoint.OutVal) / 3.0f));
				}
				else if (Point.IsCurveKey())
				{
					LeavePos = Transform.TransformPosition(FVector(LeaveInVal, .0f, Point.OutVal + Point.LeaveTangent * LeaveTime));
				}
				else
				{
					LeavePos = Position;
				}

				DrawDebugLine(World, Position, LeavePos, Settings.TangentColor, bPersistentLines, LifeTime, DepthPriority, Settings.TangentThickness);
			}
		}

		// Draw curve
		for (int32 i = 0; i < Curve.Points.Num() - 1; ++i)
		{
			const FInterpCurvePointFloat& Point = Curve.Points[i];
			const FInterpCurvePointFloat& NextPoint = Curve.Points[i + 1];

			if (Point.InterpMode == EInterpCurveMode::CIM_Constant)
			{
				FVector Position = Transform.TransformPosition(FVector(Point.InVal, .0f, Point.OutVal));
				FVector NextPosition = Transform.TransformPosition(FVector(NextPoint.InVal, .0f, Point.OutVal));
				DrawDebugLine(World, Position, NextPosition, Settings.LineColor, bPersistentLines, LifeTime, DepthPriority, Settings.LineThickness);
			}
			else if (Point.InterpMode == EInterpCurveMode::CIM_Linear)
			{
				FVector Position = Transform.TransformPosition(FVector(Point.InVal, .0f, Point.OutVal));
				FVector NextPosition = Transform.TransformPosition(FVector(NextPoint.InVal, .0f, NextPoint.OutVal));
				DrawDebugLine(World, Position, NextPosition, Settings.LineColor, bPersistentLines, LifeTime, DepthPriority, Settings.LineThickness);
			}
			// Any of the curve modes
			else if (Point.IsCurveKey())
			{
				const float dx = (NextPoint.InVal - Point.InVal) / Settings.CurveResolution;

				float x = Point.InVal;
				FVector Position = Transform.TransformPosition(FVector(x, .0f, Curve.Eval(x, .0f)));

				for (int32 a = 0; a < Settings.CurveResolution ; ++a)
				{
					// TODO Forward differencing is more efficient for eval at constant intervals,
					// not necessary for debug drawing though.

					x += dx;
					FVector NextPosition = Transform.TransformPosition(FVector(x, .0f, Curve.Eval(x, .0f)));

					DrawDebugLine(World, Position, NextPosition, Settings.LineColor, bPersistentLines, LifeTime, DepthPriority, Settings.LineThickness);

					Position = NextPosition;
				}
			}
			// Unknown
			else
			{
				continue;
			}
		}
	}
}
