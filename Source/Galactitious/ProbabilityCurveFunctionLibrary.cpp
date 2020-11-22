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
	EPixelFormat PixelFormat = PF_R32_FLOAT;

	UTexture2D* NewTexture = UTexture2D::CreateTransient(Width, Height, PixelFormat);
	if (NewTexture)
	{
		NewTexture->MipGenSettings = TMGS_NoMipmaps;
		NewTexture->AddressX = TextureAddress::TA_Clamp;
		NewTexture->AddressY = TextureAddress::TA_Clamp;

		{
			float* MipData = static_cast<float*>(NewTexture->PlatformData->Mips[0].BulkData.Lock(LOCK_READ_WRITE));
			float Time = MinTime;
			for (int i = 0; i < Resolution; ++i)
			{
				*(MipData++) = Curve.Eval(Time);
				Time += DeltaTime;
			}

			NewTexture->PlatformData->Mips[0].BulkData.Unlock();
		}

		NewTexture->UpdateResource();
	}

	return NewTexture;
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

void UProbabilityCurveFunctionLibrary::IntegrateCurve(const FInterpCurveFloat& Curve, float Offset, FInterpCurveFloat& IntegratedCurve)
{
	IntegratedCurve.bIsLooped = false;
	IntegratedCurve.LoopKeyOffset = 0.0f;

	if (Curve.Points.Num() == 0)
	{
		IntegratedCurve.Points.Empty();
		return;
	}

	IntegratedCurve.Points.SetNum(Curve.Points.Num());

	float Value = Offset;

	for (int i = 0; i < Curve.Points.Num() - 1; ++i)
	{
		const FInterpCurvePointFloat& Point = Curve.Points[i];
		const FInterpCurvePointFloat& NextPoint = Curve.Points[i + 1];
		const float DeltaTime = NextPoint.InVal - Point.InVal;
		const float InvDeltaTime = FMath::IsNearlyZero(DeltaTime) ? 0.0f : 1.0f / DeltaTime;
		FInterpCurvePointFloat& IntPoint = IntegratedCurve.Points[i];
		FInterpCurvePointFloat& NextIntPoint = IntegratedCurve.Points[i + 1];

		// First point
		if (i == 0)
		{
			IntPoint.ArriveTangent = Point.OutVal;
		}

		IntPoint.InVal = Point.InVal;
		IntPoint.OutVal = Value;

		switch (Point.InterpMode)
		{
		case EInterpCurveMode::CIM_Constant:
		case EInterpCurveMode::CIM_Unknown:
		{
			float Slope = Point.OutVal;

			IntPoint.LeaveTangent = Slope;
			NextIntPoint.ArriveTangent = Slope;
			IntPoint.InterpMode = EInterpCurveMode::CIM_Linear;

			Value += Slope * DeltaTime;
			break;
		}

		case EInterpCurveMode::CIM_Linear:
		{
			float Slope0 = Point.OutVal;
			float Slope1 = NextPoint.OutVal;

			IntPoint.LeaveTangent = Slope0;
			NextIntPoint.ArriveTangent = Slope1;
			IntPoint.InterpMode = EInterpCurveMode::CIM_CurveBreak;

			Value += ((Slope1 - Slope0) / 2.0f + Slope0) * DeltaTime;
			break;
		}

		case EInterpCurveMode::CIM_CurveAuto:
		case EInterpCurveMode::CIM_CurveAutoClamped:
		case EInterpCurveMode::CIM_CurveBreak:
		case EInterpCurveMode::CIM_CurveUser:
		{
			float Slope0 = Point.OutVal;
			float Slope1 = NextPoint.OutVal;
			float Curv0 = Point.LeaveTangent;
			float Curv1 = NextPoint.ArriveTangent;

			IntPoint.LeaveTangent = Slope0;
			NextIntPoint.ArriveTangent = Slope1;
			IntPoint.InterpMode = EInterpCurveMode::CIM_CurveBreak;

			// XXX Using curvature can lead to overshooting curves, only linear interpolation used below
			//Value +=
			//	((((2.0f * (Slope1 - Slope0) - Curv0 - Curv1) / 4.0f * DeltaTime
			//	   + (Slope0 - Slope1 + Curv1) / 3.0f) * DeltaTime
			//	  + Curv0 / 2.0f) * DeltaTime
			//	 + Slope0) * DeltaTime;
			Value += ((Slope1 - Slope0) / 2.0f * DeltaTime + Slope0) * DeltaTime;
			break;
		}
		}
	}

	// Last point
	{
		const FInterpCurvePointFloat& Point = Curve.Points[Curve.Points.Num() - 1];
		FInterpCurvePointFloat& IntPoint = IntegratedCurve.Points[Curve.Points.Num() - 1];

		IntPoint.InVal = Point.InVal;
		IntPoint.OutVal = Value;
		IntPoint.LeaveTangent = Point.OutVal;
		IntPoint.InterpMode = EInterpCurveMode::CIM_CurveBreak;
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

struct FDrawDebugCurveSettings
{
	float PointSize = 3.0f;
	FColor PointColor = FColor::White;

	float LineThickness = 0.f;
	FColor LineColor = FColor(170, 170, 170, 255);

	float TangentThickness = 0.f;
	FColor TangentColor = FColor::Yellow;
};

void UProbabilityCurveFunctionLibrary::DrawDebugCurve(
	const UObject* WorldContextObject, const FInterpCurveFloat& Curve, const FTransform& Transform,
	int32 Resolution, bool bPersistentLines, float LifeTime, uint8 DepthPriority)
{
	static const FDrawDebugCurveSettings Settings;

	if (!ensure(Resolution >= 1))
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

		///** A straight line between two keypoint values. */
		//CIM_Linear,
		///** A cubic-hermite curve between two keypoints, using Arrive/Leave tangents. These tangents will be automatically
		//	updated when points are moved, etc.  Tangents are unclamped and will plateau at curve start and end points. */
		//CIM_CurveAuto,
		///** The out value is held constant until the next key, then will jump to that value. */
		//CIM_Constant,
		///** A smooth curve just like CIM_Curve, but tangents are not automatically updated so you can have manual control over them (eg. in Curve Editor). */
		//CIM_CurveUser,
		///** A curve like CIM_Curve, but the arrive and leave tangents are not forced to be the same, so you can create a 'corner' at this key. */
		//CIM_CurveBreak,
		///** A cubic-hermite curve between two keypoints, using Arrive/Leave tangents. These tangents will be automatically
		//	updated when points are moved, etc.  Tangents are clamped and will plateau at curve start and end points. */
		//CIM_CurveAutoClamped,
		///** Invalid or unknown curve type. */
		//CIM_Unknown

		// IsCurveKey?
		// InterpMode in { CIM_CurveAuto, CIM_CurveAutoClamped, CIM_CurveUser, CIM_CurveBreak }

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
				const float dx = (NextPoint.InVal - Point.InVal) / Resolution;

				float x = Point.InVal;
				FVector Position = Transform.TransformPosition(FVector(x, .0f, Curve.Eval(x, .0f)));

				for (int32 a = 0; a < Resolution; ++a)
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
