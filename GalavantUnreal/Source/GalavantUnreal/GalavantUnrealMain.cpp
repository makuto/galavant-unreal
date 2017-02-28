// The MIT License (MIT) Copyright (c) 2016 Macoy Madson

#include "GalavantUnreal.h"
#include "GalavantUnrealMain.h"

#include "entityComponentSystem/EntityTypes.hpp"

// Sets default values
// TODO: This task shit is ugly :(
AGalavantUnrealMain::AGalavantUnrealMain()
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if
	// you don't need it.
	PrimaryActorTick.bCanEverTick = true;
}

void AGalavantUnrealMain::InitializeGalavant()
{
	// Initialize TestMovementComponent
	{
		FVector location(0.f, 0.f, 0.f);
		FRotator rotation(0.f, 0.f, 0.f);
		FActorSpawnParameters spawnParams;

		// ATestAgent* testAgentTemplate = (ATestAgent*)ATestAgent::StaticClass();
		ATestAgent* testAgent =
		    (ATestAgent*)GetWorld()->SpawnActor<ATestAgent>(location, rotation, spawnParams);

		TestMovementComponentManager.Initialize(testAgent, &ResourceLocator);
	}

	// Initialize PlanComponentManager
	{
		PlanComponentManager.Initialize(&WorldStateManager);
	}

	EntityComponentSystem.AddComponentManager(&TestMovementComponentManager);
	EntityComponentSystem.AddComponentManager(&PlanComponentManager);

	// Initialize Tasks
	{
		testFindResourceTask.Initialize(&ResourceLocator);
		testMoveToTask.Initialize(&TestMovementComponentManager);
		testGetResourceTask.Initialize(&testFindResourceTask, &testMoveToTask);
	}

	// Create a couple test entities
	int numTestEntities = 20;
	gv::EntityList testEntities;
	EntityComponentSystem.GetNewEntities(testEntities, numTestEntities);

	// Add Movement components to all of them
	{
		TestMovementComponent::TestMovementComponentList newEntityMovementComponents;
		newEntityMovementComponents.resize(numTestEntities);

		int i = 0;
		for (gv::EntityListIterator it = testEntities.begin(); it != testEntities.end(); ++it)
		{
			newEntityMovementComponents[i].entity = (*it);
			newEntityMovementComponents[i].data.Actor = nullptr;
			newEntityMovementComponents[i].data.WorldPosition.Set(0.f, i * 500.f, 0.f);
			i++;
		}

		TestMovementComponentManager.SubscribeEntities(newEntityMovementComponents);
	}

	// Test Plan component by adding to half of them, making their goal finding other agents
	{
		// Task for each agent: find another agent
		Htn::Parameter resourceToFind;
		resourceToFind.IntValue = WorldResourceType::Agent;
		Htn::ParameterList parameters = {resourceToFind};
		// TODO: Fuck this static
		static Htn::Task getResourceTask(&testGetResourceTask);
		Htn::TaskCall findAgentCall{&getResourceTask, parameters};
		Htn::TaskCallList findAgentTasks = {findAgentCall};

		gv::PlanComponentManager::PlanComponentList newPlanComponents;
		newPlanComponents.resize(numTestEntities / 2);
		int currentEntityIndex = 0;

		for (gv::PooledComponent<gv::PlanComponentData>& currentPlanComponent : newPlanComponents)
		{
			currentPlanComponent.entity = testEntities[currentEntityIndex];
			currentPlanComponent.data.Tasks.insert(currentPlanComponent.data.Tasks.end(),
			                                       findAgentTasks.begin(), findAgentTasks.end());

			currentEntityIndex += 2;
			if (currentEntityIndex > newPlanComponents.size())
				break;
		}

		PlanComponentManager.SubscribeEntities(newPlanComponents);
	}
}

// Called when the game starts or when spawned
void AGalavantUnrealMain::BeginPlay()
{
	Super::BeginPlay();

	// Should this be here or in the constructor?
	InitializeGalavant();
}

// Called every frame
void AGalavantUnrealMain::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	GalavantMain.Update(DeltaTime);

	PlanComponentManager.Update(DeltaTime);
	TestMovementComponentManager.Update(DeltaTime);

	EntityComponentSystem.DestroyEntitiesPendingDestruction();
}
