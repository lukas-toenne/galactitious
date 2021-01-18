// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"

#include "FastMultipoleOpenVDBGuardEnter.h"
#include <openvdb/openvdb.h>
#include <openvdb/points/PointDataGrid.h>
#include "FastMultipoleOpenVDBGuardLeave.h"

#include "FastMultipoleSimulation.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(LogFastMultipole, Log, All);

UCLASS()
class FASTMULTIPOLESIMULATION_API UFastMultipoleSimulation : public UObject
{
	GENERATED_BODY()

public:
	using PointBuffer = TArray<FVector>;

	using PointTreeType = openvdb::tree::Tree<openvdb::tree::RootNode<
		openvdb::tree::InternalNode<openvdb::tree::InternalNode<openvdb::points::PointDataLeafNode<openvdb::PointDataIndex32, 3>, 4>, 5>>>;
	using PointGridType = openvdb::Grid<PointTreeType>;

	void SetPositions(const TSharedPtr<PointBuffer>& InPositions);
	TSharedPtr<PointBuffer> GetPositions() const { return Positions; }

	PointGridType::Ptr GetPointGrid() const { return PointGrid; }

	void BuildPointGrid();
	void ClearPointGrid();

	// void BuildMomentsGrid();

private:
	TSharedPtr<PointBuffer> Positions;

	PointGridType::Ptr PointGrid;
};
