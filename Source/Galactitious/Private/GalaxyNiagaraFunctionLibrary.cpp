// Fill out your copyright notice in the Description page of Project Settings.

#include "GalaxyNiagaraFunctionLibrary.h"

#include "NiagaraComponent.h"
#include "NiagaraDataInterfaceCurve.h"
#include "NiagaraParameterCollection.h"
#include "NiagaraSystemInstance.h"
#include "ProbabilityCurveFunctionLibrary.h"

#include "Curves/CurveFloat.h"
#include "Curves/CurveLinearColor.h"

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

// Utility macros to perform additional update hacks made necessary due to Niagara bugs
#define NIAGARA_UPDATE_HACK_BEGIN(_Collection, _UpdatedParameters)                                   \
	{                                                                                                \
		/** Restart any systems using this collection. */                                            \
		FNiagaraSystemUpdateContext _UpdateContext(_Collection, true);                               \
		/** XXX BUG in UE 4.26: Parameter overrides do not work, have to modify the default instance \
		/*  https://issues.unrealengine.com/issue/UE-97301                                           \
		 */                                                                                          \
		UNiagaraParameterCollectionInstance* _UpdatedParameters = _Collection->GetDefaultInstance();

#if WITH_EDITOR
#define NIAGARA_UPDATE_HACK_END(_Collection)                             \
	/** Push the change to anyone already bound. */                      \
	_Collection->GetDefaultInstance()->GetParameterStore().Tick();       \
	/** XXX this is necessary to update editor windows using old data */ \
	_Collection->OnChangedDelegate.Broadcast();                          \
	}
#else
#define NIAGARA_UPDATE_HACK_END()                                  \
	/** Push the change to anyone already bound. */                \
	_Collection->GetDefaultInstance()->GetParameterStore().Tick(); \
	}
#endif

void UGalaxyNiagaraFunctionLibrary::ApplyParameterCollectionUpdate(
	class UNiagaraParameterCollectionInstance* NiagaraParameters,
	TFunctionRef<void(class UNiagaraParameterCollectionInstance* UpdatedParameters)> ApplyFn)
{
	/** Restart any systems using this collection. */
	FNiagaraSystemUpdateContext UpdateContext(NiagaraParameters->Collection, true);

	/** XXX BUG in UE 4.26: Parameter overrides do not work, have to modify the default instance
	/*  https://issues.unrealengine.com/issue/UE-97301
	 */
	ApplyFn(NiagaraParameters->Collection->GetDefaultInstance());

	/** Push the change to anyone already bound. */
	NiagaraParameters->Collection->GetDefaultInstance()->GetParameterStore().Tick();

#if WITH_EDITOR
	/** XXX this is necessary to update editor windows using old data */ 
	NiagaraParameters->Collection->OnChangedDelegate.Broadcast();                          
#endif
}
