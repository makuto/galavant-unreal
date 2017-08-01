// The MIT License (MIT) Copyright (c) 2016 Macoy Madson

#include "GalavantUnreal.h"
#include "GalavantUnrealMain.h"

#include "Characters/GalavantUnrealFPCharacter.h"
#include "Characters/AgentCharacter.h"
#include "ActorEntityManagement.h"

#include "Utilities/GalavantUnrealLog.h"
#include "util/Logging.hpp"

#include "world/WorldResourceLocator.hpp"
#include "world/ProceduralWorld.hpp"
#include "entityComponentSystem/EntityTypes.hpp"
#include "game/agent/Needs.hpp"
#include "game/EntityLevelOfDetail.hpp"
#include "ai/htn/HTNTaskDb.hpp"
#include "util/StringHashing.hpp"

static gv::Logging::Logger s_UnrealLogger(gv::Logging::Severity::debug, &UnrealLogOutput);

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
		// Player
		{
			// Set default player class to our Blueprinted character
			static ConstructorHelpers::FClassFinder<APawn> PlayerPawnBPClass(
			    /*TEXT("Pawn'/Game/Blueprints/"
			         "GalavantUnrealFPPlayerCharacter.GalavantUnrealFPPlayerCharacter_C'"));*/
			    TEXT("Pawn'/Game/FirstPersonCPP/Blueprints/"
			         "GalavantUnrealFPCharacterTrueBP.GalavantUnrealFPCharacterTrueBP_C'"));
			if (PlayerPawnBPClass.Class)
			{
				DefaultPawnClass = PlayerPawnBPClass.Class;
			}
			else
			{
				LOGE << "Player Pawn is null!";
				DefaultPawnClass = AGalavantUnrealFPCharacter::StaticClass();
			}
		}

		// Agent
		{
			static ConstructorHelpers::FClassFinder<ACharacter> AgentCharacterBPClass(TEXT(
			    "Pawn'/Game/Blueprints/AgentCharacter1_Blueprint.AgentCharacter1_Blueprint_C'"));
			if (AgentCharacterBPClass.Class != nullptr)
			{
				DefaultAgentCharacter = AgentCharacterBPClass.Class;
			}
			else
			{
				LOGE << "Agent Character Blueprint is NULL!";
				DefaultAgentCharacter = AAgentCharacter::StaticClass();
			}
		}

		// Food
		{
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
	}

	// Set defaults for properties
	{
		Seed = 5138008;
		TileScale = 120.f;
		NoiseScale = 0.00001f;
		TestEntityCreationZ = 50.f;
		PlayerManhattanViewDistance = 10000.f;
	}

	InitializeProceduralWorld();
}

void InitializeResources()
{
	// Hunger Need
	{
		static gv::NeedDef TestHungerNeed;
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

		gv::g_NeedDefDictionary.AddResource(RESKEY("Hunger"), &TestHungerNeed);
	}

	{
		static gv::AgentGoalDef s_getResourceGoalDef{gv::AgentGoalDef::GoalType::GetResource,
		                                             /*NumRetriesIfFailed=*/2};
		gv::g_AgentGoalDefDictionary.AddResource(RESKEY("GetResource"), &s_getResourceGoalDef);
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
			newEntityMovementComponents[i].data.WorldPosition.Set(0.f, i * spacing,
			                                                      TestEntityCreationZ);
			newEntityMovementComponents[i].data.GoalManDistanceTolerance = 600.f;
			newEntityMovementComponents[i].data.MaxSpeed = 500.f;
		}

		UnrealMovementComponentManager.SubscribeEntities(newEntityMovementComponents);
	}

	// Setup agent components for all of them and give them a need
	{
		gv::Need hungerNeed;
		hungerNeed.Def = gv::g_NeedDefDictionary.GetResource(RESKEY("Hunger"));

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
	    gv::ProceduralWorld::GetActiveWorldParams();

	Params.WorldCellMaxHeight = 200.f;
	Params.WorldCellMinHeight = -10000.f;

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
	LOGI << "Initializing Galavant...";

	InitializeResources();

	InitializeProceduralWorld();

	gv::WorldResourceLocator::ClearResources();

	// Initialize Entity Components
	{
		UnrealMovementComponentManager.Initialize(GetWorld(), &TaskEventCallbacks);

		{
			PlanComponentManager.Initialize(&WorldStateManager, &TaskEventCallbacks);
			// PlanComponentManager.DebugPrint = true;
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

void AGalavantUnrealMain::InitGame(const FString& MapName, const FString& Options,
                                   FString& ErrorMessage)
{
	Super::InitGame(MapName, Options, ErrorMessage);

	// Set world settings. I'm not sure if this is the proper place for this, but it works
	UWorld* world = GetWorld();
	if (world)
	{
		AWorldSettings* worldSettings = world->GetWorldSettings(true);
		worldSettings->KillZ = gv::ProceduralWorld::GetActiveWorldParams().WorldCellMinHeight;
	}

	// It's important that this is in InitGame as opposed to something like BeginPlay() because it
	// needs to be setup before any Actors run BeginPlay()
	InitializeGalavant();
}

// Called when the game starts or when spawned
void AGalavantUnrealMain::BeginPlay()
{
	Super::BeginPlay();
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
	gv::g_NeedDefDictionary.ClearResources();
	gv::g_AgentGoalDefDictionary.ClearResources();
}
