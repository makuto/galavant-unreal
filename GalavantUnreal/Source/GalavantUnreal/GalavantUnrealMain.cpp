// The MIT License (MIT) Copyright (c) 2016 Macoy Madson

#include "GalavantUnreal.h"
#include "GalavantUnrealMain.h"

#include "AgentCharacter.h"
#include "ActorEntityManagement.h"

#include "util/Logging.hpp"
#include "plog/Log.h"
#include "GalavantUnrealLog.h"

#include "world/WorldResourceLocator.hpp"
#include "world/ProceduralWorld.hpp"
#include "entityComponentSystem/EntityTypes.hpp"
#include "game/agent/Needs.hpp"
#include "game/EntityLevelOfDetail.hpp"
#include "ai/htn/HTNTaskDb.hpp"

static GalavantUnrealLog<plog::FuncMessageFormatter> g_GalavantUnrealLogAppender;
static bool s_LogInitialized = false;

// Sets default values
AGalavantUnrealMain::AGalavantUnrealMain()
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if
	// you don't need it.
	PrimaryActorTick.bCanEverTick = true;
	// Sets this function to hipri and all prerequisites recursively (we want this because we need
	// to tick before everything else)
	PrimaryActorTick.SetPriorityIncludingPrerequisites(true);

	// Find classes needed for various things
	{
		static ConstructorHelpers::FClassFinder<ACharacter> AgentCharacterBPClass(
		    TEXT("Pawn'/Game/Blueprints/AgentCharacter1_Blueprint.AgentCharacter1_Blueprint_C'"));
		if (AgentCharacterBPClass.Class != nullptr)
		{
			DefaultAgentCharacter = AgentCharacterBPClass.Class;
		}
		else
		{
			LOGE << "Agent Character Blueprint is NULL!";
			DefaultAgentCharacter = AAgentCharacter::StaticClass();
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

	// Set defaults for properties
	{
		Seed = 5138008;
		TileScale = 120.f;
		NoiseScale = 1.f;
		TestEntityCreationZ = 100.f;
		PlayerManhattanViewDistance = 10000.f;
	}

	InitializeProceduralWorld();
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
		UnrealMovementComponent::UnrealMovementComponentList newEntityMovementComponents(
		    numTestEntities);

		float spacing = 500.f;
		int i = 0;
		for (gv::EntityListIterator it = testEntities.begin(); it != testEntities.end(); ++it, i++)
		{
			newEntityMovementComponents[i].entity = (*it);
			newEntityMovementComponents[i].data.ResourceType = gv::WorldResourceType::Agent;
			newEntityMovementComponents[i].data.SpawnParams.CharacterToSpawn =
			    DefaultAgentCharacter;
			newEntityMovementComponents[i].data.SpawnParams.OverrideSpawnZ = TestEntityCreationZ;
			newEntityMovementComponents[i].data.WorldPosition.Set(0.f, i * spacing,
			                                                      TestEntityCreationZ);
			newEntityMovementComponents[i].data.GoalManDistanceTolerance = 600.f;
			newEntityMovementComponents[i].data.MaxSpeed = 500.f;
		}

		UnrealMovementComponentManager.SubscribeEntities(newEntityMovementComponents);
	}

	// Setup agent components for all of them and give them a need
	{
		// Hunger Need
		{
			TestHungerNeed.Type = gv::NeedType::Hunger;
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
				deathByStarvation.Level = 300.f;
				deathByStarvation.DieNow = true;
				TestHungerNeed.LevelTriggers.push_back(deathByStarvation);
			}
		}

		gv::Need hungerNeed;
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

		UnrealMovementComponent::UnrealMovementComponentList newFood(numFood);
		gv::PickupRefList newPickups;
		newPickups.reserve(numFood);
		InteractComponentManager.CreatePickups(testFoodEntities, newPickups);

		float spacing = 2000.f;
		int i = 0;
		for (gv::EntityListIterator it = testFoodEntities.begin(); it != testFoodEntities.end();
		     ++it, i++)
		{
			// Movement component
			{
				FVector location(-2000.f, i * spacing, TestEntityCreationZ);

				newFood[i].entity = (*it);
				newFood[i].data.WorldPosition.Set(location.X, location.Y, location.Z);
				newFood[i].data.ResourceType = gv::WorldResourceType::Food;

				newFood[i].data.SpawnParams.ActorToSpawn = TestFoodActor;
				newFood[i].data.SpawnParams.OverrideSpawnZ = TestEntityCreationZ;
			}

			// Pickup component
			{
				newPickups[i]->AffectsNeed = gv::NeedType::Hunger;
				newPickups[i]->DestroySelfOnPickup = true;
			}
		}

		UnrealMovementComponentManager.SubscribeEntities(newFood);
	}
}

void AGalavantUnrealMain::InitializeProceduralWorld()
{
	gv::ProceduralWorld::ProceduralWorldParams& Params =
	    gv::ProceduralWorld::GetCurrentActiveWorldParams();

	Params.Seed = Seed;

	for (int i = 0; i < 3; i++)
		Params.WorldCellTileSize[i] = TileScale;

	// Hardcoded noise values because I won't be changing these often
	Params.ScaledNoiseParams.lowBound = 0.f;
	Params.ScaledNoiseParams.highBound = 255.f;
	Params.ScaledNoiseParams.octaves = 10;
	Params.ScaledNoiseParams.scale = NoiseScale;
	Params.ScaledNoiseParams.persistence = 0.55f;
	Params.ScaledNoiseParams.lacunarity = 2.f;
}

void AGalavantUnrealMain::InitializeGalavant()
{
	if (!s_LogInitialized)
	{
		plog::init(plog::debug, &g_GalavantUnrealLogAppender);
		s_LogInitialized = true;
		LOGI << "Galavant Log Initialized";
	}

	InitializeProceduralWorld();

	gv::WorldResourceLocator::ClearResources();

	// Initialize Entity Components
	{
		UnrealMovementComponentManager.Initialize(GetWorld(), &TaskEventCallbacks);

		{
			PlanComponentManager.Initialize(&WorldStateManager, &TaskEventCallbacks);
			PlanComponentManager.DebugPrint = false;
		}

		{
			AgentComponentManager.Initialize(&PlanComponentManager);
			AgentComponentManager.DebugPrint = true;
		}

		EntityComponentSystem.AddComponentManager(&UnrealMovementComponentManager);
		EntityComponentSystem.AddComponentManager(&PlanComponentManager);
		EntityComponentSystem.AddComponentManager(&AgentComponentManager);
		// TODO: Figure out these ComponentManager type shenanigans
		EntityComponentSystem.AddComponentManagerOfType(gv::ComponentType::Interact,
		                                                &InteractComponentManager);
	}

	// Initialize Tasks
	{
		MoveToTask.Initialize(&UnrealMovementComponentManager);
		GetResourceTask.Initialize(&FindResourceTask, &MoveToTask);
		InteractPickupTask.Initialize(&InteractComponentManager);

		// Done to support Unreal hotreloading
		Htn::TaskDb::ClearAllTasks();

		Htn::TaskDb::AddTask(FindResourceTask.GetTask(), Htn::TaskName::FindResource);
		Htn::TaskDb::AddTask(MoveToTask.GetTask(), Htn::TaskName::MoveTo);
		Htn::TaskDb::AddTask(GetResourceTask.GetTask(), Htn::TaskName::GetResource);
		Htn::TaskDb::AddTask(InteractPickupTask.GetTask(), Htn::TaskName::InteractPickup);
	}

	// Initialize test levels
	{
		InitializeEntityTests();
	}

	// Initialize LOD settings
	{
		gv::EntityLOD::g_EntityLODSettings.PlayerManhattanViewDistance =
		    PlayerManhattanViewDistance;
	}

	LOGI << "Galavant Initialized";
}

#if WITH_EDITOR
void AGalavantUnrealMain::PostEditChangeProperty(struct FPropertyChangedEvent& e)
{
	InitializeProceduralWorld();

	Super::PostEditChangeProperty(e);
}
#endif

// Called when the game starts or when spawned
void AGalavantUnrealMain::BeginPlay()
{
	Super::BeginPlay();

	InitializeGalavant();
}

// Called every frame
void AGalavantUnrealMain::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	UWorld* world = GetWorld();

	// Make sure the Entity Component System knows if an Actor associated with an Entity has been
	// destroyed. Component Managers should be able to trust that their Subscribers have valid data.
	// This wouldn't be needed if Unreal would stop killing our Actors for strange reasons or if I
	// added a ECS hookup to the AActor base class
	ActorEntityManager::UpdateNotifyOnActorDestroy(world);
	ActorEntityManager::DestroyEntitiesWithDestroyedActors(world, &EntityComponentSystem);

	// Destroy entities now because Unreal might have destroyed actors, so we don't want our code to
	// break not knowing that
	EntityComponentSystem.DestroyEntitiesPendingDestruction();

	GalavantMain.Update(DeltaTime);

	AgentComponentManager.Update(DeltaTime);
	PlanComponentManager.Update(DeltaTime);
	UnrealMovementComponentManager.Update(DeltaTime);
}

void AGalavantUnrealMain::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	// This is because Unreal will begin destroying actors that our entities expected to have
	EntityComponentSystem.DestroyAllEntities();
	LOGI << "Destroyed all entities";
	ActorEntityManager::Clear();
	gv::WorldResourceLocator::ClearResources();
}
