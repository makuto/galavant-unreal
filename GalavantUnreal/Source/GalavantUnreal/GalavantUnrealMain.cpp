// The MIT License (MIT) Copyright (c) 2016 Macoy Madson

#include "GalavantUnreal.h"
#include "GalavantUnrealMain.h"

#include "AgentCharacter.h"

#include "util/Logging.hpp"
#include "plog/Log.h"
#include "GalavantUnrealLog.h"

#include "entityComponentSystem/EntityTypes.hpp"
#include "game/agent/Needs.hpp"

static GalavantUnrealLog<plog::FuncMessageFormatter> g_GalavantUnrealLogAppender;
static bool s_LogInitialized = false;

// Sets default values
AGalavantUnrealMain::AGalavantUnrealMain()
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if
	// you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	// TODO https://answers.unrealengine.com/questions/53689/spawn-blueprint-from-c.html
	// /Game/Blueprints/AgentCharacter1_Blueprint
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
}

void AGalavantUnrealMain::InitializeEntityTests()
{
	// Create a couple test entities
	int numTestEntities = 20;
	gv::EntityList testEntities;
	EntityComponentSystem.GetNewEntities(testEntities, numTestEntities);

	// Add Movement components to all of them
	{
		TestMovementComponent::TestMovementComponentList newEntityMovementComponents;
		newEntityMovementComponents.resize(numTestEntities);

		float spacing = 500.f;
		int i = 0;
		for (gv::EntityListIterator it = testEntities.begin(); it != testEntities.end(); ++it, i++)
		{
			newEntityMovementComponents[i].entity = (*it);
			newEntityMovementComponents[i].data.Character = nullptr;
			newEntityMovementComponents[i].data.WorldPosition.Set(0.f, i * spacing, 3600.f);
			newEntityMovementComponents[i].data.GoalManDistanceTolerance = 2300.f;

			// Add a bus stop (this doesn't belong here, it's just for testing)
			if (i % 4 == 0)
			{
				gv::Position busStopPosition(-2000.f, i * spacing, 4000.f);
				ResourceLocator.AddResource(WorldResourceType::BusStop, busStopPosition);
			}
		}

		TestMovementComponentManager.SubscribeEntities(newEntityMovementComponents);
	}

	// Setup agent components for all of them and give them a need
	{
		TestHungerNeed.Name = "Hunger";
		TestHungerNeed.UpdateRate = 10.f;
		TestHungerNeed.AddPerUpdate = 101.f;
		gv::NeedLevelTrigger deathByStarvation;
		deathByStarvation.GreaterThanLevel = true;
		deathByStarvation.Level = 100.f;
		deathByStarvation.DieNow = true;
		TestHungerNeed.LevelTriggers.push_back(deathByStarvation);
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

	// Test Plan component by adding one to some of them, making their goal finding bus stops
	{
		// Task for each agent: find a bus stop
		Htn::Parameter resourceToFind;
		resourceToFind.IntValue = WorldResourceType::BusStop;
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

	TestMovementComponentManager.Initialize(GetWorld(), &ResourceLocator);

	{
		PlanComponentManager.Initialize(&WorldStateManager);
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
		testFindResourceTask.Initialize(&ResourceLocator);
		testMoveToTask.Initialize(&TestMovementComponentManager);
		testGetResourceTask.Initialize(&testFindResourceTask, &testMoveToTask);
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
