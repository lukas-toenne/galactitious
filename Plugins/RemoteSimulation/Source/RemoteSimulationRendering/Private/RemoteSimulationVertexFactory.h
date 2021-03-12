// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "MeshMaterialShader.h"
#include "RemoteSimulationRenderBuffers.h"
#include "VertexFactory.h"

#include "HAL/ThreadSafeBool.h"

/**
 * Holds all data to be passed to the FLidarPointCloudVertexFactoryShaderParameters as UserData
 */
struct FRemoteSimulationBatchElementUserData
{
	FRHIShaderResourceView* DataBuffer;
	int32 bEditorView;
	float SpriteSize;
	FVector ViewRightVector;
	FVector ViewUpVector;
	int32 bUseCameraFacing;
	// FVector SelectionColor;
	// int32 IndexDivisor;
	// FVector LocationOffset;
	// float VirtualDepth;
	// int32 bUseLODColoration;
	// FVector BoundsSize;
	// FVector ElevationColorBottom;
	// FVector ElevationColorTop;
	// int32 bUseCircle;
	// int32 bUseColorOverride;
	// int32 bUseElevationColor;
	// FVector4 Offset;
	// FVector4 Contrast;
	// FVector4 Saturation;
	// FVector4 Gamma;
	// FVector Tint;
	// float IntensityInfluence;
	// int32 bUseClassification;
	// FVector4 ClassificationColors[32];
	// FMatrix ClippingVolume[16];
	// uint32 NumClippingVolumes;
	// uint32 bStartClipped;

	FRemoteSimulationBatchElementUserData();
};

/**
 * Binds shader parameters necessary for rendering
 */
class FRemoteSimulationVertexFactoryShaderParameters : public FVertexFactoryShaderParameters
{
	DECLARE_INLINE_TYPE_LAYOUT(FRemoteSimulationVertexFactoryShaderParameters, NonVirtual);

public:
	void Bind(const FShaderParameterMap& ParameterMap);
	void GetElementShaderBindings(
		const class FSceneInterface* Scene, const FSceneView* View, const FMeshMaterialShader* Shader,
		const EVertexInputStreamType InputStreamType, ERHIFeatureLevel::Type FeatureLevel, const FVertexFactory* VertexFactory,
		const FMeshBatchElement& BatchElement, class FMeshDrawSingleShaderBindings& ShaderBindings,
		FVertexInputStreamArray& VertexStreams) const;

	LAYOUT_FIELD(FShaderResourceParameter, DataBuffer);
	LAYOUT_FIELD(FShaderParameter, bEditorView);
	LAYOUT_FIELD(FShaderParameter, SpriteSize);
	LAYOUT_FIELD(FShaderParameter, ViewRightVector);
	LAYOUT_FIELD(FShaderParameter, ViewUpVector);
	LAYOUT_FIELD(FShaderParameter, bUseCameraFacing);
	// LAYOUT_FIELD(FShaderParameter, SelectionColor);
	// LAYOUT_FIELD(FShaderParameter, IndexDivisor);
	// LAYOUT_FIELD(FShaderParameter, LocationOffset);
	// LAYOUT_FIELD(FShaderParameter, VirtualDepth);
	// LAYOUT_FIELD(FShaderParameter, bUseLODColoration);
	// LAYOUT_FIELD(FShaderParameter, BoundsSize);
	// LAYOUT_FIELD(FShaderParameter, ElevationColorBottom);
	// LAYOUT_FIELD(FShaderParameter, ElevationColorTop);
	// LAYOUT_FIELD(FShaderParameter, bUseCircle);
	// LAYOUT_FIELD(FShaderParameter, bUseColorOverride);
	// LAYOUT_FIELD(FShaderParameter, bUseElevationColor);
	// LAYOUT_FIELD(FShaderParameter, Offset);
	// LAYOUT_FIELD(FShaderParameter, Contrast);
	// LAYOUT_FIELD(FShaderParameter, Saturation);
	// LAYOUT_FIELD(FShaderParameter, Gamma);
	// LAYOUT_FIELD(FShaderParameter, Tint);
	// LAYOUT_FIELD(FShaderParameter, IntensityInfluence);
	// LAYOUT_FIELD(FShaderParameter, bUseClassification);
	// LAYOUT_FIELD(FShaderParameter, ClassificationColors);
	// LAYOUT_FIELD(FShaderParameter, ClippingVolume);
	// LAYOUT_FIELD(FShaderParameter, NumClippingVolumes);
	// LAYOUT_FIELD(FShaderParameter, bStartClipped);
};

/**
 * Implementation of the custom Vertex Factory, containing only a ZeroStride position stream.
 */
class FRemoteSimulationVertexFactory : public FVertexFactory
{
	DECLARE_VERTEX_FACTORY_TYPE(FRemoteSimulationVertexFactory);

public:
	static bool ShouldCompilePermutation(const FVertexFactoryShaderPermutationParameters& Parameters);

	FRemoteSimulationVertexFactory() : FVertexFactory(ERHIFeatureLevel::SM5) {}

private:
	/** Very simple implementation of a ZeroStride Vertex Buffer */
	class FPointCloudVertexBuffer : public FVertexBuffer
	{
	public:
		virtual void InitRHI() override
		{
			FRHIResourceCreateInfo CreateInfo;
			void* Buffer = nullptr;
			VertexBufferRHI = RHICreateAndLockVertexBuffer(sizeof(FVector), BUF_Static | BUF_ZeroStride, CreateInfo, Buffer);
			FMemory::Memzero(Buffer, sizeof(FVector));
			RHIUnlockVertexBuffer(VertexBufferRHI);
			Buffer = nullptr;
		}

		virtual FString GetFriendlyName() const override { return TEXT("FPointCloudVertexBuffer"); }
	} VertexBuffer;

	virtual void InitRHI() override;
	virtual void ReleaseRHI() override;
};

/** Global vertex factory shared between all proxies */
extern TGlobalResource<FRemoteSimulationVertexFactory> GRemoteSimulationVertexFactory;
