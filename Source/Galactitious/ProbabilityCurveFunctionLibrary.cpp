// Fill out your copyright notice in the Description page of Project Settings.

#include "ProbabilityCurveFunctionLibrary.h"

#include "DrawDebugHelpers.h"

#include "Curves/CurveFloat.h"
#include "Curves/CurveLinearColor.h"

DEFINE_LOG_CATEGORY_STATIC(LogProbabilityCurve, Log, All);

namespace
{
	EInterpCurveMode ConvertInterpolationMode(ERichCurveInterpMode InterpMode, ERichCurveTangentMode TangentMode)
	{
		switch (InterpMode)
		{
		case RCIM_None:
			return CIM_Unknown;
		case RCIM_Constant:
			return CIM_Constant;
		case RCIM_Linear:
			return CIM_Linear;
		case RCIM_Cubic:
		{
			switch (TangentMode)
			{
			case RCTM_Auto:
				return CIM_CurveAuto;
			case RCTM_Break:
				return CIM_CurveBreak;
			case RCTM_User:
				return CIM_CurveUser;
			case RCTM_None:
				return CIM_Unknown;
			}
		}
		}
		return CIM_Unknown;
	}

	void ConvertInterpolationMode(EInterpCurveMode CurveMode, ERichCurveInterpMode& OutInterpMode, ERichCurveTangentMode& OutTangentMode)
	{
		switch (CurveMode)
		{
		case CIM_Unknown:
			OutInterpMode = RCIM_None;
			OutTangentMode = RCTM_None;
		case CIM_Constant:
			OutInterpMode = RCIM_Constant;
			OutTangentMode = RCTM_None;
		case CIM_Linear:
			OutInterpMode = RCIM_Linear;
			OutTangentMode = RCTM_None;
		case CIM_CurveAuto:
			OutInterpMode = RCIM_Cubic;
			OutTangentMode = RCTM_Auto;
		case CIM_CurveAutoClamped:
			OutInterpMode = RCIM_Cubic;
			OutTangentMode = RCTM_Auto;
		case CIM_CurveBreak:
			OutInterpMode = RCIM_Cubic;
			OutTangentMode = RCTM_Break;
		case CIM_CurveUser:
			OutInterpMode = RCIM_Cubic;
			OutTangentMode = RCTM_User;
		}
	}
} // namespace

void UProbabilityCurveFunctionLibrary::ConvertFloatCurveAsset(UCurveFloat* CurveAsset, FInterpCurveFloat& Curve)
{
	if (!ensure(CurveAsset != nullptr))
	{
		Curve.Points.Empty();
		return;
	}

	ConvertFromRichCurve(CurveAsset->FloatCurve, Curve);
}

namespace
{
	bool EnsureCompatibleInterpMode(EInterpCurveMode ModeA, EInterpCurveMode ModeB)
	{
		if (ensureMsgf(
				ModeA != EInterpCurveMode::CIM_Unknown && ModeB != EInterpCurveMode::CIM_Unknown,
				TEXT("Incompatible interpolation mode: unknown mode")))
		{
			if (ensureMsgf(
					(ModeA == EInterpCurveMode::CIM_Constant) == (ModeB == EInterpCurveMode::CIM_Constant),
					TEXT("Incompatible interpolation mode: only one key is constant")))
			{
				if (ensureMsgf(
						(ModeA == EInterpCurveMode::CIM_Linear) == (ModeB == EInterpCurveMode::CIM_Linear),
						TEXT("Incompatible interpolation mode: only one key is linear")))
				{
					return true;
				}
			}
		}
		return false;
	}
} // namespace

void UProbabilityCurveFunctionLibrary::ConvertColorCurveAsset(UCurveLinearColor* CurveAsset, FInterpCurveLinearColor& Curve)
{
	Curve.bIsLooped = false;
	Curve.LoopKeyOffset = 0.0f;

	if (!ensure(CurveAsset != nullptr))
	{
		Curve.Points.Empty();
		return;
	}

	for (int i = 0; i < 4; ++i)
	{
		FInterpCurveFloat FloatCurve;
		ConvertFromRichCurve(CurveAsset->FloatCurves[i], FloatCurve);

		int32 LastValidIndex = 0;
		for (auto KeyIter = FloatCurve.Points.CreateConstIterator(); KeyIter; ++KeyIter)
		{
			bool bPointFound = false;
			int32 ExistingIndex = -1;

			if (Curve.Points.Num() > 0)
			{
				ExistingIndex = Curve.GetPointIndexForInputValue(KeyIter->InVal);
				if (ExistingIndex >= 0)
				{
					FInterpCurvePointLinearColor& ExistingPoint = Curve.Points[ExistingIndex];
					if (FMath::IsNearlyEqual(KeyIter->InVal, ExistingPoint.InVal))
					{
						if (EnsureCompatibleInterpMode(KeyIter->InterpMode, ExistingPoint.InterpMode))
						{
							ExistingPoint.OutVal.Component(i) = KeyIter->OutVal;
							ExistingPoint.ArriveTangent.Component(i) = KeyIter->ArriveTangent;
							ExistingPoint.LeaveTangent.Component(i) = KeyIter->LeaveTangent;
						}
						bPointFound = true;
					}
				}
			}

			if (!bPointFound)
			{
				FInterpCurvePointLinearColor NewPoint;
				NewPoint.InterpMode = KeyIter->InterpMode;
				NewPoint.InVal = KeyIter->InVal;
				NewPoint.OutVal.Component(i) = KeyIter->OutVal;
				NewPoint.ArriveTangent.Component(i) = KeyIter->ArriveTangent;
				NewPoint.LeaveTangent.Component(i) = KeyIter->LeaveTangent;

				// Fill other components from existing curves
				FLinearColor CurrentValue = Curve.Eval(KeyIter->InVal);
				FLinearColor CurrentTangent = Curve.EvalDerivative(KeyIter->InVal);
				for (int k = 0; k < i - 1; ++k)
				{
					NewPoint.OutVal.Component(k) = CurrentValue.Component(k);
					NewPoint.ArriveTangent.Component(k) = CurrentTangent.Component(k);
					NewPoint.LeaveTangent.Component(k) = CurrentTangent.Component(k);
				}

				Curve.Points.Insert(NewPoint, ExistingIndex + 1);
			}

			// Interpolate points between last valid index and the new point
			for (int FillIndex = LastValidIndex + 1; FillIndex < ExistingIndex; ++FillIndex)
			{
				FInterpCurvePointLinearColor& FillPoint = Curve.Points[FillIndex];
				float FillValue = FloatCurve.Eval(FillPoint.InVal);
				float FillTangent = FloatCurve.EvalDerivative(FillPoint.InVal);
				FillPoint.OutVal.Component(i) = FillValue;
				FillPoint.ArriveTangent.Component(i) = FillTangent;
				FillPoint.LeaveTangent.Component(i) = FillTangent;

				LastValidIndex = FillIndex;
			}
		}
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
				OutExtrema[0] = -b / (2.0f * a);
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
		IntegratedCurve.Points.Add(FInterpCurvePointFloat(Point.InVal, Value, .0f, Point.OutVal, EInterpCurveMode::CIM_Unknown));
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
				Point.OutVal, NextPoint.OutVal, Point.LeaveTangent * TotalDeltaTime, NextPoint.ArriveTangent * TotalDeltaTime, Extrema);
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
		const FInterpCurvePointFloat& Point = Curve.Points.Last();
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

namespace
{
	bool FindCurveRoot(const FInterpCurveFloat& Curve, float OutVal, float InValEstimate, float& InVal, float MaxIter = 100)
	{
		return false;
	}

	bool EstimateCurveRoot(const FInterpCurveFloat& Curve, float OutVal, float& InVal)
	{
		if (Curve.Points.Num() == 0)
		{
			return false;
		}

		float OutValMin, OutValMax;
		Curve.CalcBounds(OutValMin, OutValMax, Curve.Points[0].OutVal);

		const float OutValRange = OutValMax - OutValMin;
		const float Alpha = (FMath::IsNearlyZero(OutValRange) ? .0f : (OutVal - OutValMin) / OutValRange);

		const float InValMin = Curve.Points[0].InVal;
		const float InValMax = Curve.Points.Last().InVal;
		InVal = FMath::Lerp(InValMin, InValMax, Alpha);
		return true;
	}
} // namespace

void UProbabilityCurveFunctionLibrary::InvertCurve(const FInterpCurveFloat& Curve, int32 Resolution, FInterpCurveFloat& InvertedCurve)
{
#if 1
	if (!ensure(Resolution > 0))
	{
		return;
	}

	InvertedCurve.bIsLooped = Curve.bIsLooped;
	InvertedCurve.LoopKeyOffset = Curve.LoopKeyOffset;
	InvertedCurve.Points.Empty();

	// First point
	{
		const FInterpCurvePointFloat& Point = Curve.Points[0];
		InvertedCurve.Points.Add(FInterpCurvePointFloat(Point.OutVal, Point.InVal, .0f, .0f, EInterpCurveMode::CIM_Unknown));
	}

	for (int i = 0; i < Curve.Points.Num() - 1; ++i)
	{
		const FInterpCurvePointFloat& Point = Curve.Points[i];
		const FInterpCurvePointFloat& NextPoint = Curve.Points[i + 1];

		// Check monotonicity
		if (NextPoint.OutVal <= Point.OutVal + KINDA_SMALL_NUMBER)
		{
			continue;
		}

		if (Point.InterpMode == EInterpCurveMode::CIM_Constant)
		{
			FInterpCurvePointFloat& CurrentPoint = InvertedCurve.Points.Last();

			CurrentPoint.InterpMode = EInterpCurveMode::CIM_Constant;
			InvertedCurve.Points.Add(FInterpCurvePointFloat(NextPoint.OutVal, NextPoint.InVal, .0f, .0f, EInterpCurveMode::CIM_Unknown));
			break;
		}
		else if (Point.InterpMode == EInterpCurveMode::CIM_Linear)
		{
			FInterpCurvePointFloat& CurrentPoint = InvertedCurve.Points.Last();

			CurrentPoint.InterpMode = EInterpCurveMode::CIM_Linear;
			InvertedCurve.Points.Add(FInterpCurvePointFloat(NextPoint.OutVal, NextPoint.InVal, .0f, .0f, EInterpCurveMode::CIM_Unknown));
		}
		else if (Point.IsCurveKey())
		{
			const float DeltaTime = (NextPoint.InVal - Point.InVal) / Resolution;
			float NextTime = Point.InVal + DeltaTime;
			for (int a = 0; a < Resolution; ++a)
			{
				FInterpCurvePointFloat& CurrentPoint = InvertedCurve.Points.Last();
				const float NextValue = Curve.Eval(NextTime);

				// Check monotonicity
				if (NextValue > CurrentPoint.InVal + KINDA_SMALL_NUMBER)
				{
					CurrentPoint.InterpMode = EInterpCurveMode::CIM_Linear;
					InvertedCurve.Points.Add(FInterpCurvePointFloat(NextValue, NextTime, .0f, .0f, EInterpCurveMode::CIM_Unknown));
				}

				NextTime += DeltaTime;
			}
		}
		else
		{
			FInterpCurvePointFloat& CurrentPoint = InvertedCurve.Points.Last();

			CurrentPoint.InterpMode = EInterpCurveMode::CIM_Unknown;
			InvertedCurve.Points.Add(FInterpCurvePointFloat(NextPoint.OutVal, NextPoint.InVal, .0f, .0f, EInterpCurveMode::CIM_Unknown));
		}
	}
#else
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
		const bool bRequireJumpPoint = Point.InterpMode == EInterpCurveMode::CIM_Constant ||
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
#endif
}

void UProbabilityCurveFunctionLibrary::DrawDebugCurve(
	const UObject* WorldContextObject, const FInterpCurveFloat& Curve, const FTransform& Transform, const FDrawDebugCurveSettings& Settings,
	bool bPersistentLines, float LifeTime, uint8 DepthPriority)
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

				DrawDebugLine(
					World, ArrivePos, Position, Settings.TangentColor, bPersistentLines, LifeTime, DepthPriority,
					Settings.TangentThickness);
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

				DrawDebugLine(
					World, Position, LeavePos, Settings.TangentColor, bPersistentLines, LifeTime, DepthPriority, Settings.TangentThickness);
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
				DrawDebugLine(
					World, Position, NextPosition, Settings.LineColor, bPersistentLines, LifeTime, DepthPriority, Settings.LineThickness);
			}
			else if (Point.InterpMode == EInterpCurveMode::CIM_Linear)
			{
				FVector Position = Transform.TransformPosition(FVector(Point.InVal, .0f, Point.OutVal));
				FVector NextPosition = Transform.TransformPosition(FVector(NextPoint.InVal, .0f, NextPoint.OutVal));
				DrawDebugLine(
					World, Position, NextPosition, Settings.LineColor, bPersistentLines, LifeTime, DepthPriority, Settings.LineThickness);
			}
			// Any of the curve modes
			else if (Point.IsCurveKey())
			{
				const float dx = (NextPoint.InVal - Point.InVal) / Settings.CurveResolution;

				float x = Point.InVal;
				FVector Position = Transform.TransformPosition(FVector(x, .0f, Curve.Eval(x, .0f)));

				for (int32 a = 0; a < Settings.CurveResolution; ++a)
				{
					// TODO Forward differencing is more efficient for eval at constant intervals,
					// not necessary for debug drawing though.

					x += dx;
					FVector NextPosition = Transform.TransformPosition(FVector(x, .0f, Curve.Eval(x, .0f)));

					DrawDebugLine(
						World, Position, NextPosition, Settings.LineColor, bPersistentLines, LifeTime, DepthPriority,
						Settings.LineThickness);

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

void UProbabilityCurveFunctionLibrary::ConvertFromRichCurve(const FRichCurve& RichCurve, FInterpCurveFloat& Curve)
{
	Curve.bIsLooped = false;
	Curve.LoopKeyOffset = 0.0f;

	const int32 NumKeys = RichCurve.GetNumKeys();
	if (NumKeys == 0)
	{
		Curve.Points.Empty();
		return;
	}

	Curve.Points.SetNum(NumKeys);

	for (auto KeyIter = RichCurve.GetKeyIterator(); KeyIter; ++KeyIter)
	{
		FInterpCurvePointFloat& Point = Curve.Points[KeyIter.GetIndex()];

		Point.InVal = KeyIter->Time;
		Point.OutVal = KeyIter->Value;
		Point.ArriveTangent = KeyIter->ArriveTangent;
		Point.LeaveTangent = KeyIter->LeaveTangent;
		Point.InterpMode = ConvertInterpolationMode(KeyIter->InterpMode, KeyIter->TangentMode);
	}
}

void UProbabilityCurveFunctionLibrary::ConvertToRichCurve(const FInterpCurveFloat& Curve, FRichCurve& RichCurve)
{
	if (Curve.bIsLooped)
	{
		RichCurve.PreInfinityExtrap = ERichCurveExtrapolation::RCCE_Cycle;
		RichCurve.PostInfinityExtrap = ERichCurveExtrapolation::RCCE_Cycle;
	}
	else
	{
		RichCurve.PreInfinityExtrap = ERichCurveExtrapolation::RCCE_None;
		RichCurve.PostInfinityExtrap = ERichCurveExtrapolation::RCCE_None;
	}

	const int32 NumKeys = Curve.Points.Num();
	if (NumKeys == 0)
	{
		RichCurve.Reset();
		return;
	}

	for (const FInterpCurvePointFloat& Point : Curve.Points)
	{
		FKeyHandle KeyHandle = RichCurve.UpdateOrAddKey(Point.InVal, Point.OutVal);
		FRichCurveKey& Key = RichCurve.GetKey(KeyHandle);
		ERichCurveInterpMode InterpMode;
		ERichCurveTangentMode TangentMode;
		ConvertInterpolationMode(Point.InterpMode, InterpMode, TangentMode);
		Key.InterpMode = InterpMode;
		Key.TangentMode = TangentMode;
		Key.TangentWeightMode = ERichCurveTangentWeightMode::RCTWM_WeightedNone;
		Key.ArriveTangent = Point.ArriveTangent;
		Key.LeaveTangent = Point.LeaveTangent;
	}
}

namespace
{
	using RichCurveIterator = TIndexedContainerIterator<const TArray<FRichCurveKey>, FRichCurveKey, int32>;

	void AddRichCurveKey(
		FRichCurve& Curve, float Time, float Value, float ArriveTangent, float LeaveTangent, ERichCurveInterpMode InterpMode = RCIM_None,
		ERichCurveTangentMode TangentMode = RCTM_Auto)
	{
		FKeyHandle Handle = Curve.AddKey(Time, Value);
		FRichCurveKey& Key = Curve.GetKey(Handle);
		Key.InterpMode = InterpMode;
		Key.TangentMode = TangentMode;
		Key.ArriveTangent = ArriveTangent;
		Key.LeaveTangent = LeaveTangent;
	}
} // namespace

void UProbabilityCurveFunctionLibrary::IntegrateRichCurve(
	const FRichCurve& Curve, float Offset, FRichCurve& IntegratedCurve, float& TotalArea)
{
	IntegratedCurve.PreInfinityExtrap = RCCE_Constant;
	IntegratedCurve.PostInfinityExtrap = RCCE_Constant;
	IntegratedCurve.Reset();
	if (Curve.GetNumKeys() == 0)
	{
		return;
	}

	float Value = Offset;
	// First point
	{
		const FRichCurveKey& Point = Curve.Keys[0];
		AddRichCurveKey(IntegratedCurve, Point.Time, Value, .0f, Point.Value);
	}

	for (auto KeyIter(Curve.GetKeyIterator()); KeyIter && KeyIter + 1; ++KeyIter)
	{
		const FRichCurveKey& Point = *KeyIter;
		const FRichCurveKey& NextPoint = *(KeyIter + 1);

		if (Point.InterpMode == RCIM_Constant)
		{
			const float Time0 = Point.Time;
			const float Time1 = NextPoint.Time;
			const float DeltaTime = Time1 - Time0;
			const float Slope0 = Point.Value;
			FRichCurveKey& CurrentPoint = IntegratedCurve.Keys.Last();

			Value += Slope0 * DeltaTime;

			CurrentPoint.LeaveTangent = Slope0;
			CurrentPoint.InterpMode = RCIM_Linear;
			AddRichCurveKey(IntegratedCurve, Time1, Value, Slope0, .0f);
			break;
		}
		else if (Point.InterpMode == RCIM_Linear)
		{
			const float Time0 = Point.Time;
			const float Time1 = NextPoint.Time;
			const float DeltaTime = Time1 - Time0;
			const float Slope0 = Point.Value;
			const float Slope1 = NextPoint.Value;
			FRichCurveKey& CurrentPoint = IntegratedCurve.Keys.Last();

			Value += (Slope0 + Slope1) * DeltaTime * 0.5f;

			CurrentPoint.LeaveTangent = Slope0;
			CurrentPoint.InterpMode = RCIM_Cubic;
			CurrentPoint.TangentMode = RCTM_Break;
			AddRichCurveKey(IntegratedCurve, Time1, Value, Slope1, .0f);
		}
		else if (Point.InterpMode == RCIM_Cubic)
		{
			// Cannot go above cubic splines, so use linear interpolation between extrema to minimize errors.

			const float TotalDeltaTime = NextPoint.Time - Point.Time;
			float CurrentTime = Point.Time;
			float CurrentSlope = Point.Value;

			float Extrema[2];
			int NumExtrema = FindExtrema(
				Point.Value, NextPoint.Value, Point.LeaveTangent * TotalDeltaTime, NextPoint.ArriveTangent * TotalDeltaTime, Extrema);
			for (int a = 0; a < NumExtrema; ++a)
			{
				if (Extrema[a] <= .0f || Extrema[a] >= 1.0f)
				{
					continue;
				}

				const float NextTime = CurrentTime + Extrema[a] * TotalDeltaTime;
				const float NextSlope = Curve.Eval(NextTime);
				FRichCurveKey& CurrentPoint = IntegratedCurve.Keys.Last();

				Value += (CurrentSlope + NextSlope) * (NextTime - CurrentTime) * 0.5f;

				CurrentPoint.LeaveTangent = CurrentSlope;
				CurrentPoint.InterpMode = RCIM_Cubic;
				CurrentPoint.TangentMode = RCTM_Break;
				AddRichCurveKey(IntegratedCurve, NextTime, Value, NextSlope, .0f);

				CurrentTime = NextTime;
				CurrentSlope = NextSlope;
			}

			// Last point of segment
			const float NextTime = NextPoint.Time;
			const float NextSlope = NextPoint.Value;
			FRichCurveKey& CurrentPoint = IntegratedCurve.Keys.Last();

			Value += (CurrentSlope + NextSlope) * (NextTime - CurrentTime) * 0.5f;

			CurrentPoint.LeaveTangent = CurrentSlope;
			CurrentPoint.InterpMode = RCIM_Cubic;
			CurrentPoint.TangentMode = RCTM_Break;
			AddRichCurveKey(IntegratedCurve, NextTime, Value, NextSlope, .0f);
		}
		else
		{
			const float Time1 = NextPoint.Time;
			FRichCurveKey& CurrentPoint = IntegratedCurve.Keys.Last();

			// Unknown
			CurrentPoint.LeaveTangent = .0f;
			CurrentPoint.InterpMode = RCIM_None;
			AddRichCurveKey(IntegratedCurve, Time1, Value, .0f, .0f);
		}
	}

	// Last point
	{
		const FRichCurveKey& Point = Curve.Keys.Last();
		FRichCurveKey& CurrentPoint = IntegratedCurve.Keys.Last();

		CurrentPoint.LeaveTangent = Point.Value;
		CurrentPoint.InterpMode = RCIM_Cubic;
		CurrentPoint.TangentMode = RCTM_Break;
	}

	TotalArea = Value;

	IntegratedCurve.AutoSetTangents();
}

void UProbabilityCurveFunctionLibrary::TransformRichCurve(const FRichCurve& Curve, float Scale, float Offset, FRichCurve& ScaledCurve)
{
	ScaledCurve.PreInfinityExtrap = Curve.PreInfinityExtrap;
	ScaledCurve.PostInfinityExtrap = Curve.PostInfinityExtrap;
	ScaledCurve.Reset();

	if (Curve.GetNumKeys() == 0)
	{
		return;
	}

	for (auto KeyIter(Curve.GetKeyIterator()); KeyIter; ++KeyIter)
	{
		const FRichCurveKey& Point = *KeyIter;

		AddRichCurveKey(
			ScaledCurve, Point.Time, Point.Value * Scale + Offset, Point.ArriveTangent * Scale, Point.LeaveTangent * Scale,
			Point.InterpMode, Point.TangentMode);
	}

	ScaledCurve.AutoSetTangents();
}

void UProbabilityCurveFunctionLibrary::NormalizeRichCurve(const FRichCurve& Curve, FRichCurve& NormalizedCurve)
{
	float MinValue, MaxValue;
	Curve.GetValueRange(MinValue, MaxValue);
	const float Range = MaxValue - MinValue;
	if (!FMath::IsNearlyZero(Range))
	{
		TransformRichCurve(Curve, 1.0f / Range, -MinValue / Range, NormalizedCurve);
	}
}

void UProbabilityCurveFunctionLibrary::InvertRichCurve(const FRichCurve& Curve, int32 Resolution, FRichCurve& InvertedCurve)
{
#if 1
	if (!ensure(Resolution > 0))
	{
		return;
	}

	InvertedCurve.PreInfinityExtrap = Curve.PreInfinityExtrap;
	InvertedCurve.PostInfinityExtrap = Curve.PostInfinityExtrap;
	InvertedCurve.Reset();

	// First point
	{
		const FRichCurveKey& Point = Curve.Keys[0];
		AddRichCurveKey(InvertedCurve, Point.Value, Point.Time, .0f, .0f);
	}

	for (auto KeyIter(Curve.GetKeyIterator()); KeyIter && KeyIter + 1; ++KeyIter)
	{
		const FRichCurveKey& Point = *KeyIter;
		const FRichCurveKey& NextPoint = *(KeyIter + 1);

		// Check monotonicity
		if (NextPoint.Value <= Point.Value + KINDA_SMALL_NUMBER)
		{
			continue;
		}

		if (Point.InterpMode == RCIM_Constant)
		{
			FRichCurveKey& CurrentPoint = InvertedCurve.Keys.Last();

			CurrentPoint.InterpMode = RCIM_Constant;
			AddRichCurveKey(InvertedCurve, NextPoint.Value, NextPoint.Time, .0f, .0f);
			break;
		}
		else if (Point.InterpMode == RCIM_Linear)
		{
			FRichCurveKey& CurrentPoint = InvertedCurve.Keys.Last();

			CurrentPoint.InterpMode = RCIM_Linear;
			AddRichCurveKey(InvertedCurve, NextPoint.Value, NextPoint.Time, .0f, .0f);
		}
		else if (Point.InterpMode == RCIM_Cubic)
		{
			const float DeltaTime = (NextPoint.Time - Point.Time) / Resolution;
			float NextTime = Point.Time + DeltaTime;
			for (int a = 0; a < Resolution; ++a)
			{
				FRichCurveKey& CurrentPoint = InvertedCurve.Keys.Last();
				const float NextValue = Curve.Eval(NextTime);

				// Check monotonicity
				if (NextValue > CurrentPoint.Time + KINDA_SMALL_NUMBER)
				{
					CurrentPoint.InterpMode = RCIM_Linear;
					AddRichCurveKey(InvertedCurve, NextValue, NextTime, .0f, .0f);
				}

				NextTime += DeltaTime;
			}
		}
		else
		{
			FRichCurveKey& CurrentPoint = InvertedCurve.Keys.Last();

			CurrentPoint.InterpMode = RCIM_None;
			AddRichCurveKey(InvertedCurve, NextPoint.Value, NextPoint.Time, .0f, .0f);
		}
	}

	InvertedCurve.AutoSetTangents();
#else
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
		const bool bRequireJumpPoint = Point.InterpMode == EInterpCurveMode::CIM_Constant ||
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
#endif
}

void UProbabilityCurveFunctionLibrary::ComputeQuantileRichCurve(
	const FRichCurve& DensityCurve, FRichCurve& NormalizedDensityCurve, FRichCurve& QuantileCurve)
{
	FRichCurve IntegratedDensityCurve;
	float IntegratedDensity;
	UProbabilityCurveFunctionLibrary::IntegrateRichCurve(DensityCurve, 0.0f, IntegratedDensityCurve, IntegratedDensity);

	if (!FMath::IsNearlyZero(IntegratedDensity))
	{
		UProbabilityCurveFunctionLibrary::TransformRichCurve(DensityCurve, 1.0f / IntegratedDensity, 0.0f, NormalizedDensityCurve);
	}

	FRichCurve CumulativeDensityCurve;
	UProbabilityCurveFunctionLibrary::NormalizeRichCurve(IntegratedDensityCurve, CumulativeDensityCurve);
	UProbabilityCurveFunctionLibrary::InvertRichCurve(CumulativeDensityCurve, 10, QuantileCurve);
}
