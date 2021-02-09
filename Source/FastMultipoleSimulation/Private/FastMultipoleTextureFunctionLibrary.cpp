// Fill out your copyright notice in the Description page of Project Settings.

#include "FastMultipoleTextureFunctionLibrary.h"

#include "FastMultipoleSimulationCache.h"
#include "OpenVDBConvert.h"

#include "Async/Async.h"

#include "FastMultipoleOpenVDBGuardEnter.h"
#include <openvdb/points/PointConversion.h>
#include "FastMultipoleOpenVDBGuardLeave.h"

namespace
{
	template <class SamplerType>
	void BakeMassTextureData(
		UFastMultipoleTextureFunctionLibrary::FAsyncTextureData* TextureData, const FTransform& TextureToWorld,
		const FastMultipole::PointDataGridType& Grid)
	{
		using SamplerType = openvdb::tools::GridSampler<GridType, openvdb::tools::PointSampler>;

		if (TextureData->bStopRequested)
		{
			return;
		}

		const int32 BytesPerPixel = FTextureSource::GetBytesPerPixel(TextureData->SourceFormat);
		check(BytesPerPixel == sizeof(FColor));
		const int32 Width = TextureData->Width;
		const int32 Height = TextureData->Height;
		const float InvWidth = 1.0f / FMath::Max(Width - 1, 1);
		const float InvHeight = 1.0f / FMath::Max(Height - 1, 1);

		TextureData->Bytes.AddUninitialized(Width * Height * BytesPerPixel);
		uint8* Data = TextureData->Bytes.GetData();
		SamplerType Sampler(Grid);

		for (int32 j = 0; j < Height; j++)
		{
			for (int32 i = 0; i < Width; i++)
			{
				if (TextureData->bStopRequested)
				{
					return;
				}

				const float X = (float)i * InvWidth;
				const float Y = (float)j * InvHeight;
				FVector LocalPosition = FVector(X, Y, 0.5f);
				openvdb::Vec3f vdbPosition = OpenVDBConvert::Vector(TextureToWorld.TransformPosition(LocalPosition));
				float Mass = Sampler.wsSample(vdbPosition);
				*(FColor*)Data = FColor::MakeRedToGreenColorFromScalar(FMath::Clamp(Mass, 0.0f, 1.0f));

				Data += BytesPerPixel;
			}
		}

		TextureData->bCompleted = true;
	}

	void BakePointsTextureData(
		UFastMultipoleTextureFunctionLibrary::FAsyncTextureData* TextureData, const FTransform& TextureToWorld,
		const FastMultipole::PointDataGridType& Grid)
	{
		if (TextureData->bStopRequested)
		{
			return;
		}

		const int32 BytesPerPixel = FTextureSource::GetBytesPerPixel(TextureData->SourceFormat);
		check(BytesPerPixel == sizeof(FColor));
		const int32 Width = TextureData->Width;
		const int32 Height = TextureData->Height;
		const float InvWidth = 1.0f / FMath::Max(Width - 1, 1);
		const float InvHeight = 1.0f / FMath::Max(Height - 1, 1);

		TextureData->Bytes.AddZeroed(Width * Height * BytesPerPixel);
		uint8* Data = TextureData->Bytes.GetData();

		for (auto LeafIter = Grid.tree().cbeginLeaf(); LeafIter; ++LeafIter)
		{
			// Verify the leaf origin.
			std::cout << "Leaf" << LeafIter->origin() << std::endl;
			// Extract the position attribute from the leaf by name (P is position).
			const openvdb::points::AttributeArray& PositionArray = LeafIter->constAttributeArray("P");
			// Create a read-only AttributeHandle. Position always uses Vec3f.
			openvdb::points::AttributeHandle<openvdb::Vec3f> PositionHandle(PositionArray);
			// Iterate over the point indices in the leaf.
			for (auto IndexIter = LeafIter->beginIndexOn(); IndexIter; ++IndexIter)
			{
				// Extract the voxel-space position of the point.
				openvdb::Vec3f VoxelPosition = PositionHandle.get(*IndexIter);
				// Extract the world-space position of the voxel.
				const openvdb::Vec3d xyz = IndexIter.getCoord().asVec3d();
				// Compute the world-space position of the point.
				openvdb::Vec3f WorldPosition = Grid.transform().indexToWorld(VoxelPosition + xyz);
				// Verify the index and world-space position of the point
				std::cout << "* PointIndex=[" << *IndexIter << "] ";
				std::cout << "WorldPosition=" << WorldPosition << std::endl;

				const FVector TexPosition = TextureToWorld.InverseTransformPosition(OpenVDBConvert::Vector(WorldPosition));
				const int32 i = (int32)(TexPosition.X * Width);
				const int32 j = (int32)(TexPosition.Y * Height);
				if (i >= 0 && i < Width && j >= 0 && j < Height)
				{
					*(FColor*)(Data + BytesPerPixel * (i + j * Width)) = FColor::Green;
				}
			}
		}

		TextureData->bCompleted = true;
	}
} // namespace

void UFastMultipoleTextureFunctionLibrary::BakeSliceTextureAsync(
	UFastMultipoleTextureFunctionLibrary::FAsyncTextureDataPtr TextureData, const FTransform& TextureToWorld,
	const UFastMultipoleSimulationCache* SimulationCache)
{
	using GridType = FastMultipole::PointDataGridType;

	if (TextureData->bStopRequested)
	{
		return;
	}

	if (FFastMultipoleSimulationFrame::ConstPtr Frame = SimulationCache->GetLastFrame())
	{
		GridType::Ptr Grid = Frame->GetPointDataGrid();
		if (!Grid)
		{
			return;
		}

		Async(EAsyncExecution::ThreadPool, [TextureData, TextureToWorld, Grid]() {
			BakePointsTextureData(TextureData.Get(), TextureToWorld, *Grid);
		});
	}
}
