// Fill out your copyright notice in the Description page of Project Settings.

#include "GravitySimulationActor.h"

#include "VDB/VDBGridComponent.h"

AGravitySimulationActor::AGravitySimulationActor()
{
	FMMGrid = CreateDefaultSubobject<UVDBGridComponent>(TEXT("FMMGrid"), true);
	SetRootComponent(FMMGrid);
}

void AGravitySimulationActor::BeginPlay()
{
	Super::BeginPlay();
}
