// Fill out your copyright notice in the Description page of Project Settings.

#include "RemoteSimulationRenderBuffers.h"

#include "RemoteSimulationTypes.h"

void FRemoteSimulationIndexBuffer::Resize(const uint32& RequestedCapacity)
{
	// This must be called from Rendering thread
	check(IsInRenderingThread());

	if (Capacity != RequestedCapacity)
	{
		ReleaseResource();
		Capacity = RequestedCapacity;
		InitResource();
	}
}

void FRemoteSimulationIndexBuffer::InitRHI()
{
	FRHIResourceCreateInfo CreateInfo;
	void* Buffer = nullptr;
	uint32 Size = Capacity * 7 * sizeof(uint32);
	PointOffset = Capacity * 6;

	IndexBufferRHI = RHICreateAndLockIndexBuffer(sizeof(uint32), Size, BUF_Dynamic, CreateInfo, Buffer);

	uint32* Data = (uint32*)Buffer;
	for (uint32 i = 0; i < Capacity; i++)
	{
		// Full quads
		{
			uint32 idx = i * 6;
			uint32 v = i * 4;

			Data[idx] = v;
			Data[idx + 1] = v + 1;
			Data[idx + 2] = v + 2;
			Data[idx + 3] = v;
			Data[idx + 4] = v + 2;
			Data[idx + 5] = v + 3;
		}

		// Points
		Data[PointOffset + i] = i;
	}

	RHIUnlockIndexBuffer(IndexBufferRHI);
	Buffer = nullptr;
}

//////////////////////////////////////////////////////////// Structured Buffer

void FRemoteSimulationRenderBuffer::Resize(const uint32& RequestedCapacity)
{
	// This must be called from Rendering thread
	check(IsInRenderingThread());

	if (Capacity != RequestedCapacity)
	{
		ReleaseResource();
		Capacity = RequestedCapacity;
		InitResource();
	}
	else if (!IsInitialized())
	{
		InitResource();
	}
}

void FRemoteSimulationRenderBuffer::InitRHI()
{
	// This must be called from Rendering thread
	check(IsInRenderingThread());

	FRHIResourceCreateInfo CreateInfo;
	Buffer = RHICreateVertexBuffer(Capacity * sizeof(FRemoteSimulationPoint), BUF_ShaderResource | BUF_Dynamic, CreateInfo);
	SRV = RHICreateShaderResourceView(Buffer, sizeof(uint32), PF_R32_FLOAT);
}

void FRemoteSimulationRenderBuffer::ReleaseRHI()
{
	// This must be called from Rendering thread
	check(IsInRenderingThread());

	if (Buffer)
	{
		RHIDiscardTransientResource(Buffer);
		Buffer.SafeRelease();
	}

	SRV.SafeRelease();
}
