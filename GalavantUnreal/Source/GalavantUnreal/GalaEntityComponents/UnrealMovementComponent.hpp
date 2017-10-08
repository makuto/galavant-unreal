#pragma once

// Unreal includes
#include "GameFramework/Actor.h"

#include "TestAgent.h"

// Galavant includes
#include "entityComponentSystem/EntityTypes.hpp"
#include "entityComponentSystem/ComponentManager.hpp"
#include "entityComponentSystem/PooledComponentManager.hpp"
#include "world/Position.hpp"
#include "world/WorldResourceLocator.hpp"
#include "ai/htn/HTNTypes.hpp"
#include "game/agent/MovementManager.hpp"
#include "util/CallbackContainer.hpp"

// When set, UnrealMovementComponent can spawn the actor automatically, as well as respawn if needed
struct MovementComponentActorSpawnParams
{
	TSubclassOf<AActor> ActorToSpawn;
	TSubclassOf<ACharacter> CharacterToSpawn;
};

struct UnrealMovementComponentData
{
	MovementComponentActorSpawnParams SpawnParams;

	TWeakObjectPtr<ACharacter> Character = nullptr;
	TWeakObjectPtr<AActor> Actor = nullptr;

	gv::Position WorldPosition;

	gv::Position GoalWorldPosition;
	float GoalManDistanceTolerance = 400.f;
	float MaxSpeed = 500.f;

	// If provided, this entity will be registered in the WorldResourceLocator under this type
	gv::WorldResourceType ResourceType = gv::WorldResourceType::None;

	// The last position we told the ResourceLocator we were at (used so that when we move we can
	// find the agent to move in ResourceLocator)
	gv::Position ResourcePosition;
};

class UnrealMovementComponent : public gv::PooledComponentManager<UnrealMovementComponentData>,
                                public gv::MovementManager
{
private:
	void SpawnActorIfNecessary(gv::PooledComponent<UnrealMovementComponentData>* component);
	void DestroyActor(gv::PooledComponent<UnrealMovementComponentData>* component);

	UWorld* World;

protected:
	typedef std::vector<gv::PooledComponent<UnrealMovementComponentData>*>
	    UnrealMovementComponentRefList;

	virtual void SubscribeEntitiesInternal(const gv::EntityList& subscribers,
	                                       UnrealMovementComponentRefList& components);
	virtual void UnsubscribePoolEntitiesInternal(const gv::EntityList& unsubscribers,
	                                             UnrealMovementComponentRefList& components);

public:
	bool DebugPrint;

	typedef std::vector<gv::PooledComponent<UnrealMovementComponentData>>
	    UnrealMovementComponentList;

	TSubclassOf<ACharacter> DefaultCharacter;

	gv::CallbackContainer<Htn::TaskEventCallback>* TaskEventCallbacks;

	UnrealMovementComponent();
	virtual ~UnrealMovementComponent();

	void Initialize(UWorld* newWorld,
	                gv::CallbackContainer<Htn::TaskEventCallback>* taskEventCallbacks);
	virtual void Update(float deltaSeconds);

	// TODO: This should return whether it was actually successful (i.e. the entity exists)
	virtual void PathEntitiesTo(const gv::EntityList& entities, const gv::PositionList& positions);

	void OnActorDestroyed(gv::Entity entity);
};

extern UnrealMovementComponent g_UnrealMovementComponentManager;