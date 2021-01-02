// Fill out your copyright notice in the Description page of Project Settings.

#include "GravitySimulationActor.h"

THIRD_PARTY_INCLUDES_START
#include <openvdb/tools/LevelSetSphere.h>
THIRD_PARTY_INCLUDES_END

const FMassMoments FMassMoments::ZeroMoments = FMassMoments(0.0f);

AGravitySimulationActor::AGravitySimulationActor()
{
}

void AGravitySimulationActor::BeginPlay()
{
	Super::BeginPlay();

	openvdb::initialize();

	// Create a FloatGrid and populate it with a narrow-band
	// signed distance field of a sphere.
	Grid = openvdb::tools::createLevelSetSphere<GridType>(
		/*radius=*/50.0, /*center=*/openvdb::Vec3f(1.5, 2, 3),
		/*voxel size=*/0.5, /*width=*/4.0);
	// Associate some metadata with the grid.
	Grid->insertMeta("radius", openvdb::FloatMetadata(50.0));
	// Name the grid "LevelSetSphere".
	Grid->setName("LevelSetSphere");
}
