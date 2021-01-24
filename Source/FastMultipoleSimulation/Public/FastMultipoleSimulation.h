// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"

#include "FastMultipoleOpenVDBGuardEnter.h"
#include <openvdb/openvdb.h>
#include <openvdb/points/PointDataGrid.h>
#include "FastMultipoleOpenVDBGuardLeave.h"

#include "FastMultipoleSimulation.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(LogFastMultipole, Log, All);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FFastMultipoleSimulationResetDelegate, UFastMultipoleSimulation*, Simulation);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FFastMultipoleSimulationStepDelegate, UFastMultipoleSimulation*, Simulation);

UCLASS(BlueprintType)
class FASTMULTIPOLESIMULATION_API UFastMultipoleSimulation : public UObject
{
	GENERATED_BODY()

public:
	using PointBuffer = TArray<FVector>;

	using PointDataTreeType = openvdb::tree::Tree<openvdb::tree::RootNode<
		openvdb::tree::InternalNode<openvdb::tree::InternalNode<openvdb::points::PointDataLeafNode<openvdb::PointDataIndex32, 3>, 4>, 5>>>;
	using PointIndexTreeType = openvdb::tree::Tree<openvdb::tree::RootNode<openvdb::tree::InternalNode<openvdb::tree::InternalNode<openvdb::tools::PointIndexLeafNode<openvdb::PointIndex32, 3>, 4>, 5>>>;

	using PointDataGridType = openvdb::Grid<PointDataTreeType>;
	using PointIndexGridType = openvdb::Grid<PointIndexTreeType>;

	UFastMultipoleSimulation();

	void Reset(const TSharedPtr<PointBuffer>& InPositions);
	void Clear();

	TSharedPtr<PointBuffer> GetPositions() const { return Positions; }

	UFUNCTION(BlueprintCallable)
	const TArray<FVector>& GetPositionData() const;

	PointDataGridType::Ptr GetPointDataGrid() const { return PointDataGrid; }

protected:

	void BuildPointGrid();
	void ClearPointGrid();

	// void BuildMomentsGrid();

public:
	UPROPERTY(BlueprintAssignable)
	FFastMultipoleSimulationResetDelegate OnSimulationReset;

	UPROPERTY(BlueprintAssignable)
	FFastMultipoleSimulationStepDelegate OnSimulationStep;

private:
	TSharedPtr<PointBuffer> Positions;

	PointDataGridType::Ptr PointDataGrid;
};
