// Fill out your copyright notice in the Description page of Project Settings.

#include "NiagaraDataInterfaceMultipoleGrid.h"

#include "NiagaraSystemInstance.h"

#define LOCTEXT_NAMESPACE "NiagaraDataInterfaceMultipoleGrid"
DEFINE_LOG_CATEGORY_STATIC(LogMultipoleGrid, Log, All);

//------------------------------------------------------------------------------------------------------------

static const FName BuildDistanceFieldName(TEXT("BuildDistanceField"));
static const FName BuildDensityFieldName(TEXT("BuildDensityField"));
static const FName SolveGridPressureName(TEXT("SolveGridPressure"));
static const FName ScaleCellFieldsName(TEXT("ScaleCellFields"));
static const FName SetSolidBoundaryName(TEXT("SetSolidBoundary"));
static const FName ComputeBoundaryWeightsName(TEXT("ComputeBoundaryWeights"));
static const FName GetNodePositionName(TEXT("GetNodePosition"));
static const FName GetDensityFieldName(TEXT("GetDensityField"));
static const FName UpdateDeformationGradientName(TEXT("UpdateDeformationGradient"));

//------------------------------------------------------------------------------------------------------------

bool FNDIMultipoleGridData::Init(FNiagaraSystemInstance* SystemInstance)
{
	return true;
}

void FNDIMultipoleGridData::Release()
{
}

//------------------------------------------------------------------------------------------------------------

FNDIMultipoleGridParametersName::FNDIMultipoleGridParametersName(const FString& Suffix)
{
	//GridCurrentBufferName = UNiagaraDataInterfaceVelocityGrid::GridCurrentBufferName + Suffix;
	//GridDestinationBufferName = UNiagaraDataInterfaceVelocityGrid::GridDestinationBufferName + Suffix;

	//GridSizeName = UNiagaraDataInterfaceVelocityGrid::GridSizeName + Suffix;
	//WorldTransformName = UNiagaraDataInterfaceVelocityGrid::WorldTransformName + Suffix;
	//WorldInverseName = UNiagaraDataInterfaceVelocityGrid::WorldInverseName + Suffix;
}

//------------------------------------------------------------------------------------------------------------

void FNDIMultipoleGridParametersCS::Bind(
	const FNiagaraDataInterfaceGPUParamInfo& ParameterInfo, const class FShaderParameterMap& ParameterMap)
{
	FNDIMultipoleGridParametersName ParamNames(ParameterInfo.DataInterfaceHLSLSymbol);

	//GridCurrentBuffer.Bind(ParameterMap, *ParamNames.GridCurrentBufferName);
	//GridDestinationBuffer.Bind(ParameterMap, *ParamNames.GridDestinationBufferName);

	//GridSize.Bind(ParameterMap, *ParamNames.GridSizeName);
	//WorldTransform.Bind(ParameterMap, *ParamNames.WorldTransformName);
	//WorldInverse.Bind(ParameterMap, *ParamNames.WorldInverseName);
}

void FNDIMultipoleGridParametersCS::Set(FRHICommandList& RHICmdList, const FNiagaraDataInterfaceSetArgs& Context) const
{
	check(IsInRenderingThread());

	FRHIComputeShader* ComputeShaderRHI = RHICmdList.GetBoundComputeShader();

	FNDIMultipoleGridProxy* InterfaceProxy = static_cast<FNDIMultipoleGridProxy*>(Context.DataInterface);
	FNDIMultipoleGridData* ProxyData = InterfaceProxy->SystemInstancesToProxyData.Find(Context.SystemInstanceID);

	//if (ProxyData != nullptr && ProxyData->CurrentGridBuffer != nullptr && ProxyData->DestinationGridBuffer != nullptr &&
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
	//else
	//{
	//	SetUAVParameter(
	//		RHICmdList, ComputeShaderRHI, GridDestinationBuffer, Context.Batcher->GetEmptyRWBufferFromPool(RHICmdList, PF_R32_UINT));
	//	SetSRVParameter(RHICmdList, ComputeShaderRHI, GridCurrentBuffer, FNiagaraRenderer::GetDummyUIntBuffer());

	//	SetShaderValue(RHICmdList, ComputeShaderRHI, GridSize, FIntVector());
	//	SetShaderValue(RHICmdList, ComputeShaderRHI, WorldTransform, FMatrix::Identity);
	//	SetShaderValue(RHICmdList, ComputeShaderRHI, WorldInverse, FMatrix::Identity);
	//}
}

void FNDIMultipoleGridParametersCS::Unset(FRHICommandList& RHICmdList, const FNiagaraDataInterfaceSetArgs& Context) const
{
	FRHIComputeShader* ShaderRHI = RHICmdList.GetBoundComputeShader();
	//SetUAVParameter(RHICmdList, ShaderRHI, GridDestinationBuffer, nullptr);
}

IMPLEMENT_TYPE_LAYOUT(FNDIMultipoleGridParametersCS);

IMPLEMENT_NIAGARA_DI_PARAMETER(UNiagaraDataInterfaceMultipoleGrid, FNDIMultipoleGridParametersCS);

//------------------------------------------------------------------------------------------------------------

UNiagaraDataInterfaceMultipoleGrid::UNiagaraDataInterfaceMultipoleGrid(FObjectInitializer const& ObjectInitializer)
	: Super(ObjectInitializer)
{
	Proxy.Reset(new FNDIMultipoleGridProxy());
}

bool UNiagaraDataInterfaceMultipoleGrid::InitPerInstanceData(void* PerInstanceData, FNiagaraSystemInstance* SystemInstance)
{
	// const FIntVector ClampedSize =
	//	FIntVector(FMath::Clamp(GridSize.X, 0, 50), FMath::Clamp(GridSize.Y, 0, 50), FMath::Clamp(GridSize.Z, 0, 50));

	// if (GridSize != ClampedSize)
	//{
	//	UE_LOG(LogMultipoleGrid, Warning, TEXT("The grid size is beyond its maximum value (50)"));
	//}
	// GridSize = ClampedSize;

	FNDIMultipoleGridData* InstanceData = new (PerInstanceData) FNDIMultipoleGridData();
	check(InstanceData);

	return InstanceData->Init(SystemInstance);
}

void UNiagaraDataInterfaceMultipoleGrid::DestroyPerInstanceData(void* PerInstanceData, FNiagaraSystemInstance* SystemInstance)
{
	FNDIMultipoleGridData* InstanceData = static_cast<FNDIMultipoleGridData*>(PerInstanceData);

	InstanceData->Release();
	InstanceData->~FNDIMultipoleGridData();

	FNDIMultipoleGridProxy* ThisProxy = GetProxyAs<FNDIMultipoleGridProxy>();
	ENQUEUE_RENDER_COMMAND(FNiagaraDIDestroyInstanceData)
	([ThisProxy, InstanceID = SystemInstance->GetId(), Batcher = SystemInstance->GetBatcher()](FRHICommandListImmediate& CmdList) {
		ThisProxy->SystemInstancesToProxyData.Remove(InstanceID);
	});
}

bool UNiagaraDataInterfaceMultipoleGrid::PerInstanceTick(void* PerInstanceData, FNiagaraSystemInstance* SystemInstance, float InDeltaSeconds)
{
	FNDIMultipoleGridData* InstanceData = static_cast<FNDIMultipoleGridData*>(PerInstanceData);

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

bool UNiagaraDataInterfaceMultipoleGrid::CopyToInternal(UNiagaraDataInterface* Destination) const
{
	if (!Super::CopyToInternal(Destination))
	{
		return false;
	}

	UNiagaraDataInterfaceMultipoleGrid* OtherTyped = CastChecked<UNiagaraDataInterfaceMultipoleGrid>(Destination);
	// OtherTyped->GridSize = GridSize;

	return true;
}

bool UNiagaraDataInterfaceMultipoleGrid::Equals(const UNiagaraDataInterface* Other) const
{
	if (!Super::Equals(Other))
	{
		return false;
	}
	const UNiagaraDataInterfaceMultipoleGrid* OtherTyped = CastChecked<const UNiagaraDataInterfaceMultipoleGrid>(Other);

	// return (OtherTyped->GridSize == GridSize);
	return true;
}

void UNiagaraDataInterfaceMultipoleGrid::PostInitProperties()
{
	Super::PostInitProperties();

	if (HasAnyFlags(RF_ClassDefaultObject))
	{
		FNiagaraTypeRegistry::Register(FNiagaraTypeDefinition(GetClass()), true, false, false);
	}
}

void UNiagaraDataInterfaceMultipoleGrid::GetFunctions(TArray<FNiagaraFunctionSignature>& OutFunctions)
{
	//{
	//	FNiagaraFunctionSignature Sig;
	//	Sig.Name = BuildVelocityFieldName;
	//	Sig.bMemberFunction = true;
	//	Sig.bRequiresContext = false;
	//	Sig.bWriteFunction = true;
	//	Sig.bSupportsGPU = true;
	//	Sig.bSupportsCPU = false;
	//	Sig.Inputs.Add(FNiagaraVariable(FNiagaraTypeDefinition(GetClass()), TEXT(" Velocity Grid")));
	//	Sig.Inputs.Add(FNiagaraVariable(FNiagaraTypeDefinition::GetVec3Def(), TEXT("Grid Origin")));
	//	Sig.Inputs.Add(FNiagaraVariable(FNiagaraTypeDefinition::GetFloatDef(), TEXT("Grid Length")));
	//	Sig.Inputs.Add(FNiagaraVariable(FNiagaraTypeDefinition::GetVec3Def(), TEXT("Particle Position")));
	//	Sig.Inputs.Add(FNiagaraVariable(FNiagaraTypeDefinition::GetFloatDef(), TEXT("Particle Mass")));
	//	Sig.Inputs.Add(FNiagaraVariable(FNiagaraTypeDefinition::GetVec3Def(), TEXT("Particle Velocity")));
	//	Sig.Inputs.Add(FNiagaraVariable(FNiagaraTypeDefinition::GetMatrix4Def(), TEXT("Velocity Gradient")));
	//	Sig.Outputs.Add(FNiagaraVariable(FNiagaraTypeDefinition::GetBoolDef(), TEXT("Function Status")));

	//	OutFunctions.Add(Sig);
	//}
	//{
	//	FNiagaraFunctionSignature Sig;
	//	Sig.Name = SampleVelocityFieldName;
	//	Sig.bMemberFunction = true;
	//	Sig.bRequiresContext = false;
	//	Sig.bSupportsGPU = true;
	//	Sig.bSupportsCPU = false;
	//	Sig.Inputs.Add(FNiagaraVariable(FNiagaraTypeDefinition(GetClass()), TEXT(" Velocity Grid")));
	//	Sig.Inputs.Add(FNiagaraVariable(FNiagaraTypeDefinition::GetVec3Def(), TEXT("Grid Origin")));
	//	Sig.Inputs.Add(FNiagaraVariable(FNiagaraTypeDefinition::GetFloatDef(), TEXT("Grid Length")));
	//	Sig.Inputs.Add(FNiagaraVariable(FNiagaraTypeDefinition::GetVec3Def(), TEXT("Particle Position")));
	//	Sig.Inputs.Add(FNiagaraVariable(FNiagaraTypeDefinition::GetBoolDef(), TEXT("Scaled Velocity")));
	//	Sig.Outputs.Add(FNiagaraVariable(FNiagaraTypeDefinition::GetFloatDef(), TEXT("Particle Mass")));
	//	Sig.Outputs.Add(FNiagaraVariable(FNiagaraTypeDefinition::GetVec3Def(), TEXT("Particle Velocity")));
	//	Sig.Outputs.Add(FNiagaraVariable(FNiagaraTypeDefinition::GetMatrix4Def(), TEXT("Velocity Gradient")));

	//	OutFunctions.Add(Sig);
	//}
	//{
	//	FNiagaraFunctionSignature Sig;
	//	Sig.Name = ComputeGridSizeName;
	//	Sig.bMemberFunction = true;
	//	Sig.bRequiresContext = false;
	//	Sig.bSupportsGPU = true;
	//	Sig.bSupportsCPU = false;
	//	Sig.Inputs.Add(FNiagaraVariable(FNiagaraTypeDefinition(GetClass()), TEXT(" Velocity Grid")));
	//	Sig.Inputs.Add(FNiagaraVariable(FNiagaraTypeDefinition::GetVec3Def(), TEXT("Grid Center")));
	//	Sig.Inputs.Add(FNiagaraVariable(FNiagaraTypeDefinition::GetVec3Def(), TEXT("Grid Extent")));
	//	Sig.Outputs.Add(FNiagaraVariable(FNiagaraTypeDefinition::GetVec3Def(), TEXT("Grid Origin")));
	//	Sig.Outputs.Add(FNiagaraVariable(FNiagaraTypeDefinition::GetFloatDef(), TEXT("Grid Length")));

	//	OutFunctions.Add(Sig);
	//}
	//{
	//	FNiagaraFunctionSignature Sig;
	//	Sig.Name = UpdateGridTransformName;
	//	Sig.bMemberFunction = true;
	//	Sig.bRequiresContext = false;
	//	Sig.bWriteFunction = true;
	//	Sig.bSupportsGPU = false;
	//	Sig.bSupportsCPU = true;
	//	Sig.Inputs.Add(FNiagaraVariable(FNiagaraTypeDefinition(GetClass()), TEXT(" Velocity Grid")));
	//	Sig.Inputs.Add(FNiagaraVariable(FNiagaraTypeDefinition::GetMatrix4Def(), TEXT("Grid Transform")));
	//	Sig.Outputs.Add(FNiagaraVariable(FNiagaraTypeDefinition::GetBoolDef(), TEXT("Function Status")));

	//	OutFunctions.Add(Sig);
	//}
	//{
	//	FNiagaraFunctionSignature Sig;
	//	Sig.Name = SetGridDimensionName;
	//	Sig.bMemberFunction = true;
	//	Sig.bRequiresContext = false;
	//	Sig.bWriteFunction = true;
	//	Sig.bSupportsGPU = true;
	//	Sig.bSupportsCPU = true;
	//	Sig.Inputs.Add(FNiagaraVariable(FNiagaraTypeDefinition(GetClass()), TEXT(" Velocity Grid")));
	//	Sig.Inputs.Add(FNiagaraVariable(FNiagaraTypeDefinition::GetVec3Def(), TEXT("Grid Dimension")));
	//	Sig.Outputs.Add(FNiagaraVariable(FNiagaraTypeDefinition::GetBoolDef(), TEXT("Function Status")));

	//	OutFunctions.Add(Sig);
	//}
}

// DEFINE_NDI_DIRECT_FUNC_BINDER(UNiagaraDataInterfaceMultipoleGrid, BuildVelocityField);
// DEFINE_NDI_DIRECT_FUNC_BINDER(UNiagaraDataInterfaceMultipoleGrid, SampleVelocityField);
// DEFINE_NDI_DIRECT_FUNC_BINDER(UNiagaraDataInterfaceMultipoleGrid, ComputeGridSize);
// DEFINE_NDI_DIRECT_FUNC_BINDER(UNiagaraDataInterfaceMultipoleGrid, UpdateGridTransform);
// DEFINE_NDI_DIRECT_FUNC_BINDER(UNiagaraDataInterfaceMultipoleGrid, SetGridDimension);

void UNiagaraDataInterfaceMultipoleGrid::GetVMExternalFunction(
	const FVMExternalFunctionBindingInfo& BindingInfo, void* InstanceData, FVMExternalFunction& OutFunc)
{
	// if (BindingInfo.Name == BuildVelocityFieldName)
	//{
	//	check(BindingInfo.GetNumInputs() == 28 && BindingInfo.GetNumOutputs() == 1);
	//	NDI_FUNC_BINDER(UNiagaraDataInterfaceMultipoleGrid, BuildVelocityField)::Bind(this, OutFunc);
	//}
	// else if (BindingInfo.Name == SampleVelocityFieldName)
	//{
	//	check(BindingInfo.GetNumInputs() == 9 && BindingInfo.GetNumOutputs() == 20);
	//	NDI_FUNC_BINDER(UNiagaraDataInterfaceMultipoleGrid, SampleVelocityField)::Bind(this, OutFunc);
	//}
	// else if (BindingInfo.Name == ComputeGridSizeName)
	//{
	//	check(BindingInfo.GetNumInputs() == 7 && BindingInfo.GetNumOutputs() == 4);
	//	NDI_FUNC_BINDER(UNiagaraDataInterfaceMultipoleGrid, ComputeGridSize)::Bind(this, OutFunc);
	//}
	// else if (BindingInfo.Name == UpdateGridTransformName)
	//{
	//	check(BindingInfo.GetNumInputs() == 17 && BindingInfo.GetNumOutputs() == 1);
	//	NDI_FUNC_BINDER(UNiagaraDataInterfaceMultipoleGrid, UpdateGridTransform)::Bind(this, OutFunc);
	//}
	// else if (BindingInfo.Name == SetGridDimensionName)
	//{
	//	check(BindingInfo.GetNumInputs() == 4 && BindingInfo.GetNumOutputs() == 1);
	//	NDI_FUNC_BINDER(UNiagaraDataInterfaceMultipoleGrid, SetGridDimension)::Bind(this, OutFunc);
	//}
}

void UNiagaraDataInterfaceMultipoleGrid::BuildVelocityField(FVectorVMContext& Context)
{
	// @todo : implement function for cpu
}

void UNiagaraDataInterfaceMultipoleGrid::SampleVelocityField(FVectorVMContext& Context)
{
	// @todo : implement function for cpu
}

void UNiagaraDataInterfaceMultipoleGrid::ComputeGridSize(FVectorVMContext& Context)
{
	// @todo : implement function for cpu
}

void UNiagaraDataInterfaceMultipoleGrid::SetGridDimension(FVectorVMContext& Context)
{
	// VectorVM::FUserPtrHandler<FNDIMultipoleGridData> InstData(Context);
	// VectorVM::FExternalFuncInputHandler<float> GridDimensionX(Context);
	// VectorVM::FExternalFuncInputHandler<float> GridDimensionY(Context);
	// VectorVM::FExternalFuncInputHandler<float> GridDimensionZ(Context);

	// VectorVM::FExternalFuncRegisterHandler<bool> OutFunctionStatus(Context);

	// for (int32 InstanceIdx = 0; InstanceIdx < Context.NumInstances; ++InstanceIdx)
	//{
	//	FIntVector GridDimension;
	//	GridDimension.X = *GridDimensionX.GetDestAndAdvance();
	//	GridDimension.Y = *GridDimensionY.GetDestAndAdvance();
	//	GridDimension.Z = *GridDimensionZ.GetDestAndAdvance();

	//	InstData->GridSize = GridDimension;
	//	InstData->NeedResize = true;

	//	*OutFunctionStatus.GetDestAndAdvance() = true;
	//}
}

void UNiagaraDataInterfaceMultipoleGrid::UpdateGridTransform(FVectorVMContext& Context)
{
	// VectorVM::FUserPtrHandler<FNDIMultipoleGridData> InstData(Context);

	// VectorVM::FExternalFuncInputHandler<float> Out00(Context);
	// VectorVM::FExternalFuncInputHandler<float> Out01(Context);
	// VectorVM::FExternalFuncInputHandler<float> Out02(Context);
	// VectorVM::FExternalFuncInputHandler<float> Out03(Context);

	// VectorVM::FExternalFuncInputHandler<float> Out10(Context);
	// VectorVM::FExternalFuncInputHandler<float> Out11(Context);
	// VectorVM::FExternalFuncInputHandler<float> Out12(Context);
	// VectorVM::FExternalFuncInputHandler<float> Out13(Context);

	// VectorVM::FExternalFuncInputHandler<float> Out20(Context);
	// VectorVM::FExternalFuncInputHandler<float> Out21(Context);
	// VectorVM::FExternalFuncInputHandler<float> Out22(Context);
	// VectorVM::FExternalFuncInputHandler<float> Out23(Context);

	// VectorVM::FExternalFuncInputHandler<float> Out30(Context);
	// VectorVM::FExternalFuncInputHandler<float> Out31(Context);
	// VectorVM::FExternalFuncInputHandler<float> Out32(Context);
	// VectorVM::FExternalFuncInputHandler<float> Out33(Context);

	// VectorVM::FExternalFuncRegisterHandler<bool> OutTransformStatus(Context);

	// for (int32 InstanceIdx = 0; InstanceIdx < Context.NumInstances; ++InstanceIdx)
	//{
	//	FMatrix Transform;
	//	Transform.M[0][0] = *Out00.GetDestAndAdvance();
	//	Transform.M[0][1] = *Out01.GetDestAndAdvance();
	//	Transform.M[0][2] = *Out02.GetDestAndAdvance();
	//	Transform.M[0][3] = *Out03.GetDestAndAdvance();

	//	Transform.M[1][0] = *Out10.GetDestAndAdvance();
	//	Transform.M[1][1] = *Out11.GetDestAndAdvance();
	//	Transform.M[1][2] = *Out12.GetDestAndAdvance();
	//	Transform.M[1][3] = *Out13.GetDestAndAdvance();

	//	Transform.M[2][0] = *Out20.GetDestAndAdvance();
	//	Transform.M[2][1] = *Out21.GetDestAndAdvance();
	//	Transform.M[2][2] = *Out22.GetDestAndAdvance();
	//	Transform.M[2][3] = *Out23.GetDestAndAdvance();

	//	Transform.M[3][0] = *Out30.GetDestAndAdvance();
	//	Transform.M[3][1] = *Out31.GetDestAndAdvance();
	//	Transform.M[3][2] = *Out32.GetDestAndAdvance();
	//	Transform.M[3][3] = *Out33.GetDestAndAdvance();

	//	InstData->WorldTransform = Transform;
	//	InstData->WorldInverse = Transform.Inverse();

	//	*OutTransformStatus.GetDestAndAdvance() = true;
	//}
}

bool UNiagaraDataInterfaceMultipoleGrid::GetFunctionHLSL(
	const FNiagaraDataInterfaceGPUParamInfo& ParamInfo, const FNiagaraDataInterfaceGeneratedFunction& FunctionInfo,
	int FunctionInstanceIndex, FString& OutHLSL)
{
	//FNDIVelocityGridParametersName ParamNames(ParamInfo.DataInterfaceHLSLSymbol);

	// TMap<FString, FStringFormatArg> ArgsSample = {
	//	{TEXT("InstanceFunctionName"), FunctionInfo.InstanceName},
	//	{TEXT("VelocityGridContextName"), TEXT("DIVelocityGrid_MAKE_CONTEXT(") + ParamInfo.DataInterfaceHLSLSymbol + TEXT(")")},
	//};

	// if (FunctionInfo.DefinitionName == BuildVelocityFieldName)
	//{
	//	static const TCHAR* FormatSample = TEXT(R"(
	//			void {InstanceFunctionName} (in float3 GridOrigin, in float GridLength, in float3 ParticlePosition, in float ParticleMass, in
	//float3 ParticleVelocity, in float4x4 VelocityGradient, out bool OutFunctionStatus)
	//			{
	//				{VelocityGridContextName}
	//DIVelocityGrid_BuildVelocityField(DIContext,GridOrigin,GridLength,ParticlePosition,ParticleMass,ParticleVelocity,VelocityGradient,OutFunctionStatus);
	//			}
	//			)");
	//	OutHLSL += FString::Format(FormatSample, ArgsSample);
	//	return true;
	//}
	// else if (FunctionInfo.DefinitionName == SampleVelocityFieldName)
	//{
	//	static const TCHAR* FormatSample = TEXT(R"(
	//			void {InstanceFunctionName} (in float3 GridOrigin, in float GridLength, in float3 ParticlePosition, in bool ScaledVelocity, out
	//float OutParticleMass, out float3 OutParticleVelocity, out float4x4 OutVelocityGradient)
	//			{
	//				{VelocityGridContextName}
	//DIVelocityGrid_SampleVelocityField(DIContext,GridOrigin,GridLength,ParticlePosition,ScaledVelocity,OutParticleMass,OutParticleVelocity,OutVelocityGradient);
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

void UNiagaraDataInterfaceMultipoleGrid::GetCommonHLSL(FString& OutHLSL)
{
	// OutHLSL += TEXT("#include \"/Plugin/Runtime/HairStrands/Private/NiagaraDataInterfaceVelocityGrid.ush\"\n");
}

void UNiagaraDataInterfaceMultipoleGrid::GetParameterDefinitionHLSL(const FNiagaraDataInterfaceGPUParamInfo& ParamInfo, FString& OutHLSL)
{
	OutHLSL += /*TEXT("DIVelocityGrid_DECLARE_CONSTANTS(") +*/ ParamInfo.DataInterfaceHLSLSymbol + TEXT(")\n");
}

void UNiagaraDataInterfaceMultipoleGrid::ProvidePerInstanceDataForRenderThread(
	void* DataForRenderThread, void* PerInstanceData, const FNiagaraSystemInstanceID& SystemInstance)
{
	FNDIMultipoleGridData* GameThreadData = static_cast<FNDIMultipoleGridData*>(PerInstanceData);
	FNDIMultipoleGridData* RenderThreadData = static_cast<FNDIMultipoleGridData*>(DataForRenderThread);

	// RenderThreadData->WorldTransform = GameThreadData->WorldTransform;
	// RenderThreadData->WorldInverse = GameThreadData->WorldInverse;
	// RenderThreadData->CurrentGridBuffer = GameThreadData->CurrentGridBuffer;
	// RenderThreadData->DestinationGridBuffer = GameThreadData->DestinationGridBuffer;
	// RenderThreadData->GridSize = GameThreadData->GridSize;
}

//------------------------------------------------------------------------------------------------------------

// class FCopyMultipoleGridCS : public FGlobalShader
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
// IMPLEMENT_GLOBAL_SHADER(FCopyMultipoleGridCS, "/Plugin/Runtime/HairStrands/Private/NiagaraCopyVelocityGrid.usf", "MainCS", SF_Compute);
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

void FNDIMultipoleGridProxy::ConsumePerInstanceDataFromGameThread(void* PerInstanceData, const FNiagaraSystemInstanceID& Instance)
{
	FNDIMultipoleGridData* SourceData = static_cast<FNDIMultipoleGridData*>(PerInstanceData);
	FNDIMultipoleGridData* TargetData = &(SystemInstancesToProxyData.FindOrAdd(Instance));

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
			LogMultipoleGrid, Log, TEXT("ConsumePerInstanceDataFromGameThread() ... could not find %s"),
			*FNiagaraUtilities::SystemInstanceIDToString(Instance));
	}
}

void FNDIMultipoleGridProxy::PreStage(FRHICommandList& RHICmdList, const FNiagaraDataInterfaceStageArgs& Context)
{
	FNDIMultipoleGridData* ProxyData = SystemInstancesToProxyData.Find(Context.SystemInstanceID);

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

void FNDIMultipoleGridProxy::PostStage(FRHICommandList& RHICmdList, const FNiagaraDataInterfaceStageArgs& Context)
{
	FNDIMultipoleGridData* ProxyData = SystemInstancesToProxyData.Find(Context.SystemInstanceID);

	if (ProxyData != nullptr)
	{
		// ProxyData->Swap();
		// CopyTexture(RHICmdList, ProxyData->DestinationGridBuffer, ProxyData->CurrentGridBuffer, ProxyData->GridSize);
		// FRHICopyTextureInfo CopyInfo;
		// RHICmdList.CopyTexture(ProxyData->DestinationGridBuffer->GridDataBuffer.Buffer,
		//	ProxyData->CurrentGridBuffer->GridDataBuffer.Buffer, CopyInfo);
	}
}

void FNDIMultipoleGridProxy::ResetData(FRHICommandList& RHICmdList, const FNiagaraDataInterfaceArgs& Context)
{
	FNDIMultipoleGridData* ProxyData = SystemInstancesToProxyData.Find(Context.SystemInstanceID);

	// if (ProxyData != nullptr && ProxyData->DestinationGridBuffer != nullptr && ProxyData->CurrentGridBuffer != nullptr)
	//{
	//	ClearBuffer(RHICmdList, ProxyData->DestinationGridBuffer);
	//	ClearBuffer(RHICmdList, ProxyData->CurrentGridBuffer);
	//}
}

// Get the element count for this instance
FIntVector FNDIMultipoleGridProxy::GetElementCount(FNiagaraSystemInstanceID SystemInstanceID) const
{
	if (const FNDIMultipoleGridData* ProxyData = SystemInstancesToProxyData.Find(SystemInstanceID))
	{
		// return FIntVector(ProxyData->GridSize.X + 1, ProxyData->GridSize.Y + 1, ProxyData->GridSize.Z + 1);
	}
	return FIntVector::ZeroValue;
}

#undef LOCTEXT_NAMESPACE
