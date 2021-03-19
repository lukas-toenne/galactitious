// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "RenderResource.h"

/**
 * This class creates an IndexBuffer shared between all assets and all instances.
 */
class REMOTESIMULATIONRENDERING_API FRemoteSimulationIndexBuffer : public FIndexBuffer
{
public:
	FRemoteSimulationIndexBuffer() : Capacity(100000) {}

	void Resize(const uint32& RequestedCapacity);

	virtual void InitRHI() override;

	uint32 Capacity;
};

struct REMOTESIMULATIONRENDERING_API FRemoteSimulationPointData
{
	FVector Location;
};

/**
 * Encapsulates a GPU read buffer with its SRV.
 */
class REMOTESIMULATIONRENDERING_API FRemoteSimulationPointDataBuffer : public FRenderResource
{
public:
	FRemoteSimulationPointDataBuffer() : Capacity(100000) {}

	void Resize(const uint32& RequestedCapacity);

	virtual void InitRHI() override;
	virtual void ReleaseRHI() override;

	FVertexBufferRHIRef Buffer;
	FShaderResourceViewRHIRef SRV;

	uint32 Capacity;
	int32 PointCount;
};
