// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"

#include "FastMultipoleOpenVDBGuardEnter.h"
#include <openvdb/openvdb.h>
#include <openvdb/points/PointDataGrid.h>
#include "FastMultipoleOpenVDBGuardLeave.h"

namespace FastMultipole
{
	using PointDataTreeType = openvdb::tree::Tree<openvdb::tree::RootNode<
		openvdb::tree::InternalNode<openvdb::tree::InternalNode<openvdb::points::PointDataLeafNode<openvdb::PointDataIndex32, 3>, 4>, 5>>>;
	using PointIndexTreeType = openvdb::tree::Tree<openvdb::tree::RootNode<
		openvdb::tree::InternalNode<openvdb::tree::InternalNode<openvdb::tools::PointIndexLeafNode<openvdb::PointIndex32, 3>, 4>, 5>>>;

	using PointDataGridType = openvdb::Grid<PointDataTreeType>;
	using PointIndexGridType = openvdb::Grid<PointIndexTreeType>;
} // namespace FastMultipole
