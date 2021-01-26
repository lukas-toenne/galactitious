// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "FastMultipoleSimulation.h"
#include "NiagaraDataInterface.h"

#include "NiagaraDataInterfaceGalaxySimulation.generated.h"

/** Data stored per simulation interface instance*/
struct FNDIGalaxySimulation_InstanceData
{
	/** Initialize the buffers */
	bool Init(UNiagaraDataInterfaceGalaxySimulation* Interface, FNiagaraSystemInstance* SystemInstance);

	void Release();

	/** Cached ptr to the mesh so that we can make sure that we haven't been deleted. */
	TWeakObjectPtr<UFastMultipoleSimulation> Simulation;
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
UCLASS(EditInlineNew, Category = "Physics", meta = (DisplayName = "Multipole Grid"))
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
	virtual bool HasPreSimulateTick() const override { return true; }
	virtual bool InitPerInstanceData(void* PerInstanceData, FNiagaraSystemInstance* SystemInstance) override;
	virtual void DestroyPerInstanceData(void* PerInstanceData, FNiagaraSystemInstance* SystemInstance) override;
	virtual bool PerInstanceTick(void* PerInstanceData, FNiagaraSystemInstance* SystemInstance, float DeltaSeconds) override;
	virtual int32 PerInstanceDataSize() const override { return sizeof(FNDIGalaxySimulation_InstanceData); }
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

	/** Get current position of a point */
	void GetPointPosition(FVectorVMContext& Context);

public:
	/** Simulation targeted by the data interface */
	UPROPERTY(EditAnywhere)
	class UGalaxySimulationAsset* SimulationAsset;

protected:
	/** Copy one niagara DI to this */
	virtual bool CopyToInternal(UNiagaraDataInterface* Destination) const override;
};

/** Proxy to send data to gpu */
struct FNDIGalaxySimulationProxy : public FNiagaraDataInterfaceProxy
{
	/** Get the size of the data that will be passed to render*/
	virtual int32 PerInstanceDataPassedToRenderThreadSize() const override { return sizeof(FNDIGalaxySimulation_InstanceData); }

	/** Get the data that will be passed to render*/
	virtual void ConsumePerInstanceDataFromGameThread(void* PerInstanceData, const FNiagaraSystemInstanceID& Instance) override;

	/** Launch all pre stage functions */
	virtual void PreStage(FRHICommandList& RHICmdList, const FNiagaraDataInterfaceStageArgs& Context) override;

	/** Launch all post stage functions */
	virtual void PostStage(FRHICommandList& RHICmdList, const FNiagaraDataInterfaceStageArgs& Context) override;

	/** Reset the buffers  */
	virtual void ResetData(FRHICommandList& RHICmdList, const FNiagaraDataInterfaceArgs& Context) override;

	/** List of proxy data for each system instances*/
	TMap<FNiagaraSystemInstanceID, FNDIGalaxySimulation_InstanceData> SystemInstancesToProxyData;
};
