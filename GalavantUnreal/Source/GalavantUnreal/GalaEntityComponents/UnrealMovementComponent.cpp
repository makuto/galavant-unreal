#include "GalavantUnreal.h"

#include "UnrealMovementComponent.hpp"

#include "Characters/AgentCharacter.h"
#include "RandomStream.h"
#include "ActorEntityManagement.h"
#include "Utilities/ConversionHelpers.h"

#include "util/Logging.hpp"

#include "entityComponentSystem/PooledComponentManager.hpp"
#include "entityComponentSystem/EntitySharedData.hpp"

#include "world/WorldResourceLocator.hpp"
#include "world/ProceduralWorld.hpp"
#include "game/EntityLevelOfDetail.hpp"

#include <functional>

#include <iostream>

#include "GalavantUnrealMain.h"

UnrealMovementComponent g_UnrealMovementComponentManager;

UnrealMovementComponent::UnrealMovementComponent()
    : gv::PooledComponentManager<UnrealMovementComponentData>(100)
{
	DebugName = "UnrealMovementComponent";
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
		gv::Position& worldPosition = currentComponent->data.WorldPosition;
		gv::Position& goalWorldPosition = currentComponent->data.GoalWorldPosition;
		float goalManDistanceTolerance = currentComponent->data.GoalManDistanceTolerance;
		FVector trueWorldPosition;
		gv::Position targetPosition;
		bool shouldActorExist = gv::EntityLOD::ShouldRenderForPlayer(worldPosition);
		bool moveActor = false;

		if (currentComponent->data.SpawnParams.CharacterToSpawn)
		{
			struct bullshit
			{
				void OnActorDestroyed(gv::Entity ent)
				{
				}
			};
			std::cout << "Spawning " << &currentComponent->data.SpawnParams.CharacterToSpawn
			          << " in world " << World << "\n";
			gv::Position position;
			bullshit someBullshit;
			/*TSharedPtr<ACharacter> newLockedCharacter(
			    AGalavantUnrealMain::CreateActorForEntity<ACharacter>(
			        World, currentComponent->data.SpawnParams.CharacterToSpawn, 0, position,
			        std::bind(&bullshit::OnActorDestroyed, &someBullshit,
			   std::placeholders::_1)));*/
			ACharacter* newLockedCharacter = AGalavantUnrealMain::CreateActorForEntity<ACharacter>(
			    World, currentComponent->data.SpawnParams.CharacterToSpawn, 0, position,
			    std::bind(&bullshit::OnActorDestroyed, &someBullshit, std::placeholders::_1));
			LOGD << "Spawned character. What the fuck";
		}

		TSharedPtr<AActor> lockedActor(currentComponent->data.Actor.Pin());
		TSharedPtr<ACharacter> lockedCharacter(currentComponent->data.Character.Pin());

		// Debug Actor/Entity lifetime
		if (GEngine)
		{
			bool spawnDiscrepancy =
			    (shouldActorExist && (!lockedActor.IsValid() && !lockedCharacter.IsValid())) ||
			    (!shouldActorExist && (lockedActor.IsValid() || lockedCharacter.IsValid()));
			// Use entity as string key so it'll overwrite
			GEngine->AddOnScreenDebugMessage(
			    /*key=*/(uint64)currentComponent->entity, /*timeToDisplay=*/1.5f,
			    spawnDiscrepancy ? FColor::Red : FColor::Green,
			    FString::Printf(TEXT("Entity %d Actor: %d Character: %d Should Render: %d"),
			                    currentComponent->entity, lockedActor.IsValid(),
			                    lockedCharacter.IsValid(), shouldActorExist));
		}

		SpawnActorIfNecessary(currentComponent);

		if (lockedActor.IsValid())
		{
			trueWorldPosition = lockedActor->GetActorLocation();
			worldPosition = ToPosition(trueWorldPosition);
		}
		else if (lockedCharacter.IsValid())
		{
			trueWorldPosition = lockedCharacter->GetActorLocation();
			worldPosition = ToPosition(trueWorldPosition);
		}
		else
			currentComponent->data.Actor = currentComponent->data.Character = nullptr;

		// Destroy the actor if we are far away
		if (!shouldActorExist && (lockedActor.IsValid() || lockedCharacter.IsValid()))
		{
			DestroyActor(currentComponent);

			LOGD << "Entity " << currentComponent->entity
			     << " destroyed its actor because it shouldn't be rendered to the player";
		}
		else
			moveActor = true;

		if (!moveActor)
			trueWorldPosition = ToFVector(worldPosition);

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
				if (lockedActor.IsValid())
					lockedActor->AddActorLocalOffset(deltaVelocity, false, nullptr,
					                                 ETeleportType::None);
				else if (lockedCharacter.IsValid())
					lockedCharacter->AddMovementInput(deltaVelocity, 1.f);
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

		DestroyActor(currentComponent);

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

	TSharedPtr<AActor> lockedActor(component->data.Actor.Pin());
	TSharedPtr<ACharacter> lockedCharacter(component->data.Character.Pin());

	if (lockedActor.IsValid() || lockedCharacter.IsValid())
		return;

	if (gv::EntityLOD::ShouldRenderForPlayer(component->data.WorldPosition))
	{
		// TODO: Store rotation
		// FRotator defaultRotation(0.f, 0.f, 0.f);
		FVector position(ToFVector(component->data.WorldPosition));
		FActorSpawnParameters spawnParams;

		bool hitWorld = false;
		// Raycast to find the ground to spawn on
		// TODO: Far future: If anything is ever flying, we'll need to not do this
		{
			gv::ProceduralWorld::ProceduralWorldParams& params =
			    gv::ProceduralWorld::GetActiveWorldParams();
			FVector rayStart(position);
			FVector rayEnd(position);
			rayStart.Z = params.WorldCellMaxHeight;
			rayEnd.Z = params.WorldCellMinHeight;
			FHitResult hitResult;
			if (World->LineTraceSingleByChannel(hitResult, rayStart, rayEnd,
			                                    ECollisionChannel::ECC_WorldDynamic))
			{
				position.Z = hitResult.ImpactPoint.Z + 100.f;
				hitWorld = true;
			}
		}

		if (!hitWorld)
		{
			LOGD << "Entity " << component->entity
			     << " will NOT spawn actor/character because it doesn't have a place to stand";
			return;
		}

		LOGD << "Entity " << component->entity
		     << " spawning actor/character because it should still be rendered";

		TWeakPtr<AActor> newActor;
		TWeakPtr<ACharacter> newCharacter;

		if (component->data.SpawnParams.ActorToSpawn)
		{
			TSharedPtr<AActor> newLockedActor(ActorEntityManager::CreateActorForEntity<AActor>(
			    World, component->data.SpawnParams.ActorToSpawn, component->entity,
			    ToPosition(position), std::bind(&UnrealMovementComponent::OnActorDestroyed, this,
			                                    std::placeholders::_1)));
			newActor = newLockedActor;
		}
		else if (component->data.SpawnParams.CharacterToSpawn)
		{
			TSharedPtr<ACharacter> newLockedCharacter(
			    ActorEntityManager::CreateActorForEntity<ACharacter>(
			        World, component->data.SpawnParams.CharacterToSpawn, component->entity,
			        ToPosition(position), std::bind(&UnrealMovementComponent::OnActorDestroyed,
			                                        this, std::placeholders::_1)));
			newCharacter = newLockedCharacter;
		}
		else
		{
			LOGE << "Entity " << component->entity << " couldn't spawn; SpawnParams not set!";
			return;
		}

		if (!newCharacter.IsValid() && !newActor.IsValid())
			LOGE << "Unable to spawn entity " << component->entity << "!";
		else
		{
			component->data.Actor = newActor;
			component->data.Character = newCharacter;
		}
	}
}

void UnrealMovementComponent::DestroyActor(
    gv::PooledComponent<UnrealMovementComponentData>* component)
{
	TSharedPtr<AActor> lockedActor(component->data.Actor.Pin());
	TSharedPtr<ACharacter> lockedCharacter(component->data.Character.Pin());
	if (lockedActor.IsValid())
		lockedActor->Destroy();
	if (lockedCharacter.IsValid())
		lockedCharacter->Destroy();

	component->data.Actor = component->data.Character = nullptr;
}

// @Callback: TrackActorLifetimeCallback
void UnrealMovementComponent::OnActorDestroyed(gv::Entity entity)
{
	gv::PooledComponentManager<UnrealMovementComponentData>::FragmentedPoolIterator it =
	    gv::PooledComponentManager<UnrealMovementComponentData>::NULL_POOL_ITERATOR;
	for (gv::PooledComponent<UnrealMovementComponentData>* currentComponent = ActivePoolBegin(it);
	     currentComponent != nullptr &&
	     it != gv::PooledComponentManager<UnrealMovementComponentData>::NULL_POOL_ITERATOR;
	     currentComponent = GetNextActivePooledComponent(it))
	{
		if (currentComponent->entity == entity)
		{
			if (gv::EntityLOD::ShouldRenderForPlayer(currentComponent->data.WorldPosition))
			{
				LOGD << "Entity " << currentComponent->entity << " had its actor destroyed "
				                                                 "(possibly against its will); it "
				                                                 "is in player view ";
			}
			currentComponent->data.Actor = nullptr;
			currentComponent->data.Character = nullptr;

			// Actors will be respawned during Update() if necessary
			// SpawnActorIfNecessary(currentComponent);
		}
	}
}