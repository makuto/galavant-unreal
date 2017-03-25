// The MIT License (MIT) Copyright (c) 2016 Macoy Madson

#include "GalavantUnreal.h"
#include "GalavantUnrealMain.h"

#include "AgentCharacter.h"

#include "util/Logging.hpp"
#include "plog/Log.h"
#include "GalavantUnrealLog.h"

#include "world/WorldResourceLocator.hpp"
#include "entityComponentSystem/EntityTypes.hpp"
#include "game/agent/Needs.hpp"
#include "ai/htn/HTNTaskDb.hpp"

static GalavantUnrealLog<plog::FuncMessageFormatter> g_GalavantUnrealLogAppender;
static bool s_LogInitialized = false;

// Sets default values
AGalavantUnrealMain::AGalavantUnrealMain()
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if
	// you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	static ConstructorHelpers::FClassFinder<ACharacter> AgentCharacterBPClass(
	    TEXT("Pawn'/Game/Blueprints/AgentCharacter1_Blueprint.AgentCharacter1_Blueprint_C'"));
	if (AgentCharacterBPClass.Class != nullptr)
	{
		TestMovementComponentManager.DefaultCharacter = AgentCharacterBPClass.Class;
	}
	else
	{
		LOGE << "Agent Character Blueprint is NULL!";
		TestMovementComponentManager.DefaultCharacter = AAgentCharacter::StaticClass();
	}

	static ConstructorHelpers::FClassFinder<AActor> FoodPlaceholderBPClass(
	    TEXT("Actor'/Game/Blueprints/FoodPlaceholder_Pickup.FoodPlaceholder_Pickup_C'"));
	if (FoodPlaceholderBPClass.Class != nullptr)
		TestFoodActor = FoodPlaceholderBPClass.Class;
	else
	{
		LOGE << "Food Placeholder Blueprint is NULL!";
		TestFoodActor = AActor::StaticClass();
	}
}

void AGalavantUnrealMain::InitializeEntityTests()
{
	// Create a couple test entities
	int numTestEntities = 20;
	gv::EntityList testEntities;
	testEntities.reserve(numTestEntities);
	EntityComponentSystem.GetNewEntities(testEntities, numTestEntities);

	// Add Movement components to all of them
	{
		TestMovementComponent::TestMovementComponentList newEntityMovementComponents(
		    numTestEntities);

		float spacing = 500.f;
		int i = 0;
		for (gv::EntityListIterator it = testEntities.begin(); it != testEntities.end(); ++it, i++)
		{
			newEntityMovementComponents[i].entity = (*it);
			newEntityMovementComponents[i].data.ResourceType = gv::WorldResourceType::Agent;
			newEntityMovementComponents[i].data.Character = nullptr;
			newEntityMovementComponents[i].data.WorldPosition.Set(0.f, i * spacing, 3600.f);
			newEntityMovementComponents[i].data.GoalManDistanceTolerance = 600.f;
			newEntityMovementComponents[i].data.MaxSpeed = 500.f;
		}

		TestMovementComponentManager.SubscribeEntities(newEntityMovementComponents);
	}

	// Setup agent components for all of them and give them a need
	{
		// Hunger Need
		{
			TestHungerNeed.Name = "Hunger";
			TestHungerNeed.UpdateRate = 10.f;
			TestHungerNeed.AddPerUpdate = 10.f;

			// Hunger Need Triggers
			{
				gv::NeedLevelTrigger lookForFood;
				lookForFood.GreaterThanLevel = true;
				lookForFood.Level = 10.f;
				lookForFood.NeedsResource = true;
				lookForFood.WorldResource = gv::WorldResourceType::Food;
				TestHungerNeed.LevelTriggers.push_back(lookForFood);

				gv::NeedLevelTrigger deathByStarvation;
				deathByStarvation.GreaterThanLevel = true;
				deathByStarvation.Level = 100.f;
				deathByStarvation.DieNow = true;
				TestHungerNeed.LevelTriggers.push_back(deathByStarvation);
			}
		}

		gv::Need hungerNeed = {0};
		hungerNeed.Def = &TestHungerNeed;

		gv::AgentComponentManager::AgentComponentList newAgentComponents(numTestEntities);

		int i = 0;
		for (gv::PooledComponent<gv::AgentComponentData>& currentAgentComponent :
		     newAgentComponents)
		{
			currentAgentComponent.entity = testEntities[i++];
			currentAgentComponent.data.Needs.push_back(hungerNeed);
		}

		AgentComponentManager.SubscribeEntities(newAgentComponents);
	}

	// Test Plan component by adding one to some of them, making their goal finding food
	/* Commented because this is no longer needed - AgentComponentManager will add plans based on
	needs
	{
	    // Task for each agent: find a food
	    Htn::Parameter resourceToFind;
	    resourceToFind.IntValue = gv::WorldResourceType::Food;
	    resourceToFind.Type = Htn::Parameter::ParamType::Int;
	    Htn::ParameterList parameters = {resourceToFind};
	    Htn::TaskCall findAgentCall{testGetResourceTask.GetTask(), parameters};
	    Htn::TaskCallList findAgentTasks = {findAgentCall};

	    gv::PlanComponentManager::PlanComponentList newPlanComponents(numTestEntities);

	    int currentEntityIndex = 0;
	    for (gv::PooledComponent<gv::PlanComponentData>& currentPlanComponent : newPlanComponents)
	    {
	        currentPlanComponent.entity = testEntities[currentEntityIndex++];
	        currentPlanComponent.data.Tasks.insert(currentPlanComponent.data.Tasks.end(),
	                                               findAgentTasks.begin(), findAgentTasks.end());
	    }

	    PlanComponentManager.SubscribeEntities(newPlanComponents);
	}*/

	// Add food
	{
		int numFood = 4;

		gv::EntityList testFoodEntities;
		testFoodEntities.reserve(numFood);
		EntityComponentSystem.GetNewEntities(testFoodEntities, numFood);

		TestMovementComponent::TestMovementComponentList newFood(numFood);
		gv::PickupRefList newPickups;
		newPickups.reserve(numFood);
		InteractComponentManager.CreatePickups(numFood, newPickups);

		float spacing = 2000.f;
		int i = 0;
		for (gv::EntityListIterator it = testFoodEntities.begin(); it != testFoodEntities.end();
		     ++it, i++)
		{
			// Movement component
			{
				FVector location(-2000.f, i * spacing, 3600.f);
				FRotator defaultRotation(0.f, 0.f, 0.f);
				FActorSpawnParameters spawnParams;

				newFood[i].entity = (*it);
				newFood[i].data.WorldPosition.Set(location.X, location.Y, location.Z);
				newFood[i].data.ResourceType = gv::WorldResourceType::Food;

				newFood[i].data.Character = nullptr;
				newFood[i].data.Actor = (AActor*)GetWorld()->SpawnActor<AActor>(
				    TestFoodActor, location, defaultRotation, spawnParams);
			}

			// Pickup component
			{
				newPickups[i]->entity = (*it);
				newPickups[i]->ResourceType = gv::WorldResourceType::Food;
			}
		}

		TestMovementComponentManager.SubscribeEntities(newFood);
	}
}

void AGalavantUnrealMain::InitializeGalavant()
{
	if (!s_LogInitialized)
	{
		plog::init(plog::debug, &g_GalavantUnrealLogAppender);
		s_LogInitialized = true;
		LOGI << "Galavant Log Initialized";
	}

	gv::WorldResourceLocator::ClearResources();

	TestMovementComponentManager.Initialize(GetWorld(), &TaskEventCallbacks);

	{
		PlanComponentManager.Initialize(&WorldStateManager, &TaskEventCallbacks);
		PlanComponentManager.DebugPrint = true;
	}

	{
		AgentComponentManager.Initialize(&PlanComponentManager);
		AgentComponentManager.DebugPrint = true;
	}

	EntityComponentSystem.AddComponentManager(&TestMovementComponentManager);
	EntityComponentSystem.AddComponentManager(&PlanComponentManager);
	EntityComponentSystem.AddComponentManager(&AgentComponentManager);

	// Initialize Tasks
	{
		testFindResourceTask.Initialize();
		testMoveToTask.Initialize(&TestMovementComponentManager);
		testGetResourceTask.Initialize(&testFindResourceTask, &testMoveToTask);

		Htn::TaskDb::ClearAllTasks();

		Htn::TaskDb::AddTask(testFindResourceTask.GetTask(), Htn::TaskName::FindResource);
		Htn::TaskDb::AddTask(testMoveToTask.GetTask(), Htn::TaskName::MoveTo);
		Htn::TaskDb::AddTask(testGetResourceTask.GetTask(), Htn::TaskName::GetResource);
	}

	InitializeEntityTests();

	LOGI << "Galavant Initialized";
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

	AgentComponentManager.Update(DeltaTime);
	PlanComponentManager.Update(DeltaTime);
	TestMovementComponentManager.Update(DeltaTime);

	EntityComponentSystem.DestroyEntitiesPendingDestruction();
}

void AGalavantUnrealMain::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	// This is because Unreal will begin destroying actors that our entities expected to have
	EntityComponentSystem.DestroyAllEntities();
	LOGI << "Destroyed all entities";
}
