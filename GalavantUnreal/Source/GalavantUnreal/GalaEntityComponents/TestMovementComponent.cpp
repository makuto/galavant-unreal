#include "GalavantUnreal.h"

#include "TestMovementComponent.hpp"

#include "AgentCharacter.h"
#include "RandomStream.h"

#include "util/Logging.hpp"
#include "entityComponentSystem/PooledComponentManager.hpp"
#include "entityComponentSystem/EntitySharedData.hpp"

TestMovementComponent::TestMovementComponent()
    : gv::PooledComponentManager<TestMovementComponentData>(100)
{
}

TestMovementComponent::~TestMovementComponent()
{
}

void TestMovementComponent::Initialize(UWorld* newWorld,
                                       TestWorldResourceLocator* newResourceLocator)
{
	World = newWorld;
	ResourceLocator = newResourceLocator;
}

ACharacter* TestMovementComponent::CreateDefaultCharacter(FVector& location)
{
	if (!World)
		return nullptr;

	FRotator defaultRotation(0.f, 0.f, 0.f);
	FActorSpawnParameters spawnParams;

	AAgentCharacter* newAgentCharacter = (AAgentCharacter*)World->SpawnActor<AAgentCharacter>(
	    DefaultCharacter, location, defaultRotation, spawnParams);

	if (!newAgentCharacter)
		LOG_ERROR << "Unable to spawn agent character!";

	return newAgentCharacter;
}

void TestMovementComponent::Update(float deltaSeconds)
{
	const float SPEED = 500.f;
	static FRandomStream randomStream;

	std::vector<Htn::TaskEvent> eventQueue;

	// TODO: Adding true iterator support to pool will drastically help damning this to hell
	gv::PooledComponentManager<TestMovementComponentData>::FragmentedPoolIterator it =
	    gv::PooledComponentManager<TestMovementComponentData>::NULL_POOL_ITERATOR;
	for (gv::PooledComponent<TestMovementComponentData>* currentComponent = ActivePoolBegin(it);
	     currentComponent != nullptr &&
	     it != gv::PooledComponentManager<TestMovementComponentData>::NULL_POOL_ITERATOR;
	     currentComponent = GetNextActivePooledComponent(it))
	{
		ACharacter* currentCharacter = currentComponent->data.Character;
		FVector& currentPosition = currentComponent->data.Position;
		gv::Position& goalWorldPosition = currentComponent->data.GoalWorldPosition;
		float goalManDistanceTolerance = currentComponent->data.GoalManDistanceTolerance;

		USceneComponent* sceneComponent = currentCharacter->GetRootComponent();

		// Hit a segfault where this happened. I jumped onto a group of agents all running into
		// eachother. Weird.
		if (!sceneComponent)
			continue;

		FVector trueWorldPosition = sceneComponent->GetComponentLocation();
		FVector deltaLocation(0.f, 0.f, 0.f);

		if (goalWorldPosition)
		{
			gv::Position currentWorldPosition(trueWorldPosition.X, trueWorldPosition.Y,
			                                  trueWorldPosition.Z);
			if (currentWorldPosition.ManhattanTo(goalWorldPosition) > goalManDistanceTolerance)
			{
				// TODO: This fucking sucks
				currentPosition =
				    FVector(goalWorldPosition.X, goalWorldPosition.Y, goalWorldPosition.Z);
			}
			else
			{
				goalWorldPosition.Reset();

				Htn::TaskEvent goalPositionReachedEvent{Htn::TaskEvent::TaskResult::TaskSucceeded,
				                                        currentComponent->entity};
				eventQueue.push_back(goalPositionReachedEvent);
			}
		}
		// If the component Position doesn't match the actor's actual position, have the actor
		//  move towards the component Position. Otherwise, pick a random new target (show you're
		//  alive). Only random walk if no goal position
		/*else if (trueWorldPosition.Equals(currentPosition, 100.f))
		{
		    float randomRange = 250.f;
		    currentPosition += FVector(randomStream.FRandRange(-randomRange, randomRange),
		                               randomStream.FRandRange(-randomRange, randomRange),
		                               randomStream.FRandRange(-randomRange, randomRange));
		}*/

		// Give ResourceLocator our new position
		if (ResourceLocator)
		{
			gv::Position trueGvWorldPosition(trueWorldPosition.X, trueWorldPosition.Y,
			                                 trueWorldPosition.Z);
			ResourceLocator->MoveResource(WorldResourceType::Agent,
			                              currentComponent->data.ResourcePosition,
			                              trueGvWorldPosition);
			currentComponent->data.ResourcePosition = trueGvWorldPosition;
		}

		deltaLocation = currentPosition - trueWorldPosition;

		FVector deltaVelocity = deltaLocation.GetSafeNormal(0.1f);
		deltaVelocity *= SPEED * deltaSeconds;
		// Disallow flying
		deltaVelocity[2] = 0;

		// sceneComponent->AddLocalOffset(deltaVelocity, false, NULL, ETeleportType::None);
		currentCharacter->AddMovementInput(deltaVelocity, 1.f);
	}

	for (gv::CallbackCall<Htn::TaskEventCallback>& callback : TaskEventCallbacks.Callbacks)
		callback.Callback(eventQueue, callback.UserData);
}

void TestMovementComponent::SubscribeEntitiesInternal(const gv::EntityList& subscribers,
                                                      TestMovementComponentRefList& components)
{
	gv::PositionRefList newPositionRefs;

	for (TestMovementComponentRefList::iterator it = components.begin(); it != components.end();
	     ++it)
	{
		gv::PooledComponent<TestMovementComponentData>* currentComponent = (*it);

		if (!currentComponent)
			continue;

		if (!currentComponent->data.Character)
		{
			FVector newActorLocation(currentComponent->data.WorldPosition.X,
			                         currentComponent->data.WorldPosition.Y,
			                         currentComponent->data.WorldPosition.Z);
			currentComponent->data.Character = CreateDefaultCharacter(newActorLocation);
			currentComponent->data.Position = newActorLocation;
		}

		if (ResourceLocator)
		{
			ResourceLocator->AddResource(WorldResourceType::Agent,
			                             currentComponent->data.WorldPosition);
			currentComponent->data.ResourcePosition = currentComponent->data.WorldPosition;
			newPositionRefs.push_back(&currentComponent->data.WorldPosition);
		}
	}

	gv::EntityCreatePositions(subscribers, newPositionRefs);
}

void TestMovementComponent::UnsubscribeEntitiesInternal(const gv::EntityList& unsubscribers,
                                                        TestMovementComponentRefList& components)
{
	for (TestMovementComponentRefList::iterator it = components.begin(); it != components.end();
	     ++it)
	{
		gv::PooledComponent<TestMovementComponentData>* currentComponent = (*it);
		if (!currentComponent)
			continue;

		if (currentComponent->data.Character)
			currentComponent->data.Character->Destroy();

		if (ResourceLocator)
			ResourceLocator->RemoveResource(WorldResourceType::Agent,
			                                currentComponent->data.ResourcePosition);
	}

	// TODO: We assume we own the positions of these entities. This could be dangerous later on
	gv::EntityDestroyPositions(unsubscribers);
}

void TestMovementComponent::PathEntitiesTo(gv::EntityList& entities,
                                           std::vector<gv::Position>& positions)
{
	const gv::EntityList& subscribers = GetSubscriberList();

	if (entities.empty() || entities.size() != positions.size())
		return;

	for (int i = 0; i < entities.size(); i++)
	{
		gv::Entity& entityToMove = entities[i];
		gv::Position& targetPosition = positions[i];

		// For now, entities which are not subscribed will be tossed out
		if (std::find(subscribers.begin(), subscribers.end(), entityToMove) == subscribers.end())
			continue;

		gv::PooledComponentManager<TestMovementComponentData>::FragmentedPoolIterator it =
		    gv::PooledComponentManager<TestMovementComponentData>::NULL_POOL_ITERATOR;
		for (gv::PooledComponent<TestMovementComponentData>* currentComponent = ActivePoolBegin(it);
		     currentComponent != nullptr &&
		     it != gv::PooledComponentManager<TestMovementComponentData>::NULL_POOL_ITERATOR;
		     currentComponent = GetNextActivePooledComponent(it))
		{
			if (currentComponent->entity == entityToMove)
			{
				currentComponent->data.GoalWorldPosition = targetPosition;
			}
		}
	}
}