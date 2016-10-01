#ifndef TESTMOVEMENTCOMPONENT_H__
#define TESTMOVEMENTCOMPONENT_H__

// Unreal includes
#include "GameFramework/Actor.h"

#include "TestAgent.h"

// Galavant includes
#include "entityComponentSystem/ComponentManager.hpp"
#include "entityComponentSystem/PooledComponentManager.hpp"

struct TestMovementComponentData
{
	// If nullptr, TestMovementComponent will create a default actor (for debugging)
	AActor* Actor;

	FVector Position;
};

class TestMovementComponent : public PooledComponentManager<TestMovementComponentData>
{
private:
	EntityList Subscribers;

	ATestAgent* DefaultActor;
	AActor* CreateDefaultActor(FVector& location);

protected:
	typedef std::vector<PooledComponent<TestMovementComponentData>*> TestMovementComponentRefList;

	virtual void SubscribeEntitiesInternal(TestMovementComponentRefList& components);
	virtual void UnsubscribeEntitiesInternal(TestMovementComponentRefList& components);

public:
	typedef std::vector<PooledComponent<TestMovementComponentData> > TestMovementComponentList;

	TestMovementComponent(void);
	virtual ~TestMovementComponent(void);
	void Initialize(ATestAgent* defaultTestActor);
	virtual void Update(float deltaSeconds);
};

#endif /* end of include guard: TESTMOVEMENTCOMPONENT_H__ */