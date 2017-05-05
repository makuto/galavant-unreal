// The MIT License (MIT) Copyright (c) 2016 Macoy Madson

#pragma once

#include "GameFramework/Actor.h"

#include "GalavantMain.hpp"

#include "entityComponentSystem/EntityComponentManager.hpp"
#include "game/agent/PlanComponentManager.hpp"
#include "game/agent/AgentComponentManager.hpp"
#include "game/InteractComponentManager.hpp"
#include "game/agent/Needs.hpp"
#include "game/agent/htnTasks/MovementTasks.hpp"
#include "game/agent/htnTasks/InteractTasks.hpp"
#include "util/CallbackContainer.hpp"

// Testing
#include "GalaEntityComponents/TestMovementComponent.hpp"

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
	gv::AgentComponentManager AgentComponentManager;
	gv::InteractComponentManager InteractComponentManager;

	// World
	gv::WorldStateManager WorldStateManager;

	// Hierarchical Task Networks
	// TODO: How should this work?
	gv::FindResourceTask FindResourceTask;
	gv::MoveToTask MoveToTask;
	gv::GetResourceTask GetResourceTask;
	gv::InteractPickupTask InteractPickupTask;
	gv::CallbackContainer<Htn::TaskEventCallback> TaskEventCallbacks;

	gv::NeedDef TestHungerNeed;

	TSubclassOf<AActor> TestFoodActor;

	void InitializeGalavant();

public:
	// Sets default values for this actor's properties
	AGalavantUnrealMain();

	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	// Called every frame
	virtual void Tick(float DeltaSeconds) override;

	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	void InitializeEntityTests();
};
