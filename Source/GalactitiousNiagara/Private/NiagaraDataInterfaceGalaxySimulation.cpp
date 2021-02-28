// Fill out your copyright notice in the Description page of Project Settings.

#include "NiagaraDataInterfaceGalaxySimulation.h"

#include "FastMultipoleSimulation.h"
#include "FastMultipoleSimulationCache.h"
#include "FastMultipoleSimulationModule.h"
#include "GalaxySimulationAsset.h"
#include "NiagaraSystemInstance.h"

#define LOCTEXT_NAMESPACE "NiagaraDataInterfaceGalaxySimulation"
DEFINE_LOG_CATEGORY_STATIC(LogGalaxySimulation, Log, All);

//------------------------------------------------------------------------------------------------------------

static const FName GetNumPointsName(TEXT("GetNumPoints"));
static const FName GetPointStateName(TEXT("GetPointState"));
static const FName ResetCacheName(TEXT("ResetCache"));
static const FName AddPointName(TEXT("AddPoint"));

//------------------------------------------------------------------------------------------------------------

bool FNDIGalaxySimulationInstanceData_GameThread::Init(
	UNiagaraDataInterfaceGalaxySimulation* Interface, FNiagaraSystemInstance* SystemInstance)
{
	SimulationCacheWeak = nullptr;

	if (!Interface->SimulationAsset)
	{
		UE_LOG(
			LogGalaxySimulation, Log, TEXT("GalaxySimulation data interface has no valid simulation asset - %s"),
			*Interface->GetFullName());
		return false;
	}

	if (UFastMultipoleSimulationCache* SimulationCache = Interface->SimulationAsset->GetSimulationCache())
	{
		SimulationCacheWeak = SimulationCache;
		CachePlayer.SetSimulationCache(SimulationCache);
		CacheResetHandle = SimulationCache->OnReset.AddRaw(this, &FNDIGalaxySimulationInstanceData_GameThread::OnCacheReset);
		CacheFrameAddedHandle = SimulationCache->OnFrameAdded.AddRaw(this, &FNDIGalaxySimulationInstanceData_GameThread::OnCacheFrameAdded);
	}

	SimulationSettings = Interface->SimulationAsset->SimulationSettings;

	return true;
}

void FNDIGalaxySimulationInstanceData_GameThread::Release()
{
	StopSimulation();

	if (UFastMultipoleSimulationCache* SimulationCache = SimulationCacheWeak.Get())
	{
		SimulationCache->OnReset.Remove(CacheResetHandle);
		SimulationCache->OnFrameAdded.Remove(CacheFrameAddedHandle);
	}
}

bool FNDIGalaxySimulationInstanceData_GameThread::HasExportedParticles() const
{
	return !ExportedParticles.IsEmpty();
}

void FNDIGalaxySimulationInstanceData_GameThread::AddExportedParticles(FGalaxySimulationParticleExportData& ExportData)
{
	ExportedParticles.Enqueue(ExportData);
}

void FNDIGalaxySimulationInstanceData_GameThread::GatherExportedParticles(
	const FFastMultipoleSimulationInvariants::Ptr& OutInvariants, const FFastMultipoleSimulationFrame::Ptr& OutFrame)
{
	// Drain the queue into a new simulation frame
	int32 NumExportedParticles = 0;
	TQueue<FGalaxySimulationParticleExportData, EQueueMode::Spsc> CountedParticles;
	FGalaxySimulationParticleExportData ParticleData;
	while (ExportedParticles.Dequeue(ParticleData))
	{
		NumExportedParticles += ParticleData.Positions.Num();
		CountedParticles.Enqueue(ParticleData);
	}

	// TODO use point IDs
	TArray<float>& Masses = OutInvariants->Masses;
	TArray<FVector>& Positions = OutFrame->GetPositions();
	TArray<FVector>& Velocities = OutFrame->GetVelocities();
	TArray<FVector>& Forces = OutFrame->GetForces();
	Masses.Empty(NumExportedParticles);
	Positions.Empty(NumExportedParticles);
	Velocities.Empty(NumExportedParticles);
	Forces.Empty(NumExportedParticles);

	while (CountedParticles.Dequeue(ParticleData))
	{
		Masses.Append(ParticleData.Masses);
		Positions.Append(ParticleData.Positions);
		Velocities.Append(ParticleData.Velocities);
	}

	TArray<float>& InvMasses = OutInvariants->InvMasses;
	InvMasses.Empty(NumExportedParticles);
	for (int32 i = 0; i < NumExportedParticles; ++i)
	{
		const float Mass = Masses[i];
		InvMasses.Add(FMath::IsNearlyZero(Mass) ? 0.0f : 1.0f / Mass);
	}

	Forces.AddZeroed(NumExportedParticles);

	check(OutFrame->IsValid());
}

void FNDIGalaxySimulationInstanceData_GameThread::DiscardExportedParticles()
{
	ExportedParticles.Empty();
}

bool FNDIGalaxySimulationInstanceData_GameThread::StartSimulation(UWorld* DebugWorld)
{
	UFastMultipoleSimulationCache* SimCache = SimulationCacheWeak.Get();
	if (!SimCache || !HasExportedParticles())
	{
		return false;
	}

	// TODO implement variable-size cache frames and allow adding/removing particles over time
	if (SimulationCacheWeak->GetNumFrames() == 0)
	{
		FFastMultipoleSimulationInvariants::Ptr Invariants = MakeShared<FFastMultipoleSimulationInvariants, ESPMode::ThreadSafe>();
		FFastMultipoleSimulationFrame::Ptr StartFrame = MakeShared<FFastMultipoleSimulationFrame, ESPMode::ThreadSafe>();
		GatherExportedParticles(Invariants, StartFrame);

		float KineticEnergyAverage;
		FFastMultipoleSimulationUtils::ComputeStableForceFactor(
			SimulationSettings, Invariants, StartFrame, KineticEnergyAverage, Invariants->ForceFactor);

		SimulationCacheWeak->AddFrame(StartFrame);

		FFastMultipoleSimulationModule::Get().StartSimulation(SimulationSettings, Invariants, StartFrame, DebugWorld);

		return true;
	}

	return false;
}

void FNDIGalaxySimulationInstanceData_GameThread::StopSimulation()
{
	FFastMultipoleSimulationModule::Get().StopSimulation();
}

void FNDIGalaxySimulationInstanceData_GameThread::SchedulePrecomputeSteps(int32 NumStepsPrecompute)
{
	if (UFastMultipoleSimulationCache* SimCache = SimulationCacheWeak.Get())
	{
		FFastMultipoleSimulationModule& SimulationModule = FFastMultipoleSimulationModule::Get();

		const int32 LastValidStep = FMath::Max(SimulationCacheWeak->GetNumFrames() - 1, 0);
		// Player interpolates between current cache step and the next, so have to add one frame
		const int32 LastStepUsed = CachePlayer.GetCacheStep() + 1;
		check(LastStepUsed >= 1 && LastStepUsed <= LastValidStep + 1);
		const int32 RemainingCacheFrames = LastValidStep - LastStepUsed;
		// -1 means the cache player stopped on the last valid step, waiting for more cache frames
		check(RemainingCacheFrames >= -1);
		const int32 MaxStepsSchedule = NumStepsPrecompute - RemainingCacheFrames;
		SimulationModule.ScheduleSteps(MaxStepsSchedule);
	}
}

void FNDIGalaxySimulationInstanceData_GameThread::OnCacheReset(UFastMultipoleSimulationCache* SimulationCache)
{
	CachePlayer.SetToFront();
}

void FNDIGalaxySimulationInstanceData_GameThread::OnCacheFrameAdded(UFastMultipoleSimulationCache* SimulationCache)
{
	CachePlayer.UpdateCacheFrames();
}

//------------------------------------------------------------------------------------------------------------

FNDIGalaxySimulationParametersName::FNDIGalaxySimulationParametersName(const FString& Suffix)
{
	// GridCurrentBufferName = UNiagaraDataInterfaceVelocityGrid::GridCurrentBufferName + Suffix;
	// GridDestinationBufferName = UNiagaraDataInterfaceVelocityGrid::GridDestinationBufferName + Suffix;

	// GridSizeName = UNiagaraDataInterfaceVelocityGrid::GridSizeName + Suffix;
	// WorldTransformName = UNiagaraDataInterfaceVelocityGrid::WorldTransformName + Suffix;
	// WorldInverseName = UNiagaraDataInterfaceVelocityGrid::WorldInverseName + Suffix;
}

//------------------------------------------------------------------------------------------------------------

void FNDIGalaxySimulationParametersCS::Bind(
	const FNiagaraDataInterfaceGPUParamInfo& ParameterInfo, const class FShaderParameterMap& ParameterMap)
{
	FNDIGalaxySimulationParametersName ParamNames(ParameterInfo.DataInterfaceHLSLSymbol);

	// GridCurrentBuffer.Bind(ParameterMap, *ParamNames.GridCurrentBufferName);
	// GridDestinationBuffer.Bind(ParameterMap, *ParamNames.GridDestinationBufferName);

	// GridSize.Bind(ParameterMap, *ParamNames.GridSizeName);
	// WorldTransform.Bind(ParameterMap, *ParamNames.WorldTransformName);
	// WorldInverse.Bind(ParameterMap, *ParamNames.WorldInverseName);
}

void FNDIGalaxySimulationParametersCS::Set(FRHICommandList& RHICmdList, const FNiagaraDataInterfaceSetArgs& Context) const
{
	check(IsInRenderingThread());

	FRHIComputeShader* ComputeShaderRHI = RHICmdList.GetBoundComputeShader();

	FNDIGalaxySimulationProxy* InterfaceProxy = static_cast<FNDIGalaxySimulationProxy*>(Context.DataInterface);
	FNDIGalaxySimulationInstanceData_RenderThread* ProxyData = InterfaceProxy->SystemInstancesToProxyData.Find(Context.SystemInstanceID);

	// if (ProxyData != nullptr && ProxyData->CurrentGridBuffer != nullptr && ProxyData->DestinationGridBuffer != nullptr &&
	//	ProxyData->CurrentGridBuffer->IsInitialized() && ProxyData->DestinationGridBuffer->IsInitialized())
	//{
	//	FNDIVelocityGridBuffer* CurrentGridBuffer = ProxyData->CurrentGridBuffer;
	//	FNDIVelocityGridBuffer* DestinationGridBuffer = ProxyData->DestinationGridBuffer;

	//	SetUAVParameter(RHICmdList, ComputeShaderRHI, GridDestinationBuffer, DestinationGridBuffer->GridDataBuffer.UAV);
	//	SetSRVParameter(RHICmdList, ComputeShaderRHI, GridCurrentBuffer, CurrentGridBuffer->GridDataBuffer.SRV);

	//	SetShaderValue(RHICmdList, ComputeShaderRHI, GridSize, ProxyData->GridSize);
	//	SetShaderValue(RHICmdList, ComputeShaderRHI, WorldTransform, ProxyData->WorldTransform);
	//	SetShaderValue(RHICmdList, ComputeShaderRHI, WorldInverse, ProxyData->WorldTransform.Inverse());
	//}
	// else
	//{
	//	SetUAVParameter(
	//		RHICmdList, ComputeShaderRHI, GridDestinationBuffer, Context.Batcher->GetEmptyRWBufferFromPool(RHICmdList, PF_R32_UINT));
	//	SetSRVParameter(RHICmdList, ComputeShaderRHI, GridCurrentBuffer, FNiagaraRenderer::GetDummyUIntBuffer());

	//	SetShaderValue(RHICmdList, ComputeShaderRHI, GridSize, FIntVector());
	//	SetShaderValue(RHICmdList, ComputeShaderRHI, WorldTransform, FMatrix::Identity);
	//	SetShaderValue(RHICmdList, ComputeShaderRHI, WorldInverse, FMatrix::Identity);
	//}
}

void FNDIGalaxySimulationParametersCS::Unset(FRHICommandList& RHICmdList, const FNiagaraDataInterfaceSetArgs& Context) const
{
	FRHIComputeShader* ShaderRHI = RHICmdList.GetBoundComputeShader();
	// SetUAVParameter(RHICmdList, ShaderRHI, GridDestinationBuffer, nullptr);
}

IMPLEMENT_TYPE_LAYOUT(FNDIGalaxySimulationParametersCS);

IMPLEMENT_NIAGARA_DI_PARAMETER(UNiagaraDataInterfaceGalaxySimulation, FNDIGalaxySimulationParametersCS);

//------------------------------------------------------------------------------------------------------------

UNiagaraDataInterfaceGalaxySimulation::UNiagaraDataInterfaceGalaxySimulation(FObjectInitializer const& ObjectInitializer)
	: Super(ObjectInitializer)
	, SimulationAsset(nullptr)
{
	Proxy.Reset(new FNDIGalaxySimulationProxy());
}

bool UNiagaraDataInterfaceGalaxySimulation::InitPerInstanceData(void* PerInstanceData, FNiagaraSystemInstance* SystemInstance)
{
	// const FIntVector ClampedSize =
	//	FIntVector(FMath::Clamp(GridSize.X, 0, 50), FMath::Clamp(GridSize.Y, 0, 50), FMath::Clamp(GridSize.Z, 0, 50));

	// if (GridSize != ClampedSize)
	//{
	//	UE_LOG(LogGalaxySimulation, Warning, TEXT("The grid size is beyond its maximum value (50)"));
	//}
	// GridSize = ClampedSize;

	FNDIGalaxySimulationInstanceData_GameThread* InstanceData = new (PerInstanceData) FNDIGalaxySimulationInstanceData_GameThread();
	check(InstanceData);

	if (InstanceData->Init(this, SystemInstance))
	{
		if (bInitSimulationCache && SimulationAsset && SimulationAsset->GetSimulationCache())
		{
			SimulationAsset->GetSimulationCache()->Reset();
		}
		return true;
	}
	return false;
}

void UNiagaraDataInterfaceGalaxySimulation::DestroyPerInstanceData(void* PerInstanceData, FNiagaraSystemInstance* SystemInstance)
{
	FNDIGalaxySimulationInstanceData_GameThread* InstanceData = static_cast<FNDIGalaxySimulationInstanceData_GameThread*>(PerInstanceData);

	InstanceData->Release();
	InstanceData->~FNDIGalaxySimulationInstanceData_GameThread();

	FNDIGalaxySimulationProxy* ThisProxy = GetProxyAs<FNDIGalaxySimulationProxy>();
	ENQUEUE_RENDER_COMMAND(FNiagaraDIDestroyInstanceData)
	([ThisProxy, InstanceID = SystemInstance->GetId(), Batcher = SystemInstance->GetBatcher()](FRHICommandListImmediate& CmdList) {
		ThisProxy->SystemInstancesToProxyData.Remove(InstanceID);
	});
}

bool UNiagaraDataInterfaceGalaxySimulation::PerInstanceTick(
	void* PerInstanceData, FNiagaraSystemInstance* SystemInstance, float InDeltaSeconds)
{
	FNDIGalaxySimulationInstanceData_GameThread* InstanceData = static_cast<FNDIGalaxySimulationInstanceData_GameThread*>(PerInstanceData);

	bool RequireReset = false;
	if (InstanceData)
	{
		// InstanceData->WorldTransform = SystemInstance->GetWorldTransform().ToMatrixWithScale();

		// if (InstanceData->NeedResize)
		//{
		//	InstanceData->Resize();
		//}
	}
	return RequireReset;
}

bool UNiagaraDataInterfaceGalaxySimulation::PerInstanceTickPostSimulate(
	void* PerInstanceData, FNiagaraSystemInstance* SystemInstance, float DeltaSeconds)
{
	FNDIGalaxySimulationInstanceData_GameThread* InstanceData = static_cast<FNDIGalaxySimulationInstanceData_GameThread*>(PerInstanceData);
	if (InstanceData)
	{
		// Initialize the cache with spawned particles
		if (bInitSimulationCache)
		{
			if (InstanceData->StartSimulation())
			{
				InstanceData->SchedulePrecomputeSteps(NumStepsPrecompute);
			}
		}
		else
		{
			InstanceData->DiscardExportedParticles();
		}

		FFastMultipoleSimulationModule& SimulationModule = FFastMultipoleSimulationModule::Get();
		UFastMultipoleSimulationCache* SimulationCache = InstanceData->SimulationCacheWeak.Get();
		if (SimulationCache)
		{
			// Store finished simulation frames in the cache
			SimulationModule.CacheCompletedSteps(SimulationCache);

			// Schedule new simulation steps if the player reaches the end of the cache
			InstanceData->SchedulePrecomputeSteps(NumStepsPrecompute);
		}
	}
	return false;
}

bool UNiagaraDataInterfaceGalaxySimulation::CopyToInternal(UNiagaraDataInterface* Destination) const
{
	if (!Super::CopyToInternal(Destination))
	{
		return false;
	}

	UNiagaraDataInterfaceGalaxySimulation* OtherTyped = CastChecked<UNiagaraDataInterfaceGalaxySimulation>(Destination);
	OtherTyped->SimulationAsset = SimulationAsset;

	return true;
}

bool UNiagaraDataInterfaceGalaxySimulation::Equals(const UNiagaraDataInterface* Other) const
{
	if (!Super::Equals(Other))
	{
		return false;
	}
	const UNiagaraDataInterfaceGalaxySimulation* OtherTyped = CastChecked<const UNiagaraDataInterfaceGalaxySimulation>(Other);

	return (OtherTyped->SimulationAsset == SimulationAsset);
}

void UNiagaraDataInterfaceGalaxySimulation::PostInitProperties()
{
	Super::PostInitProperties();

	if (HasAnyFlags(RF_ClassDefaultObject))
	{
		FNiagaraTypeRegistry::Register(FNiagaraTypeDefinition(GetClass()), true, false, false);
		FNiagaraTypeRegistry::Register(FFastMultipoleCacheTime::StaticStruct(), true, false, false);
	}
}

void UNiagaraDataInterfaceGalaxySimulation::GetFunctions(TArray<FNiagaraFunctionSignature>& OutFunctions)
{
	OutFunctions.Reserve(OutFunctions.Num() + 2);

	{
		FNiagaraFunctionSignature& Sig = OutFunctions.AddDefaulted_GetRef();
		Sig.Name = GetNumPointsName;
#if WITH_EDITORONLY_DATA
		Sig.Description = LOCTEXT("GetNumPoints", "Get the position of the simulation point at the given zero based index.");
#endif
		Sig.bMemberFunction = true;
		Sig.bRequiresContext = false;
		Sig.bExperimental = false;
		Sig.bSupportsCPU = true;
		Sig.bSupportsGPU = false;
		Sig.Inputs.Add(FNiagaraVariable(FNiagaraTypeDefinition(GetClass()), TEXT("Simulation interface")));
		Sig.Inputs.Add(FNiagaraVariable(FFastMultipoleCacheTime::StaticStruct(), TEXT("Cache Time")));
		Sig.Outputs.Add(FNiagaraVariable(FNiagaraTypeDefinition::GetIntDef(), TEXT("Value")));
	}

	{
		FNiagaraFunctionSignature& Sig = OutFunctions.AddDefaulted_GetRef();
		Sig.Name = GetPointStateName;
#if WITH_EDITORONLY_DATA
		Sig.Description = LOCTEXT("GetPointState", "Get the state of the simulation point at the given zero based index.");
#endif
		Sig.bMemberFunction = true;
		Sig.bRequiresContext = false;
		Sig.bExperimental = false;
		Sig.bSupportsCPU = true;
		Sig.bSupportsGPU = false;
		Sig.Inputs.Add(FNiagaraVariable(FNiagaraTypeDefinition(GetClass()), TEXT("Simulation interface")));
		Sig.Inputs.Add(FNiagaraVariable(FFastMultipoleCacheTime::StaticStruct(), TEXT("Cache Time")));
		Sig.Inputs.Add(FNiagaraVariable(FNiagaraTypeDefinition::GetIntDef(), TEXT("Index")));
		Sig.Outputs.Add(FNiagaraVariable(FNiagaraTypeDefinition::GetVec3Def(), TEXT("Position")));
		Sig.Outputs.Add(FNiagaraVariable(FNiagaraTypeDefinition::GetVec3Def(), TEXT("Velocity")));
	}

	{
		FNiagaraFunctionSignature& Sig = OutFunctions.AddDefaulted_GetRef();
		Sig.Name = ResetCacheName;
#if WITH_EDITORONLY_DATA
		Sig.Description = LOCTEXT("ResetCache", "Remove all cache frames.");
#endif
		Sig.bMemberFunction = true;
		Sig.bRequiresContext = false;
		Sig.bExperimental = false;
		Sig.bRequiresExecPin = true;
		Sig.bWriteFunction = true;
		Sig.bSupportsCPU = true;
		Sig.bSupportsGPU = false;
		Sig.Inputs.Add(FNiagaraVariable(FNiagaraTypeDefinition(GetClass()), TEXT("Simulation interface")));
	}

	{
		FNiagaraFunctionSignature& Sig = OutFunctions.AddDefaulted_GetRef();
		Sig.Name = AddPointName;
#if WITH_EDITORONLY_DATA
		Sig.Description = LOCTEXT("AddPoint", "Add a new point to the simulation cache.");
#endif
		Sig.bMemberFunction = true;
		Sig.bRequiresContext = false;
		Sig.bExperimental = false;
		Sig.bRequiresExecPin = true;
		Sig.bWriteFunction = true;
		Sig.bSupportsCPU = true;
		Sig.bSupportsGPU = false;
		Sig.Inputs.Add(FNiagaraVariable(FNiagaraTypeDefinition(GetClass()), TEXT("Simulation interface")));
		Sig.Inputs.Add(FNiagaraVariable(FNiagaraTypeDefinition::GetIntDef(), TEXT("Point ID")));
		Sig.Inputs.Add(FNiagaraVariable(FNiagaraTypeDefinition::GetFloatDef(), TEXT("Mass")));
		Sig.Inputs.Add(FNiagaraVariable(FNiagaraTypeDefinition::GetVec3Def(), TEXT("Position")));
		Sig.Inputs.Add(FNiagaraVariable(FNiagaraTypeDefinition::GetVec3Def(), TEXT("Velocity")));
	}
}

DEFINE_NDI_DIRECT_FUNC_BINDER(UNiagaraDataInterfaceGalaxySimulation, GetNumPoints);
DEFINE_NDI_DIRECT_FUNC_BINDER(UNiagaraDataInterfaceGalaxySimulation, GetPointState);
DEFINE_NDI_DIRECT_FUNC_BINDER(UNiagaraDataInterfaceGalaxySimulation, ResetCache);
DEFINE_NDI_DIRECT_FUNC_BINDER(UNiagaraDataInterfaceGalaxySimulation, AddPoint);

void UNiagaraDataInterfaceGalaxySimulation::GetVMExternalFunction(
	const FVMExternalFunctionBindingInfo& BindingInfo, void* InstanceData, FVMExternalFunction& OutFunc)
{
	if (BindingInfo.Name == GetNumPointsName)
	{
		check(BindingInfo.GetNumInputs() == 3 && BindingInfo.GetNumOutputs() == 1);
		NDI_FUNC_BINDER(UNiagaraDataInterfaceGalaxySimulation, GetNumPoints)::Bind(this, OutFunc);
	}
	else if (BindingInfo.Name == GetPointStateName)
	{
		check(BindingInfo.GetNumInputs() == 4 && BindingInfo.GetNumOutputs() == 6);
		NDI_FUNC_BINDER(UNiagaraDataInterfaceGalaxySimulation, GetPointState)::Bind(this, OutFunc);
	}
	else if (BindingInfo.Name == ResetCacheName)
	{
		check(BindingInfo.GetNumInputs() == 1 && BindingInfo.GetNumOutputs() == 0);
		NDI_FUNC_BINDER(UNiagaraDataInterfaceGalaxySimulation, ResetCache)::Bind(this, OutFunc);
	}
	else if (BindingInfo.Name == AddPointName)
	{
		check(BindingInfo.GetNumInputs() == 9 && BindingInfo.GetNumOutputs() == 0);
		NDI_FUNC_BINDER(UNiagaraDataInterfaceGalaxySimulation, AddPoint)::Bind(this, OutFunc);
	}
}

void UNiagaraDataInterfaceGalaxySimulation::GetNumPoints(FVectorVMContext& Context)
{
	VectorVM::FUserPtrHandler<FNDIGalaxySimulationInstanceData_GameThread> InstanceData(Context);
	// FFastMultipoleCacheTime
	FNDIInputParam<int32> InCacheStep(Context);
	FNDIInputParam<float> InAnimationTime(Context);
	FNDIOutputParam<int32> OutNumPoints(Context);

	check(InCacheStep.Data.IsConstant());
	check(InAnimationTime.Data.IsConstant());
	FFastMultipoleCacheTime CacheTime(InCacheStep.GetAndAdvance(), InAnimationTime.GetAndAdvance());

	FFastMultipoleSimulationFrame::ConstPtr StartFrame = InstanceData->CachePlayer.GetStartFrame();
	FFastMultipoleSimulationFrame::ConstPtr EndFrame = InstanceData->CachePlayer.GetEndFrame();
	int32 NumPoints;
	if (StartFrame)
	{
		if (EndFrame)
		{
			NumPoints = FMath::Min(StartFrame->GetNumPoints(), EndFrame->GetNumPoints());
		}
		else
		{
			NumPoints = StartFrame->GetNumPoints();
		}
	}
	else
	{
		check(!EndFrame);
		NumPoints = 0;
	}

	for (int32 i = 0; i < Context.NumInstances; ++i)
	{
		OutNumPoints.SetAndAdvance(NumPoints);
	}
}

void UNiagaraDataInterfaceGalaxySimulation::GetPointState(FVectorVMContext& Context)
{
	VectorVM::FUserPtrHandler<FNDIGalaxySimulationInstanceData_GameThread> InstanceData(Context);
	// FFastMultipoleCacheTime
	FNDIInputParam<int32> InCacheStep(Context);
	FNDIInputParam<float> InAnimationTime(Context);
	FNDIInputParam<int32> InIndex(Context);
	FNDIOutputParam<FVector> OutPosition(Context);
	FNDIOutputParam<FVector> OutVelocity(Context);

	check(InCacheStep.Data.IsConstant());
	check(InAnimationTime.Data.IsConstant());
	FFastMultipoleCacheTime CacheTime(InCacheStep.GetAndAdvance(), InAnimationTime.GetAndAdvance());

	FFastMultipoleSimulationFrame::ConstPtr StartFrame = InstanceData->CachePlayer.GetStartFrame();
	FFastMultipoleSimulationFrame::ConstPtr EndFrame = InstanceData->CachePlayer.GetEndFrame();
	float Alpha = InstanceData->CachePlayer.GetAnimationTime();
	if (StartFrame)
	{
		const TArray<FVector>& StartPositions = StartFrame->GetPositions();
		const TArray<FVector>& StartVelocities = StartFrame->GetVelocities();
		if (EndFrame)
		{
			const TArray<FVector>& EndPositions = EndFrame->GetPositions();
			const TArray<FVector>& EndVelocities = EndFrame->GetVelocities();
			for (int32 i = 0; i < Context.NumInstances; ++i)
			{
				int32 Index = InIndex.GetAndAdvance();
				if (StartPositions.IsValidIndex(Index) && EndPositions.IsValidIndex(Index))
				{
					OutPosition.SetAndAdvance(FMath::Lerp(StartPositions[Index], EndPositions[Index], Alpha));
				}
				else
				{
					OutPosition.SetAndAdvance(FVector::ZeroVector);
				}
				if (StartVelocities.IsValidIndex(Index) && EndVelocities.IsValidIndex(Index))
				{
					OutVelocity.SetAndAdvance(FMath::Lerp(StartVelocities[Index], EndVelocities[Index], Alpha));
				}
				else
				{
					OutVelocity.SetAndAdvance(FVector::ZeroVector);
				}
			}
		}
		else
		{
			for (int32 i = 0; i < Context.NumInstances; ++i)
			{
				int32 Index = InIndex.GetAndAdvance();
				if (StartPositions.IsValidIndex(Index))
				{
					OutPosition.SetAndAdvance(StartPositions[Index]);
				}
				else
				{
					OutPosition.SetAndAdvance(FVector::ZeroVector);
				}
				if (StartVelocities.IsValidIndex(Index))
				{
					OutVelocity.SetAndAdvance(StartVelocities[Index]);
				}
				else
				{
					OutVelocity.SetAndAdvance(FVector::ZeroVector);
				}
			}
		}
	}
	else
	{
		for (int32 i = 0; i < Context.NumInstances; ++i)
		{
			OutPosition.SetAndAdvance(FVector::ZeroVector);
			OutVelocity.SetAndAdvance(FVector::ZeroVector);
		}
	}
}

void UNiagaraDataInterfaceGalaxySimulation::ResetCache(FVectorVMContext& Context)
{
}

void UNiagaraDataInterfaceGalaxySimulation::AddPoint(FVectorVMContext& Context)
{
	VectorVM::FUserPtrHandler<FNDIGalaxySimulationInstanceData_GameThread> InstanceData(Context);
	FNDIInputParam<int32> InPointID(Context);
	FNDIInputParam<float> InMass(Context);
	FNDIInputParam<FVector> InPosition(Context);
	FNDIInputParam<FVector> InVelocity(Context);

	if (bInitSimulationCache)
	{
		FGalaxySimulationParticleExportData ExportData;
		ExportData.UniqueIDs.SetNumUninitialized(Context.NumInstances);
		ExportData.Masses.SetNumUninitialized(Context.NumInstances);
		ExportData.Positions.SetNumUninitialized(Context.NumInstances);
		ExportData.Velocities.SetNumUninitialized(Context.NumInstances);
		for (int32 i = 0; i < Context.NumInstances; ++i)
		{
			ExportData.UniqueIDs[i] = InPointID.GetAndAdvance();
			ExportData.Masses[i] = InMass.GetAndAdvance();
			ExportData.Positions[i] = InPosition.GetAndAdvance();
			ExportData.Velocities[i] = InVelocity.GetAndAdvance();
		}

		InstanceData->AddExportedParticles(ExportData);
	}
}

bool UNiagaraDataInterfaceGalaxySimulation::GetFunctionHLSL(
	const FNiagaraDataInterfaceGPUParamInfo& ParamInfo, const FNiagaraDataInterfaceGeneratedFunction& FunctionInfo,
	int FunctionInstanceIndex, FString& OutHLSL)
{
	// FNDIVelocityGridParametersName ParamNames(ParamInfo.DataInterfaceHLSLSymbol);

	// TMap<FString, FStringFormatArg> ArgsSample = {
	//	{TEXT("InstanceFunctionName"), FunctionInfo.InstanceName},
	//	{TEXT("VelocityGridContextName"), TEXT("DIVelocityGrid_MAKE_CONTEXT(") + ParamInfo.DataInterfaceHLSLSymbol + TEXT(")")},
	//};

	// if (FunctionInfo.DefinitionName == BuildVelocityFieldName)
	//{
	//	static const TCHAR* FormatSample = TEXT(R"(
	//			void {InstanceFunctionName} (in float3 GridOrigin, in float GridLength, in float3 ParticlePosition, in float ParticleMass,
	// in float3 ParticleVelocity, in float4x4 VelocityGradient, out bool OutFunctionStatus)
	//			{
	//				{VelocityGridContextName}
	// DIVelocityGrid_BuildVelocityField(DIContext,GridOrigin,GridLength,ParticlePosition,ParticleMass,ParticleVelocity,VelocityGradient,OutFunctionStatus);
	//			}
	//			)");
	//	OutHLSL += FString::Format(FormatSample, ArgsSample);
	//	return true;
	//}
	// else if (FunctionInfo.DefinitionName == SampleVelocityFieldName)
	//{
	//	static const TCHAR* FormatSample = TEXT(R"(
	//			void {InstanceFunctionName} (in float3 GridOrigin, in float GridLength, in float3 ParticlePosition, in bool ScaledVelocity,
	// out float OutParticleMass, out float3 OutParticleVelocity, out float4x4 OutVelocityGradient)
	//			{
	//				{VelocityGridContextName}
	// DIVelocityGrid_SampleVelocityField(DIContext,GridOrigin,GridLength,ParticlePosition,ScaledVelocity,OutParticleMass,OutParticleVelocity,OutVelocityGradient);
	//			}
	//			)");
	//	OutHLSL += FString::Format(FormatSample, ArgsSample);
	//	return true;
	//}
	// else if (FunctionInfo.DefinitionName == ComputeGridSizeName)
	//{
	//	static const TCHAR* FormatSample = TEXT(R"(
	//			void {InstanceFunctionName} (in float3 GridCenter, in float3 GridExtent, out float3 OutGridOrigin, out float OutGridLength)
	//			{
	//				{VelocityGridContextName} DIVelocityGrid_ComputeGridSize(DIContext,GridCenter,GridExtent,OutGridOrigin,OutGridLength);
	//			}
	//			)");
	//	OutHLSL += FString::Format(FormatSample, ArgsSample);
	//	return true;
	//}
	OutHLSL += TEXT("\n");
	return false;
}

void UNiagaraDataInterfaceGalaxySimulation::GetCommonHLSL(FString& OutHLSL)
{
	// OutHLSL += TEXT("#include \"/Plugin/Runtime/HairStrands/Private/NiagaraDataInterfaceVelocityGrid.ush\"\n");
}

void UNiagaraDataInterfaceGalaxySimulation::GetParameterDefinitionHLSL(const FNiagaraDataInterfaceGPUParamInfo& ParamInfo, FString& OutHLSL)
{
	OutHLSL += /*TEXT("DIVelocityGrid_DECLARE_CONSTANTS(") +*/ ParamInfo.DataInterfaceHLSLSymbol + TEXT(")\n");
}

void UNiagaraDataInterfaceGalaxySimulation::ProvidePerInstanceDataForRenderThread(
	void* DataForRenderThread, void* PerInstanceData, const FNiagaraSystemInstanceID& SystemInstance)
{
	FNDIGalaxySimulationInstanceData_GameThread* GameThreadData =
		static_cast<FNDIGalaxySimulationInstanceData_GameThread*>(PerInstanceData);
	FNDIGalaxySimulationInstanceData_GameThread* RenderThreadData =
		static_cast<FNDIGalaxySimulationInstanceData_GameThread*>(DataForRenderThread);

	// RenderThreadData->WorldTransform = GameThreadData->WorldTransform;
	// RenderThreadData->WorldInverse = GameThreadData->WorldInverse;
	// RenderThreadData->CurrentGridBuffer = GameThreadData->CurrentGridBuffer;
	// RenderThreadData->DestinationGridBuffer = GameThreadData->DestinationGridBuffer;
	// RenderThreadData->GridSize = GameThreadData->GridSize;
}

//------------------------------------------------------------------------------------------------------------

// class FCopyGalaxySimulationCS : public FGlobalShader
//{
//	DECLARE_GLOBAL_SHADER(FCopyVelocityGridCS);
//	SHADER_USE_PARAMETER_STRUCT(FCopyVelocityGridCS, FGlobalShader);
//
//	BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
//	SHADER_PARAMETER(FIntVector, GridSize)
//	SHADER_PARAMETER_SRV(Texture3D, GridCurrentBuffer)
//	SHADER_PARAMETER_UAV(RWTexture3D, GridDestinationBuffer)
//	END_SHADER_PARAMETER_STRUCT()
//
// public:
//	static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters)
//	{
//		return RHISupportsComputeShaders(Parameters.Platform);
//	}
//
//	static void ModifyCompilationEnvironment(
//		const FGlobalShaderPermutationParameters& Parameters, FShaderCompilerEnvironment& OutEnvironment)
//	{
//		FGlobalShader::ModifyCompilationEnvironment(Parameters, OutEnvironment);
//		OutEnvironment.SetDefine(TEXT("THREAD_COUNT"), NIAGARA_HAIR_STRANDS_THREAD_COUNT);
//	}
//};
//
// IMPLEMENT_GLOBAL_SHADER(FCopyGalaxySimulationCS, "/Plugin/Runtime/HairStrands/Private/NiagaraCopyVelocityGrid.usf", "MainCS", SF_Compute);
//
// static void AddCopyVelocityGridPass(
//	FRDGBuilder& GraphBuilder, FRHIShaderResourceView* GridCurrentBuffer, FRHIUnorderedAccessView* GridDestinationBuffer,
//	const FIntVector& GridSize)
//{
//	const uint32 GroupSize = NIAGARA_HAIR_STRANDS_THREAD_COUNT;
//	const uint32 NumElements = (GridSize.X + 1) * (GridSize.Y + 1) * (GridSize.Z + 1);
//
//	FCopyVelocityGridCS::FParameters* Parameters = GraphBuilder.AllocParameters<FCopyVelocityGridCS::FParameters>();
//	Parameters->GridCurrentBuffer = GridCurrentBuffer;
//	Parameters->GridDestinationBuffer = GridDestinationBuffer;
//	Parameters->GridSize = GridSize;
//
//	FGlobalShaderMap* ShaderMap = GetGlobalShaderMap(ERHIFeatureLevel::SM5);
//
//	const uint32 DispatchCount = FMath::DivideAndRoundUp(NumElements, GroupSize);
//
//	TShaderMapRef<FCopyVelocityGridCS> ComputeShader(ShaderMap);
//	FComputeShaderUtils::AddPass(
//		GraphBuilder, RDG_EVENT_NAME("CopyVelocityGrid"), ComputeShader, Parameters, FIntVector(DispatchCount, 1, 1));
//}
//
// inline void CopyTexture(
//	FRHICommandList& RHICmdList, FNDIVelocityGridBuffer* CurrentGridBuffer, FNDIVelocityGridBuffer* DestinationGridBuffer,
//	const FIntVector& GridSize)
//{
//	FRHIUnorderedAccessView* DestinationGridBufferUAV = DestinationGridBuffer->GridDataBuffer.UAV;
//	FRHIShaderResourceView* CurrentGridBufferSRV = CurrentGridBuffer->GridDataBuffer.SRV;
//	FRHIUnorderedAccessView* CurrentGridBufferUAV = CurrentGridBuffer->GridDataBuffer.UAV;
//
//	if (DestinationGridBufferUAV != nullptr && CurrentGridBufferSRV != nullptr && CurrentGridBufferUAV != nullptr)
//	{
//		const FIntVector LocalGridSize = GridSize;
//
//		ENQUEUE_RENDER_COMMAND(CopyVelocityGrid)
//		([DestinationGridBufferUAV, CurrentGridBufferSRV, CurrentGridBufferUAV, LocalGridSize](FRHICommandListImmediate& RHICmdListImm) {
//			FRHITransitionInfo Transitions[] = {// FIXME: what's the source state for these?
//												FRHITransitionInfo(CurrentGridBufferUAV, ERHIAccess::Unknown, ERHIAccess::SRVCompute),
//												FRHITransitionInfo(DestinationGridBufferUAV, ERHIAccess::Unknown, ERHIAccess::UAVCompute)};
//			RHICmdListImm.Transition(MakeArrayView(Transitions, UE_ARRAY_COUNT(Transitions)));
//
//			FRDGBuilder GraphBuilder(RHICmdListImm);
//
//			AddCopyVelocityGridPass(GraphBuilder, CurrentGridBufferSRV, DestinationGridBufferUAV, LocalGridSize);
//
//			GraphBuilder.Execute();
//		});
//	}
//}

void FNDIGalaxySimulationProxy::ConsumePerInstanceDataFromGameThread(void* PerInstanceData, const FNiagaraSystemInstanceID& Instance)
{
	FNDIGalaxySimulationInstanceData_GameThread* SourceData = static_cast<FNDIGalaxySimulationInstanceData_GameThread*>(PerInstanceData);
	FNDIGalaxySimulationInstanceData_RenderThread* TargetData = &(SystemInstancesToProxyData.FindOrAdd(Instance));

	ensure(TargetData);
	if (TargetData)
	{
		// TargetData->WorldTransform = SourceData->WorldTransform;
		// TargetData->WorldInverse = SourceData->WorldInverse;
		// TargetData->GridSize = SourceData->GridSize;
		// TargetData->DestinationGridBuffer = SourceData->DestinationGridBuffer;
		// TargetData->CurrentGridBuffer = SourceData->CurrentGridBuffer;
	}
	else
	{
		UE_LOG(
			LogGalaxySimulation, Log, TEXT("ConsumePerInstanceDataFromGameThread() ... could not find %s"),
			*FNiagaraUtilities::SystemInstanceIDToString(Instance));
	}
}

void FNDIGalaxySimulationProxy::PreStage(FRHICommandList& RHICmdList, const FNiagaraDataInterfaceStageArgs& Context)
{
	FNDIGalaxySimulationInstanceData_RenderThread* ProxyData = SystemInstancesToProxyData.Find(Context.SystemInstanceID);

	if (ProxyData != nullptr)
	{
		if (Context.SimulationStageIndex == 0)
		{
			// ClearBuffer(RHICmdList, ProxyData->DestinationGridBuffer);
		}

		// FRHITransitionInfo Transitions[] = {
		//	// FIXME: what's the source state for these?
		//	FRHITransitionInfo(ProxyData->CurrentGridBuffer->GridDataBuffer.UAV, ERHIAccess::Unknown, ERHIAccess::SRVCompute),
		//	FRHITransitionInfo(ProxyData->DestinationGridBuffer->GridDataBuffer.UAV, ERHIAccess::Unknown, ERHIAccess::UAVCompute)};
		// RHICmdList.Transition(MakeArrayView(Transitions, UE_ARRAY_COUNT(Transitions)));
	}
}

void FNDIGalaxySimulationProxy::PostStage(FRHICommandList& RHICmdList, const FNiagaraDataInterfaceStageArgs& Context)
{
	FNDIGalaxySimulationInstanceData_RenderThread* ProxyData = SystemInstancesToProxyData.Find(Context.SystemInstanceID);

	if (ProxyData != nullptr)
	{
		// ProxyData->Swap();
		// CopyTexture(RHICmdList, ProxyData->DestinationGridBuffer, ProxyData->CurrentGridBuffer, ProxyData->GridSize);
		// FRHICopyTextureInfo CopyInfo;
		// RHICmdList.CopyTexture(ProxyData->DestinationGridBuffer->GridDataBuffer.Buffer,
		//	ProxyData->CurrentGridBuffer->GridDataBuffer.Buffer, CopyInfo);
	}
}

void FNDIGalaxySimulationProxy::ResetData(FRHICommandList& RHICmdList, const FNiagaraDataInterfaceArgs& Context)
{
	FNDIGalaxySimulationInstanceData_RenderThread* ProxyData = SystemInstancesToProxyData.Find(Context.SystemInstanceID);

	// if (ProxyData != nullptr && ProxyData->DestinationGridBuffer != nullptr && ProxyData->CurrentGridBuffer != nullptr)
	//{
	//	ClearBuffer(RHICmdList, ProxyData->DestinationGridBuffer);
	//	ClearBuffer(RHICmdList, ProxyData->CurrentGridBuffer);
	//}
}

#undef LOCTEXT_NAMESPACE
