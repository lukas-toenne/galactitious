// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "FastMultipoleCachePlayer.h"
#include "FastMultipoleSimulationFrame.h"
#include "FastMultipoleTypes.h"
#include "NiagaraDataInterface.h"

#include "Containers/Queue.h"

#include "NiagaraDataInterfaceGalaxySimulation.generated.h"

class UFastMultipoleSimulationCache;

struct FGalaxySimulationParticleExportData
{
	TArray<int32> UniqueIDs;
	TArray<float> Masses;
	TArray<FVector> Positions;
	TArray<FVector> Velocities;
};

/** Data stored per simulation interface instance*/
struct FNDIGalaxySimulationInstanceData_GameThread
{
public:
	/** Initialize the buffers */
	bool Init(UNiagaraDataInterfaceGalaxySimulation* Interface, FNiagaraSystemInstance* SystemInstance);

	void Release();

	bool HasExportedParticles() const;
	void AddExportedParticles(FGalaxySimulationParticleExportData& ExportData);
	void GatherExportedParticles(
		const FFastMultipoleSimulationInvariants::Ptr& OutInvariants, const FFastMultipoleSimulationFrame::Ptr& OutFrame);
	void DiscardExportedParticles();

	bool StartSimulation(UWorld* DebugWorld = nullptr);
	void StopSimulation();

	/** Schedule steps for simulation if the cache player reaches the end. */
	void SchedulePrecomputeSteps(int32 NumStepsPrecompute);

public:
	/** Cached ptr to the mesh so that we can make sure that we haven't been deleted. */
	TWeakObjectPtr<UFastMultipoleSimulationCache> SimulationCache;

	FFastMultipoleCachePlayer CachePlayer;

	FFastMultipoleSimulationSettings SimulationSettings;

private:
	TQueue<FGalaxySimulationParticleExportData, EQueueMode::Mpsc> ExportedParticles;
};

struct FNDIGalaxySimulationInstanceData_RenderThread
{
};

/** Data Interface parameters name */
struct FNDIGalaxySimulationParametersName
{
	FNDIGalaxySimulationParametersName(const FString& Suffix);

	// FString GridCurrentBufferName;
	// FString GridDestinationBufferName;

	// FString GridSizeName;
	// FString WorldTransformName;
	// FString WorldInverseName;
};

struct FNDIGalaxySimulationParametersCS : public FNiagaraDataInterfaceParametersCS
{
	DECLARE_TYPE_LAYOUT(FNDIGalaxySimulationParametersCS, NonVirtual);

	void Bind(const FNiagaraDataInterfaceGPUParamInfo& ParameterInfo, const class FShaderParameterMap& ParameterMap);

	void Set(FRHICommandList& RHICmdList, const FNiagaraDataInterfaceSetArgs& Context) const;

	void Unset(FRHICommandList& RHICmdList, const FNiagaraDataInterfaceSetArgs& Context) const;

private:
	// LAYOUT_FIELD(FShaderResourceParameter, GridCurrentBuffer);
	// LAYOUT_FIELD(FShaderResourceParameter, GridDestinationBuffer);

	// LAYOUT_FIELD(FShaderParameter, GridSize);
	// LAYOUT_FIELD(FShaderParameter, WorldTransform);
	// LAYOUT_FIELD(FShaderParameter, WorldInverse);
};

/** Data Interface for accessing a point interaction simulation. */
UCLASS(EditInlineNew, Category = "Physics", meta = (DisplayName = "Galaxy Simulation"))
class GALACTITIOUSNIAGARA_API UNiagaraDataInterfaceGalaxySimulation : public UNiagaraDataInterface
{
	GENERATED_UCLASS_BODY()

public:
	DECLARE_NIAGARA_DI_PARAMETER();
	/** UObject Interface */
	virtual void PostInitProperties() override;

	/** UNiagaraDataInterface Interface */
	virtual void GetFunctions(TArray<FNiagaraFunctionSignature>& OutFunctions) override;
	virtual void GetVMExternalFunction(
		const FVMExternalFunctionBindingInfo& BindingInfo, void* InstanceData, FVMExternalFunction& OutFunc) override;
	virtual bool CanExecuteOnTarget(ENiagaraSimTarget Target) const override { return Target == ENiagaraSimTarget::GPUComputeSim; }
	virtual bool InitPerInstanceData(void* PerInstanceData, FNiagaraSystemInstance* SystemInstance) override;
	virtual void DestroyPerInstanceData(void* PerInstanceData, FNiagaraSystemInstance* SystemInstance) override;
	virtual bool HasPreSimulateTick() const override { return true; }
	virtual bool HasPostSimulateTick() const { return true; }
	virtual bool PerInstanceTick(void* PerInstanceData, FNiagaraSystemInstance* SystemInstance, float DeltaSeconds) override;
	virtual bool PerInstanceTickPostSimulate(void* PerInstanceData, FNiagaraSystemInstance* SystemInstance, float DeltaSeconds) override;
	virtual int32 PerInstanceDataSize() const override { return sizeof(FNDIGalaxySimulationInstanceData_GameThread); }
	virtual bool Equals(const UNiagaraDataInterface* Other) const override;

	/** GPU simulation functionality */
	virtual void GetParameterDefinitionHLSL(const FNiagaraDataInterfaceGPUParamInfo& ParamInfo, FString& OutHLSL) override;
	virtual bool GetFunctionHLSL(
		const FNiagaraDataInterfaceGPUParamInfo& ParamInfo, const FNiagaraDataInterfaceGeneratedFunction& FunctionInfo,
		int FunctionInstanceIndex, FString& OutHLSL) override;
	virtual void ProvidePerInstanceDataForRenderThread(
		void* DataForRenderThread, void* PerInstanceData, const FNiagaraSystemInstanceID& SystemInstance) override;
	virtual void GetCommonHLSL(FString& OutHLSL) override;

	/** Get total number of simulated points */
	void GetNumPoints(FVectorVMContext& Context);

	/** Get current state of a point */
	void GetPointState(FVectorVMContext& Context);

	/** Remove all cache frames. */
	void ResetCache(FVectorVMContext& Context);

	/** Add a new point to the simulation cache. */
	void AddPoint(FVectorVMContext& Context);

public:
	/** Simulation targeted by the data interface. */
	UPROPERTY(EditAnywhere)
	class UGalaxySimulationAsset* SimulationAsset;

	/** Use Niagara particle spawning to initialize the simulation cache. */
	UPROPERTY(EditAnywhere)
	bool bInitSimulationCache = true;

	/** Number of simulation steps to compute in advance of the cache player. */
	UPROPERTY(EditAnywhere)
	int32 NumStepsPrecompute = 3;

	UPROPERTY(EditAnywhere, AdvancedDisplay)
	bool EnableDebugDrawing = false;

protected:
	/** Copy one niagara DI to this */
	virtual bool CopyToInternal(UNiagaraDataInterface* Destination) const override;
};

/** Proxy to send data to gpu */
struct FNDIGalaxySimulationProxy : public FNiagaraDataInterfaceProxy
{
	/** Get the size of the data that will be passed to render*/
	virtual int32 PerInstanceDataPassedToRenderThreadSize() const override { return sizeof(FNDIGalaxySimulationInstanceData_GameThread); }

	/** Get the data that will be passed to render*/
	virtual void ConsumePerInstanceDataFromGameThread(void* PerInstanceData, const FNiagaraSystemInstanceID& Instance) override;

	/** Launch all pre stage functions */
	virtual void PreStage(FRHICommandList& RHICmdList, const FNiagaraDataInterfaceStageArgs& Context) override;

	/** Launch all post stage functions */
	virtual void PostStage(FRHICommandList& RHICmdList, const FNiagaraDataInterfaceStageArgs& Context) override;

	/** Reset the buffers  */
	virtual void ResetData(FRHICommandList& RHICmdList, const FNiagaraDataInterfaceArgs& Context) override;

	/** List of proxy data for each system instances*/
	TMap<FNiagaraSystemInstanceID, FNDIGalaxySimulationInstanceData_RenderThread> SystemInstancesToProxyData;
};
