// Fill out your copyright notice in the Description page of Project Settings.

#include "FastMultipoleSimulation.h"

#include "FastMultipoleOpenVDBGuardEnter.h"
#include <openvdb/points/PointConversion.h>
#include "FastMultipoleOpenVDBGuardLeave.h"

#define LOCTEXT_NAMESPACE "FastMultipole"
DEFINE_LOG_CATEGORY(LogFastMultipole)

void UFastMultipoleSimulation::SetPositions(const TSharedPtr<PointBuffer>& InPositions)
{
	Positions = InPositions;
}

void UFastMultipoleSimulation::BuildPointGrid()
{
}

void UFastMultipoleSimulation::ClearPointGrid()
{

}

#undef LOCTEXT_NAMESPACE
