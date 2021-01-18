// Fill out your copyright notice in the Description page of Project Settings.

#include "FastMultipoleTextureFunctionLibrary.h"

#include "FastMultipoleSimulation.h"
#include "OpenVDBConvert.h"

#include "Async/Async.h"

#include "FastMultipoleOpenVDBGuardEnter.h"
#include <openvdb/points/PointConversion.h>
#include "FastMultipoleOpenVDBGuardLeave.h"

namespace
{
	template <class SamplerType>
	void BakeSliceTextureData(
		UFastMultipoleTextureFunctionLibrary::FAsyncTextureData* TextureData, const FTransform& MeshToWorld, SamplerType Sampler)
	{
		const int32 BytesPerPixel = FTextureSource::GetBytesPerPixel(TextureData->SourceFormat);
		check(BytesPerPixel == sizeof(FColor));

		const int32 Width = TextureData->Width;
		const int32 Height = TextureData->Height;
		uint8* Data = TextureData->Bytes.GetData();
		const float dX = 1.0f / FMath::Max(Width - 1, 1);
		const float dY = 1.0f / FMath::Max(Height - 1, 1);

		float Y = 0.0f;
		for (int32 j = 0; j < Height; j++)
		{
			float X = 0.0f;
			for (int32 i = 0; i < Width; i++)
			{
				if (TextureData->bStopRequested)
				{
					return;
				}

				FVector LocalPosition = FVector(X, Y, 0.5f);
				openvdb::Vec3f vdbPosition = OpenVDBConvert::Vector(MeshToWorld.TransformPosition(LocalPosition));
				float Mass = Sampler.wsSample(vdbPosition);
				*(FColor*)Data = FColor::MakeRedToGreenColorFromScalar(FMath::Clamp(Mass, 0.0f, 1.0f));

				X += dX;
				Data += BytesPerPixel;
			}
			Y += dY;
		}

		TextureData->bCompleted = true;
	}
} // namespace

void UFastMultipoleTextureFunctionLibrary::BakeSliceTextureAsync(
	UFastMultipoleTextureFunctionLibrary::FAsyncTextureDataPtr TextureData, const FTransform& TextureToWorld,
	const UFastMultipoleSimulation* Simulation)
{
	if (TextureData->bStopRequested)
	{
		return;
	}

	Async(EAsyncExecution::ThreadPool, [TextureToWorld, Simulation, TextureData]() {
		if (TextureData->bStopRequested)
		{
			return;
		}

		using GridType = UFastMultipoleSimulation::PointGridType;
		using SamplerType = openvdb::tools::GridSampler<GridType, openvdb::tools::PointSampler>;
		const int32 BytesPerPixel = FTextureSource::GetBytesPerPixel(TextureData->SourceFormat);

		TextureData->Bytes.AddUninitialized(TextureData->Width * TextureData->Height * BytesPerPixel);

		BakeSliceTextureData(TextureData.Get(), TextureToWorld, SamplerType(*Simulation->GetPointGrid()));
	});
}
