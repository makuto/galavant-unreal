#pragma once

// Unreal includes
#include "GameFramework/Actor.h"

#include "TestAgent.h"
#include "../TestHtn/TestWorldResourceLocator.hpp"

// Galavant includes
#include "entityComponentSystem/ComponentManager.hpp"
#include "entityComponentSystem/PooledComponentManager.hpp"
#include "world/Position.hpp"
#include "util/SubjectObserver.hpp"
#include "ai/htn/HTNTypes.hpp"

// using namespace gv;

struct TestMovementComponentData
{
	// If nullptr, TestMovementComponent will create a default actor (for debugging)
	AActor* Actor;

	FVector Position;

	gv::Position WorldPosition;
	gv::Position GoalWorldPosition;

	// The last position we told the ResourceLocator we were at (used so that when we move we can
	// find the agent to move in ResourceLocator)
	gv::Position ResourcePosition;
};

class TestMovementComponent : public gv::PooledComponentManager<TestMovementComponentData>,
                              public gv::Subject<Htn::TaskEvent>
{
private:
	ATestAgent* DefaultActor;
	AActor* CreateDefaultActor(FVector& location);

	float GoalPositionTolerance = 100.f;

	TestWorldResourceLocator* ResourceLocator;

protected:
	typedef std::vector<gv::PooledComponent<TestMovementComponentData>*>
	    TestMovementComponentRefList;

	virtual void SubscribeEntitiesInternal(const gv::EntityList& subscribers,
	                                       TestMovementComponentRefList& components);
	virtual void UnsubscribeEntitiesInternal(const gv::EntityList& unsubscribers,
	                                         TestMovementComponentRefList& components);

public:
	typedef std::vector<gv::PooledComponent<TestMovementComponentData>> TestMovementComponentList;

	TestMovementComponent();
	virtual ~TestMovementComponent();

	void Initialize(ATestAgent* defaultTestActor, TestWorldResourceLocator* newResourceLocator);
	virtual void Update(float deltaSeconds);

	// TODO: This should return whether it was actually successful (i.e. the entity exists)
	void PathEntitiesTo(gv::EntityList& entities, std::vector<gv::Position>& positions);
};