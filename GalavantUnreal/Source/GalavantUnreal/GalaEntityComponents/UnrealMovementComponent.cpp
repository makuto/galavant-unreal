#include "GalavantUnreal.h"

#include "UnrealMovementComponent.hpp"

#include "AgentCharacter.h"
#include "RandomStream.h"
#include "ActorEntityManagement.h"
#include "ConversionHelpers.h"

#include "util/Logging.hpp"

#include "entityComponentSystem/PooledComponentManager.hpp"
#include "entityComponentSystem/EntitySharedData.hpp"
#include "entityComponentSystem/ComponentTypes.hpp"

#include "world/WorldResourceLocator.hpp"

#include "game/EntityLevelOfDetail.hpp"

#include <functional>

UnrealMovementComponent::UnrealMovementComponent()
    : gv::PooledComponentManager<UnrealMovementComponentData>(100)
{
	Type = gv::ComponentType::Movement;
}

UnrealMovementComponent::~UnrealMovementComponent()
{
}

void UnrealMovementComponent::Initialize(
    UWorld* newWorld, gv::CallbackContainer<Htn::TaskEventCallback>* taskEventCallbacks)
{
	World = newWorld;
	TaskEventCallbacks = taskEventCallbacks;
}

void UnrealMovementComponent::Update(float deltaSeconds)
{
	static FRandomStream randomStream;

	Htn::TaskEventList eventList;
	gv::EntityList entitiesToUnsubscribe;

	// TODO: Adding true iterator support to pool will drastically help damning this to hell
	gv::PooledComponentManager<UnrealMovementComponentData>::FragmentedPoolIterator it =
	    gv::PooledComponentManager<UnrealMovementComponentData>::NULL_POOL_ITERATOR;
	for (gv::PooledComponent<UnrealMovementComponentData>* currentComponent = ActivePoolBegin(it);
	     currentComponent != nullptr &&
	     it != gv::PooledComponentManager<UnrealMovementComponentData>::NULL_POOL_ITERATOR;
	     currentComponent = GetNextActivePooledComponent(it))
	{
		ACharacter* currentCharacter = currentComponent->data.Character;
		AActor* currentActor = currentComponent->data.Actor;
		gv::Position& worldPosition = currentComponent->data.WorldPosition;
		gv::Position& goalWorldPosition = currentComponent->data.GoalWorldPosition;
		float goalManDistanceTolerance = currentComponent->data.GoalManDistanceTolerance;
		FVector trueWorldPosition;
		gv::Position targetPosition;
		bool shouldActorExist = gv::EntityLOD::ShouldRenderForPlayer(worldPosition);
		bool moveActor = false;

		USceneComponent* sceneComponent = nullptr;

		SpawnActorIfNecessary(currentComponent);

		if (currentActor)
			sceneComponent = currentActor->GetRootComponent();
		else if (currentCharacter)
			sceneComponent = currentCharacter->GetRootComponent();

		if (!sceneComponent && shouldActorExist)
		{
			LOGD << "Entity " << currentComponent->entity << " detected with no SceneComponent; "
			                                                 "was the actor destroyed? It should "
			                                                 "be respawned later...";

			// If the actor/character has been destroyed for some reason, make sure we reset these
			// so it'll be spawned. All actors/characters should have scene components
			currentActor = nullptr;
			currentCharacter = nullptr;
			// entitiesToUnsubscribe.push_back(currentComponent->entity);
			continue;
		}
		else
			moveActor = true;

		// World position is always superceded by the actual Actor position
		if (moveActor && sceneComponent)
		{
			// TODO: This motherfucker is still crashing
			trueWorldPosition = sceneComponent->GetComponentLocation();
			worldPosition = ToPosition(trueWorldPosition);
		}
		else
			trueWorldPosition = ToFVector(worldPosition);

		// Destroy the actor if we are far away
		if (!shouldActorExist && (currentActor || currentCharacter))
		{
			if (currentActor)
				currentActor->Destroy();
			if (currentCharacter)
				currentCharacter->Destroy();

			currentActor = currentCharacter = nullptr;

			moveActor = false;

			LOGD << "Entity " << currentComponent->entity
			     << " destroyed its actor because it shouldn't be rendered to the player";
		}

		// Decide where we're going to go
		{
			if (goalWorldPosition)
			{
				if (worldPosition.ManhattanTo(goalWorldPosition) > goalManDistanceTolerance)
				{
					targetPosition = goalWorldPosition;
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
		if (targetPosition)
		{
			FVector deltaLocation = ToFVector(targetPosition) - trueWorldPosition;
			FVector deltaVelocity = deltaLocation.GetSafeNormal(0.1f);
			deltaVelocity *= currentComponent->data.MaxSpeed * deltaSeconds;
			// Disallow flying
			deltaVelocity[2] = 0;

			if (moveActor)
			{
				if (currentActor)
					sceneComponent->AddLocalOffset(deltaVelocity, false, nullptr,
					                               ETeleportType::None);
				else if (currentCharacter)
					currentCharacter->AddMovementInput(deltaVelocity, 1.f);
			}

			worldPosition += ToPosition(deltaVelocity);
		}
	}

	if (TaskEventCallbacks && !eventList.empty())
	{
		for (gv::CallbackCall<Htn::TaskEventCallback>& callback : TaskEventCallbacks->Callbacks)
			callback.Callback(eventList, callback.UserData);
	}

	if (!entitiesToUnsubscribe.empty())
		UnsubscribeEntities(entitiesToUnsubscribe);
}

void UnrealMovementComponent::SubscribeEntitiesInternal(const gv::EntityList& subscribers,
                                                        UnrealMovementComponentRefList& components)
{
	gv::PositionRefList newPositionRefs;
	newPositionRefs.reserve(subscribers.size());

	for (UnrealMovementComponentRefList::iterator it = components.begin(); it != components.end();
	     ++it)
	{
		gv::PooledComponent<UnrealMovementComponentData>* currentComponent = (*it);

		if (!currentComponent)
			continue;

		SpawnActorIfNecessary(currentComponent);

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

void UnrealMovementComponent::UnsubscribePoolEntitiesInternal(
    const gv::EntityList& unsubscribers, UnrealMovementComponentRefList& components)
{
	for (UnrealMovementComponentRefList::iterator it = components.begin(); it != components.end();
	     ++it)
	{
		gv::PooledComponent<UnrealMovementComponentData>* currentComponent = (*it);
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

void UnrealMovementComponent::PathEntitiesTo(const gv::EntityList& entities,
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

		gv::PooledComponentManager<UnrealMovementComponentData>::FragmentedPoolIterator it =
		    gv::PooledComponentManager<UnrealMovementComponentData>::NULL_POOL_ITERATOR;
		for (gv::PooledComponent<UnrealMovementComponentData>* currentComponent =
		         ActivePoolBegin(it);
		     currentComponent != nullptr &&
		     it != gv::PooledComponentManager<UnrealMovementComponentData>::NULL_POOL_ITERATOR;
		     currentComponent = GetNextActivePooledComponent(it))
		{
			if (currentComponent->entity == entityToMove)
			{
				currentComponent->data.GoalWorldPosition = targetPosition;
			}
		}
	}
}

void UnrealMovementComponent::SpawnActorIfNecessary(
    gv::PooledComponent<UnrealMovementComponentData>* component)
{
	if (!World || !component)
		return;

	if (component->data.Actor || component->data.Character)
		return;

	if (gv::EntityLOD::ShouldRenderForPlayer(component->data.WorldPosition))
	{
		AActor* newActor = nullptr;
		ACharacter* newCharacter = nullptr;

		// TODO: Store rotation
		FRotator defaultRotation(0.f, 0.f, 0.f);
		FVector position(ToFVector(component->data.WorldPosition));
		FActorSpawnParameters spawnParams;

		// TODO: Store last known good position
		if (component->data.SpawnParams.OverrideSpawnZ)
			position.Z = component->data.SpawnParams.OverrideSpawnZ;

		LOGD << "Entity " << component->entity
		     << " spawning actor/character because it should still be rendered";

		if (component->data.SpawnParams.ActorToSpawn)
		{
			newActor = (AActor*)World->SpawnActor<AActor>(component->data.SpawnParams.ActorToSpawn,
			                                              position, defaultRotation, spawnParams);
		}
		else if (component->data.SpawnParams.CharacterToSpawn)
		{
			newCharacter = (ACharacter*)World->SpawnActor<ACharacter>(
			    component->data.SpawnParams.CharacterToSpawn, position, defaultRotation,
			    spawnParams);
		}
		else
		{
			LOGE << "Entity " << component->entity << " couldn't spawn; SpawnParams not set!";
			return;
		}

		if (!newCharacter && !newActor)
			LOGE << "Unable to spawn entity " << component->entity << "!";
		else
		{
			ActorEntityManager::TrackActorLifetime(
			    newCharacter ? newCharacter : newActor,
			    std::bind(&UnrealMovementComponent::OnActorDestroyed, this, std::placeholders::_1));

			component->data.Actor = newActor;
			component->data.Character = newCharacter;
		}
	}
}

// #Callback
void UnrealMovementComponent::OnActorDestroyed(const AActor* actor)
{
	gv::PooledComponentManager<UnrealMovementComponentData>::FragmentedPoolIterator it =
	    gv::PooledComponentManager<UnrealMovementComponentData>::NULL_POOL_ITERATOR;
	for (gv::PooledComponent<UnrealMovementComponentData>* currentComponent = ActivePoolBegin(it);
	     currentComponent != nullptr &&
	     it != gv::PooledComponentManager<UnrealMovementComponentData>::NULL_POOL_ITERATOR;
	     currentComponent = GetNextActivePooledComponent(it))
	{
		if (currentComponent->data.Actor == actor || currentComponent->data.Character == actor)
		{
			LOGD << "Entity " << currentComponent->entity << " had its actor " << actor
			     << " destroyed (possibly against its will)";
			currentComponent->data.Actor = nullptr;
			currentComponent->data.Character = nullptr;

			SpawnActorIfNecessary(currentComponent);
		}
	}
}