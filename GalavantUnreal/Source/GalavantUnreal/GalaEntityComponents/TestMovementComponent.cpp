#include "GalavantUnreal.h"

#include "TestMovementComponent.hpp"

#include "AgentCharacter.h"
#include "RandomStream.h"
#include "ActorEntityManagement.h"

#include "util/Logging.hpp"

#include "entityComponentSystem/PooledComponentManager.hpp"
#include "entityComponentSystem/EntitySharedData.hpp"
#include "entityComponentSystem/ComponentTypes.hpp"

#include "world/WorldResourceLocator.hpp"

TestMovementComponent::TestMovementComponent()
    : gv::PooledComponentManager<TestMovementComponentData>(100)
{
	Type = gv::ComponentType::Movement;
}

TestMovementComponent::~TestMovementComponent()
{
}

void TestMovementComponent::Initialize(
    UWorld* newWorld, gv::CallbackContainer<Htn::TaskEventCallback>* taskEventCallbacks)
{
	World = newWorld;
	TaskEventCallbacks = taskEventCallbacks;
}

ACharacter* TestMovementComponent::CreateDefaultCharacter(FVector& location, gv::Entity entity)
{
	if (!World)
		return nullptr;

	FRotator defaultRotation(0.f, 0.f, 0.f);
	FActorSpawnParameters spawnParams;

	AAgentCharacter* newAgentCharacter = (AAgentCharacter*)World->SpawnActor<AAgentCharacter>(
	    DefaultCharacter, location, defaultRotation, spawnParams);

	if (!newAgentCharacter)
		LOG_ERROR << "Unable to spawn agent character!";
	else
	{
		newAgentCharacter->Entity = entity;

		ActorEntityManager::AddActorEntity(newAgentCharacter, entity);
	}

	return newAgentCharacter;
}

void TestMovementComponent::Update(float deltaSeconds)
{
	static FRandomStream randomStream;

	Htn::TaskEventList eventList;

	// TODO: Adding true iterator support to pool will drastically help damning this to hell
	gv::PooledComponentManager<TestMovementComponentData>::FragmentedPoolIterator it =
	    gv::PooledComponentManager<TestMovementComponentData>::NULL_POOL_ITERATOR;
	for (gv::PooledComponent<TestMovementComponentData>* currentComponent = ActivePoolBegin(it);
	     currentComponent != nullptr &&
	     it != gv::PooledComponentManager<TestMovementComponentData>::NULL_POOL_ITERATOR;
	     currentComponent = GetNextActivePooledComponent(it))
	{
		ACharacter* currentCharacter = currentComponent->data.Character;
		AActor* currentActor = currentComponent->data.Actor;
		gv::Position& worldPosition = currentComponent->data.WorldPosition;
		gv::Position& goalWorldPosition = currentComponent->data.GoalWorldPosition;
		float goalManDistanceTolerance = currentComponent->data.GoalManDistanceTolerance;
		FVector trueWorldPosition;
		FVector targetPosition(0.f, 0.f, 0.f);

		USceneComponent* sceneComponent = nullptr;

		if (currentActor)
			sceneComponent = currentActor->GetRootComponent();
		else if (currentCharacter)
			sceneComponent = currentCharacter->GetRootComponent();

		if (!sceneComponent)
			continue;

		trueWorldPosition = sceneComponent->GetComponentLocation();
		worldPosition.Set(trueWorldPosition.X, trueWorldPosition.Y, trueWorldPosition.Z);

		// Decide where we're going to go
		{
			if (goalWorldPosition)
			{
				if (worldPosition.ManhattanTo(goalWorldPosition) > goalManDistanceTolerance)
				{
					targetPosition =
					    FVector(goalWorldPosition.X, goalWorldPosition.Y, goalWorldPosition.Z);
				}
				else
				{
					goalWorldPosition.Reset();

					Htn::TaskEvent goalPositionReachedEvent{
					    Htn::TaskEvent::TaskResult::TaskSucceeded, currentComponent->entity};
					eventList.push_back(goalPositionReachedEvent);
				}
			}
			// If the component Position doesn't match the actor's actual position, have the actor
			//  move towards the component Position. Otherwise, pick a random new target (show
			//  you're
			//  alive). Only random walk if no goal position
			/*else if (trueWorldPosition.Equals(currentPosition, 100.f))
			{
			    float randomRange = 250.f;
			    currentPosition += FVector(randomStream.FRandRange(-randomRange, randomRange),
			                               randomStream.FRandRange(-randomRange, randomRange),
			                               randomStream.FRandRange(-randomRange, randomRange));
			}*/
		}

		if (currentComponent->data.ResourceType != gv::WorldResourceType::None &&
		    !currentComponent->data.ResourcePosition.Equals(worldPosition, 100.f))
		{
			// Give ResourceLocator our new position
			gv::WorldResourceLocator::MoveResource(
			    currentComponent->data.ResourceType, currentComponent->entity,
			    currentComponent->data.ResourcePosition, worldPosition);
			currentComponent->data.ResourcePosition = worldPosition;
		}

		// Perform movement
		if (!targetPosition.IsZero())
		{
			FVector deltaLocation = targetPosition - trueWorldPosition;
			FVector deltaVelocity = deltaLocation.GetSafeNormal(0.1f);
			deltaVelocity *= currentComponent->data.MaxSpeed * deltaSeconds;
			// Disallow flying
			deltaVelocity[2] = 0;

			if (currentActor)
				sceneComponent->AddLocalOffset(deltaVelocity, false, nullptr, ETeleportType::None);
			else if (currentCharacter)
				currentCharacter->AddMovementInput(deltaVelocity, 1.f);
		}
	}

	if (TaskEventCallbacks)
	{
		for (gv::CallbackCall<Htn::TaskEventCallback>& callback : TaskEventCallbacks->Callbacks)
			callback.Callback(eventList, callback.UserData);
	}
}

void TestMovementComponent::SubscribeEntitiesInternal(const gv::EntityList& subscribers,
                                                      TestMovementComponentRefList& components)
{
	gv::PositionRefList newPositionRefs;
	newPositionRefs.reserve(subscribers.size());

	for (TestMovementComponentRefList::iterator it = components.begin(); it != components.end();
	     ++it)
	{
		gv::PooledComponent<TestMovementComponentData>* currentComponent = (*it);

		if (!currentComponent)
			continue;

		if (!currentComponent->data.Character && !currentComponent->data.Actor)
		{
			FVector newActorLocation(currentComponent->data.WorldPosition.X,
			                         currentComponent->data.WorldPosition.Y,
			                         currentComponent->data.WorldPosition.Z);
			currentComponent->data.Character =
			    CreateDefaultCharacter(newActorLocation, currentComponent->entity);
		}

		if (currentComponent->data.ResourceType != gv::WorldResourceType::None)
		{
			gv::WorldResourceLocator::AddResource(currentComponent->data.ResourceType,
			                                      currentComponent->entity,
			                                      currentComponent->data.WorldPosition);
			currentComponent->data.ResourcePosition = currentComponent->data.WorldPosition;
		}
		newPositionRefs.push_back(&currentComponent->data.WorldPosition);
	}

	gv::EntityCreatePositions(subscribers, newPositionRefs);
}

void TestMovementComponent::UnsubscribePoolEntitiesInternal(
    const gv::EntityList& unsubscribers, TestMovementComponentRefList& components)
{
	for (TestMovementComponentRefList::iterator it = components.begin(); it != components.end();
	     ++it)
	{
		gv::PooledComponent<TestMovementComponentData>* currentComponent = (*it);
		if (!currentComponent)
			continue;

		if (currentComponent->data.Character)
			currentComponent->data.Character->Destroy();
		if (currentComponent->data.Actor)
			currentComponent->data.Actor->Destroy();

		if (currentComponent->data.ResourceType != gv::WorldResourceType::None)
		{
			gv::WorldResourceLocator::RemoveResource(currentComponent->data.ResourceType,
			                                         currentComponent->entity,
			                                         currentComponent->data.ResourcePosition);
		}
	}

	// TODO: We assume we own the positions of these entities. This could be dangerous later on
	gv::EntityDestroyPositions(unsubscribers);
}

void TestMovementComponent::PathEntitiesTo(const gv::EntityList& entities,
                                           const gv::PositionList& positions)
{
	const gv::EntityList& subscribers = GetSubscriberList();

	if (entities.empty() || entities.size() != positions.size())
		return;

	for (int i = 0; i < entities.size(); i++)
	{
		const gv::Entity& entityToMove = entities[i];
		const gv::Position& targetPosition = positions[i];

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