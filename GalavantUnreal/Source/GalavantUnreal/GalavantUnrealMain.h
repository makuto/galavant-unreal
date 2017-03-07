// The MIT License (MIT) Copyright (c) 2016 Macoy Madson

#pragma once

#include "GameFramework/Actor.h"

#include "GalavantMain.hpp"

#include "entityComponentSystem/EntityComponentManager.hpp"
#include "game/agent/PlanComponentManager.hpp"

// Testing
#include "GalaEntityComponents/TestMovementComponent.hpp"
#include "TestHtn/TestMovementTasks.hpp"
#include "TestHtn/TestWorldResourceLocator.hpp"

#include "GalavantUnrealMain.generated.h"

UCLASS()
class GALAVANTUNREAL_API AGalavantUnrealMain : public AActor
{
	GENERATED_BODY()

	gv::GalavantMain GalavantMain;

	// Entity Components
	gv::EntityComponentManager EntityComponentSystem;
	TestMovementComponent TestMovementComponentManager;
	gv::PlanComponentManager PlanComponentManager;

	// World
	gv::WorldStateManager WorldStateManager;
	TestWorldResourceLocator ResourceLocator;

	// Hierarchical Task Networks
	// TODO: How should this work?
	TestFindResourceTask testFindResourceTask;
	TestMoveToTask testMoveToTask;
	TestGetResourceTask testGetResourceTask;

	void InitializeGalavant();

public:
	// Sets default values for this actor's properties
	AGalavantUnrealMain();

	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	// Called every frame
	virtual void Tick(float DeltaSeconds) override;

private:
	void InitializeEntityTests();
};
