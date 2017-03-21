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
#include "util/CallbackContainer.hpp"

struct TestMovementComponentData
{
	// If nullptr, TestMovementComponent will create a default actor (for debugging)
	ACharacter* Character;
	AActor* Actor;

	gv::Position WorldPosition;

	gv::Position GoalWorldPosition;
	float GoalManDistanceTolerance = 400.f;
	float MaxSpeed = 500.f;

	// If provided, this entity will be registered in the WorldResourceLocator under this type
	gv::WorldResourceType ResourceType;

	// The last position we told the ResourceLocator we were at (used so that when we move we can
	// find the agent to move in ResourceLocator)
	gv::Position ResourcePosition;
};

class TestMovementComponent : public gv::PooledComponentManager<TestMovementComponentData>
{
private:
	ACharacter* CreateDefaultCharacter(FVector& location, gv::Entity entity);

	UWorld* World;

protected:
	typedef std::vector<gv::PooledComponent<TestMovementComponentData>*>
	    TestMovementComponentRefList;

	virtual void SubscribeEntitiesInternal(const gv::EntityList& subscribers,
	                                       TestMovementComponentRefList& components);
	virtual void UnsubscribeEntitiesInternal(const gv::EntityList& unsubscribers,
	                                         TestMovementComponentRefList& components);

public:
	typedef std::vector<gv::PooledComponent<TestMovementComponentData>> TestMovementComponentList;

	TSubclassOf<ACharacter> DefaultCharacter;

	gv::CallbackContainer<Htn::TaskEventCallback>* TaskEventCallbacks;

	TestMovementComponent();
	virtual ~TestMovementComponent();

	void Initialize(UWorld* newWorld,
	                gv::CallbackContainer<Htn::TaskEventCallback>* taskEventCallbacks);
	virtual void Update(float deltaSeconds);

	// TODO: This should return whether it was actually successful (i.e. the entity exists)
	void PathEntitiesTo(gv::EntityList& entities, std::vector<gv::Position>& positions);
};