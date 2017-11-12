// The MIT License (MIT) Copyright (c) 2016 Macoy Madson

#include "GalavantUnreal.h"
#include "GalavantUnrealMain.h"

#include "Characters/GalavantUnrealFPCharacter.h"
#include "Characters/AgentCharacter.h"
#include "ActorEntityManagement.h"
#include "CharacterManager.hpp"

#include "Utilities/GalavantUnrealLog.h"
#include "util/Logging.hpp"

#include "util/StringHashing.hpp"
#include "util/Time.hpp"

#include "world/WorldResourceLocator.hpp"
#include "world/ProceduralWorld.hpp"
#include "entityComponentSystem/EntityTypes.hpp"
#include "entityComponentSystem/EntityComponentManager.hpp"
#include "game/agent/PlanComponentManager.hpp"
#include "game/agent/AgentComponentManager.hpp"
#include "game/agent/combat/CombatComponentManager.hpp"
#include "game/InteractComponentManager.hpp"
#include "game/agent/Needs.hpp"
#include "ai/htn/HTNTasks.hpp"
#include "game/EntityLevelOfDetail.hpp"

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
			    /*TEXT("Pawn'/Game/FirstPersonCPP/Blueprints/"
			         "GalavantUnrealFPCharacterTrueBPFullBody."
			         "GalavantUnrealFPCharacterTrueBPFullBody_C'"));*/
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
		TestHungerNeed.InitialLevel = 70.f;
		TestHungerNeed.MaxLevel = 300.f;
		TestHungerNeed.MinLevel = 0.f;
		TestHungerNeed.UpdateRate = 10.f;
		TestHungerNeed.AddPerUpdate = 10.f;

		// Find food goal def
		{
			Htn::Parameter resourceToFind;
			resourceToFind.IntValue = gv::WorldResourceType::Food;
			resourceToFind.Type = Htn::Parameter::ParamType::Int;
			Htn::ParameterList parameters = {resourceToFind};
			Htn::TaskCall getResourceCall{Htn::g_TaskDictionary.GetResource(RESKEY("GetResource")),
			                              parameters};
			Htn::TaskCall pickupResourceCall{
			    Htn::g_TaskDictionary.GetResource(RESKEY("InteractPickup")), parameters};
			Htn::TaskCallList getResourceTasks = {getResourceCall, pickupResourceCall};
			static gv::AgentGoalDef s_findFoodGoalDef{gv::AgentGoalDef::GoalType::HtnPlan,
			                                          /*NumRetriesIfFailed=*/2, getResourceTasks};
			gv::g_AgentGoalDefDictionary.AddResource(RESKEY("FindFood"), &s_findFoodGoalDef);
		}

		// Hunger Need Triggers
		{
			gv::NeedLevelTrigger lookForFood;
			lookForFood.Condition = gv::NeedLevelTrigger::ConditionType::GreaterThanLevel;
			lookForFood.Level = 100.f;
			lookForFood.NeedsResource = true;
			lookForFood.WorldResource = gv::WorldResourceType::Food;
			TestHungerNeed.LevelTriggers.push_back(lookForFood);

			gv::NeedLevelTrigger desperateLookForFood;
			desperateLookForFood.Condition = gv::NeedLevelTrigger::ConditionType::GreaterThanLevel;
			desperateLookForFood.Level = 200.f;
			desperateLookForFood.GoalDef =
			    gv::g_AgentGoalDefDictionary.GetResource(RESKEY("FindFood"));
			TestHungerNeed.LevelTriggers.push_back(desperateLookForFood);

			gv::NeedLevelTrigger deathByStarvation;
			deathByStarvation.Condition = gv::NeedLevelTrigger::ConditionType::GreaterThanLevel;
			deathByStarvation.Level = 290.f;
			deathByStarvation.SetConsciousState = gv::AgentConsciousState::Dead;
			TestHungerNeed.LevelTriggers.push_back(deathByStarvation);
		}

		gv::g_NeedDefDictionary.AddResource(RESKEY("Hunger"), &TestHungerNeed);
	}

	// Blood Need
	{
		static gv::NeedDef BloodNeed;
		BloodNeed.Type = gv::NeedType::Blood;
		BloodNeed.Name = "Blood";
		BloodNeed.InitialLevel = 100.f;
		BloodNeed.MaxLevel = 100.f;
		BloodNeed.MinLevel = 0.f;
		// Agents will only gain blood over time, but there's a maximum and they start at max
		BloodNeed.UpdateRate = 10.f;
		BloodNeed.AddPerUpdate = 10.f;

		// Blood Need Triggers
		{
			gv::NeedLevelTrigger lowBloodUnconscious;
			lowBloodUnconscious.Condition = gv::NeedLevelTrigger::ConditionType::LessThanLevel;
			lowBloodUnconscious.Level = 20.f;
			lowBloodUnconscious.SetConsciousState = gv::AgentConsciousState::Unconscious;
			BloodNeed.LevelTriggers.push_back(lowBloodUnconscious);

			gv::NeedLevelTrigger deathByBleedingOut;
			deathByBleedingOut.Condition = gv::NeedLevelTrigger::ConditionType::Zero;
			deathByBleedingOut.SetConsciousState = gv::AgentConsciousState::Dead;
			BloodNeed.LevelTriggers.push_back(deathByBleedingOut);
		}

		gv::g_NeedDefDictionary.AddResource(RESKEY("Blood"), &BloodNeed);
	}

	// Goals
	{
		static gv::AgentGoalDef s_getResourceGoalDef{gv::AgentGoalDef::GoalType::GetResource,
		                                             /*NumRetriesIfFailed=*/2};
		gv::g_AgentGoalDefDictionary.AddResource(RESKEY("GetResource"), &s_getResourceGoalDef);
	}

	// CombatActionDefs
	{
		// Punch
		{
			static gv::CombatActionDef s_punchAction;
			s_punchAction.Type = gv::CombatActionDef::CombatActionType::Attack;
			s_punchAction.Duration = .75f;
			s_punchAction.Damage = {RESKEY("Blood"), 10.f, 50.f};
			s_punchAction.Knockback = {1.f, 500.f};

			gv::g_CombatActionDefDictionary.AddResource(RESKEY("Punch"), &s_punchAction);
		}
	}
}

void AGalavantUnrealMain::InitializeEntityTests()
{
	// Create a couple test entities
	int numTestEntities = 20;
	gv::EntityList testEntities;
	testEntities.reserve(numTestEntities);
	gv::g_EntityComponentManager.GetNewEntities(testEntities, numTestEntities);

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

		g_UnrealMovementComponentManager.SubscribeEntities(newEntityMovementComponents);
	}

	// Setup agent components for all of them and give them a need
	{
		gv::Need hungerNeed(RESKEY("Hunger"));
		gv::Need bloodNeed(RESKEY("Blood"));

		// TODO: Will eventually need a thing which creates agents based on a creation def and sets
		// up the needs accordingly

		gv::AgentComponentManager::AgentComponentList newAgentComponents(numTestEntities);

		int i = 0;
		for (gv::PooledComponent<gv::AgentComponentData>& currentAgentComponent :
		     newAgentComponents)
		{
			currentAgentComponent.entity = testEntities[i++];
			currentAgentComponent.data.Needs.push_back(hungerNeed);
			currentAgentComponent.data.Needs.push_back(bloodNeed);
		}

		gv::g_AgentComponentManager.SubscribeEntities(newAgentComponents);
	}

	// Add food
	{
		int numFood = 4;

		gv::EntityList testFoodEntities;
		testFoodEntities.reserve(numFood);
		gv::g_EntityComponentManager.GetNewEntities(testFoodEntities, numFood);

		UnrealMovementComponent::UnrealMovementComponentList newFood(numFood);
		gv::PickupRefList newPickups;
		newPickups.reserve(numFood);
		gv::g_InteractComponentManager.CreatePickups(testFoodEntities, newPickups);

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
			if (i < newPickups.size())
			{
				newPickups[i]->AffectsNeed = gv::NeedType::Hunger;
				newPickups[i]->DestroySelfOnPickup = true;
			}
		}

		g_UnrealMovementComponentManager.SubscribeEntities(newFood);
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

	gv::ResetGameplayTime();

	InitializeProceduralWorld();

	gv::WorldResourceLocator::ClearResources();

	// Initialize Entity Components
	{
		// I originally made this happen via static initialization, but using that in combination
		// with A) split Galavant static library and GalavantUnreal library and B) Unreal
		// Hotreloading caused issues (mainly that Unreal-specific ComponentManagers weren't being
		// registered in the same list)
		gv::g_EntityComponentManager.AddComponentManager(&gv::g_InteractComponentManager);
		gv::g_EntityComponentManager.AddComponentManager(&gv::g_CombatComponentManager);
		gv::g_EntityComponentManager.AddComponentManager(&gv::g_AgentComponentManager);
		gv::g_EntityComponentManager.AddComponentManager(&gv::g_PlanComponentManager);
		gv::g_EntityComponentManager.AddComponentManager(&g_UnrealMovementComponentManager);

		// I've put this here only because of paranoia involving Unreal Play In Editor/hotreloading
		gv::g_EntityComponentManager.ClearUnsubscribeOnlyManagers();

		// Initialize managers
		{
			g_UnrealMovementComponentManager.Initialize(GetWorld(), &TaskEventCallbacks);
			// g_UnrealMovementComponentManager.DebugPrint = true;

			gv::g_PlanComponentManager.Initialize(&WorldStateManager, &TaskEventCallbacks);
			// gv::g_PlanComponentManager.DebugPrint = true;

			gv::g_AgentComponentManager.Initialize(&gv::g_PlanComponentManager,
			                                       &g_UnrealMovementComponentManager);
			gv::g_AgentComponentManager.DebugPrint = true;

			gv::g_CombatComponentManager.Initialize(&CombatFxHandler);
		}
	}

	// Initialize Tasks
	{
		MoveToTask.Initialize(&g_UnrealMovementComponentManager);
		GetResourceTask.Initialize(&FindResourceTask, &MoveToTask);
		InteractPickupTask.Initialize(&gv::g_InteractComponentManager);

		Htn::g_TaskDictionary.AddResource(RESKEY("FindResource"), FindResourceTask.GetTask());
		Htn::g_TaskDictionary.AddResource(RESKEY("MoveTo"), MoveToTask.GetTask());
		Htn::g_TaskDictionary.AddResource(RESKEY("GetResource"), GetResourceTask.GetTask());
		Htn::g_TaskDictionary.AddResource(RESKEY("InteractPickup"), InteractPickupTask.GetTask());
	}

	InitializeResources();

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

	// Destroy entities now because Unreal might have destroyed actors, so we don't want our code to
	// break not knowing that
	gv::g_EntityComponentManager.DestroyEntitiesPendingDestruction();

	if (gv::GameIsPlaying())
	{
		GalavantMain.Update(DeltaTime);

		gv::g_CombatComponentManager.Update(DeltaTime);
		gv::g_AgentComponentManager.Update(DeltaTime);
		gv::g_PlanComponentManager.Update(DeltaTime);
		g_UnrealMovementComponentManager.Update(DeltaTime);
		CharacterManager::Update(DeltaTime);
	}
	else
	{
		//
		// Paused
		//

		// Debug text
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(
			    /*key=*/0, /*timeToDisplay=*/0.2f, FColor::Blue, TEXT("Galavant Paused"));
		}

		// Exception: update UnrealMovementComponentManager because we want LOD etc. to work
		g_UnrealMovementComponentManager.Update(0.f);
	}
}

void AGalavantUnrealMain::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	// This is because Unreal will begin destroying actors that our entities expected to have
	gv::g_EntityComponentManager.DestroyAllEntities();
	LOGI << "Destroyed all entities";
	ActorEntityManager::Clear();
	gv::WorldResourceLocator::ClearResources();
	gv::ResourceDictionaryBase::ClearAllDictionaries();
}

void AGalavantUnrealMain::PauseGalaUpdate(bool pause)
{
	if (pause)
		LOGI << "Pausing Galavant Update";
	else
		LOGI << "Resuming Galavant Update";

	gv::GameSetPlaying(!pause);
}

void AGalavantUnrealMain::HighDPI()
{
	// This is a complete hack and should only be for Macoy's setup
	// Scale everything (including the engine UI) to my DPI settings
	// https://answers.unrealengine.com/questions/247475/why-would-setting-games-screen-resolution-in-gameu.html
	// Note that this breaks during cooking
	FSlateApplication::Get().SetApplicationScale(1.53f);
}

// Latelinked functions
namespace gv
{
// @LatelinkDef
float GetWorldTime()
{
	UWorld* world = GEngine->GetWorld();
	if (!world)
		return 0.f;

	return world->GetTimeSeconds();
}
}