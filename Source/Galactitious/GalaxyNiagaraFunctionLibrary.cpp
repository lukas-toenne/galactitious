// Fill out your copyright notice in the Description page of Project Settings.


#include "GalaxyNiagaraFunctionLibrary.h"
#include "ProbabilityCurveFunctionLibrary.h"

#include "Curves/CurveFloat.h"
#include "Curves/CurveLinearColor.h"
#include "NiagaraDataInterfaceCurve.h"
#include "NiagaraParameterCollection.h"

DEFINE_LOG_CATEGORY_STATIC(LogGalaxyNiagara, Log, All);

void UGalaxyNiagaraFunctionLibrary::SetCurveParameter(
	UNiagaraParameterCollectionInstance* NiagaraParameters, const FString& Name, const FRichCurve& Value, bool bOverride)
{
	const FName ParameterName = *NiagaraParameters->Collection->ParameterNameFromFriendlyName(Name);

	typedef UNiagaraDataInterfaceCurve CurveType;
	static const FNiagaraTypeDefinition CurveTypeDef(CurveType::StaticClass());
	const FNiagaraVariable Var(CurveTypeDef, ParameterName);

	if (!ensure(NiagaraParameters != nullptr))
	{
		return;
	}

	CurveType* DataInterface = (CurveType*)NiagaraParameters->GetParameterStore().GetDataInterface(Var);
	if (!ensureMsgf(DataInterface != nullptr, TEXT("Curve parameter %s not found"), *Name))
	{
		return;
	}

	DataInterface->Curve = Value;
	NiagaraParameters->GetParameterStore().SetDataInterface(DataInterface, Var);

	if (bOverride)
	{
		NiagaraParameters->SetOverridesParameter(Var, true);
	}
}

void UGalaxyNiagaraFunctionLibrary::SetFloatParameter(
	UNiagaraParameterCollectionInstance* NiagaraParameters, const FString& Name, float Value, bool bOverride)
{
	const FName ParameterName = *NiagaraParameters->Collection->ParameterNameFromFriendlyName(Name);
	const FNiagaraVariable Var(FNiagaraTypeDefinition::GetFloatDef(), ParameterName);

	NiagaraParameters->GetParameterStore().SetParameterValue(Value, Var, true);

	if (bOverride)
	{
		NiagaraParameters->SetOverridesParameter(Var, true);
	}
}
