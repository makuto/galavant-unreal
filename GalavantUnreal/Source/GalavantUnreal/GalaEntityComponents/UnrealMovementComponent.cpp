#include "GalavantUnreal.h"

#include "UnrealMovementComponent.hpp"

#include "Characters/AgentCharacter.h"
#include "RandomStream.h"
#include "ActorEntityManagement.h"
#include "CharacterManager.hpp"
#include "Utilities/ConversionHelpers.h"

#include "util/Logging.hpp"

#include "entityComponentSystem/PooledComponentManager.hpp"
#include "entityComponentSystem/EntitySharedData.hpp"

#include "world/WorldResourceLocator.hpp"
#include "world/ProceduralWorld.hpp"
#include "game/EntityLevelOfDetail.hpp"

#include "util/Math.hpp"

#include <functional>

#include "GalavantUnrealMain.h"

UnrealMovementComponent g_UnrealMovementComponentManager;

UnrealMovementComponent::UnrealMovementComponent()
    : gv::PooledComponentManager<UnrealMovementComponentData>(100)
{
	DebugName = "UnrealMovementComponent";
	DebugPrint = false;
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

		// Debug Actor/Entity lifetime
		if (GEngine)
		{
			bool spawnDiscrepancy =
			    (shouldActorExist && (!currentComponent->data.Actor.IsValid() &&
			                          !currentComponent->data.Character.IsValid())) ||
			    (!shouldActorExist && (currentComponent->data.Actor.IsValid() ||
			                           currentComponent->data.Character.IsValid()));
			// Use entity as string key so it'll overwrite
			GEngine->AddOnScreenDebugMessage(
			    /*key=*/(uint64)currentComponent->entity, /*timeToDisplay=*/1.5f,
			    spawnDiscrepancy ? FColor::Red : FColor::Green,
			    FString::Printf(TEXT("Entity %d Actor: %d Character: %d Should Render: %d"),
			                    currentComponent->entity, currentComponent->data.Actor.IsValid(),
			                    currentComponent->data.Character.IsValid(), shouldActorExist));
		}

		SpawnActorIfNecessary(currentComponent);

		if (currentComponent->data.Actor.IsValid())
		{
			trueWorldPosition = currentComponent->data.Actor->GetActorLocation();
			worldPosition = ToPosition(trueWorldPosition);
		}
		else if (currentComponent->data.Character.IsValid())
		{
			trueWorldPosition = currentComponent->data.Character->GetActorLocation();
			worldPosition = ToPosition(trueWorldPosition);
		}
		else
		{
			currentComponent->data.Actor.Reset();
			currentComponent->data.Character.Reset();
		}

		// Destroy the actor if we are far away
		if (!shouldActorExist &&
		    (currentComponent->data.Actor.IsValid() || currentComponent->data.Character.IsValid()))
		{
			DestroyActor(currentComponent);

			LOGD_IF(DebugPrint)
			    << "Entity " << currentComponent->entity
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
				if (currentComponent->data.Actor.IsValid())
					currentComponent->data.Actor->AddActorLocalOffset(deltaVelocity, false, nullptr,
					                                                  ETeleportType::None);
				else if (currentComponent->data.Character.IsValid())
					currentComponent->data.Character->AddMovementInput(deltaVelocity, 1.f);
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

void UnrealMovementComponent::SetEntitySpeeds(const gv::EntityList& entities,
                                              const std::vector<float>& speeds)
{
	gv::PooledComponentManager<UnrealMovementComponentData>::FragmentedPoolIterator it =
	    gv::PooledComponentManager<UnrealMovementComponentData>::NULL_POOL_ITERATOR;
	for (gv::PooledComponent<UnrealMovementComponentData>* currentComponent = ActivePoolBegin(it);
	     currentComponent != nullptr &&
	     it != gv::PooledComponentManager<UnrealMovementComponentData>::NULL_POOL_ITERATOR;
	     currentComponent = GetNextActivePooledComponent(it))
	{
		for (size_t i = 0; i < MIN(entities.size(), speeds.size()); i++)
		{
			if (currentComponent->entity == entities[i])
				currentComponent->data.MaxSpeed = speeds[i];
		}
	}
}

void UnrealMovementComponent::SpawnActorIfNecessary(
    gv::PooledComponent<UnrealMovementComponentData>* component)
{
	if (!World || !component)
		return;

	if (component->data.Actor.IsValid() || component->data.Character.IsValid())
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
			LOGD_IF(DebugPrint)
			    << "Entity " << component->entity
			    << " will NOT spawn actor/character because it doesn't have a place to stand";
			return;
		}

		LOGD_IF(DebugPrint) << "Entity " << component->entity
		                    << " spawning actor/character because it should still be rendered";

		if (component->data.SpawnParams.ActorToSpawn)
		{
			component->data.Actor = ActorEntityManager::CreateActorForEntity<AActor>(
			    World, component->data.SpawnParams.ActorToSpawn, component->entity,
			    ToPosition(position),
			    std::bind(&UnrealMovementComponent::OnActorDestroyed, this, std::placeholders::_1));
		}
		else if (component->data.SpawnParams.CharacterToSpawn)
		{
			component->data.Character = CharacterManager::CreateCharacterForEntity(
			    World, component->data.SpawnParams.CharacterToSpawn, component->entity,
			    ToPosition(position),
			    std::bind(&UnrealMovementComponent::OnActorDestroyed, this, std::placeholders::_1));
		}
		else
		{
			LOGE << "Entity " << component->entity << " couldn't spawn; SpawnParams not set!";
			return;
		}

		if (!component->data.Actor.IsValid() && !component->data.Character.IsValid())
			LOGE << "Unable to spawn entity " << component->entity << "!";
	}
}

void UnrealMovementComponent::DestroyActor(
    gv::PooledComponent<UnrealMovementComponentData>* component)
{
	if (component->data.Actor.IsValid())
		component->data.Actor->Destroy();
	if (component->data.Character.IsValid())
		component->data.Character->Destroy();

	component->data.Actor.Reset();
	component->data.Character.Reset();
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
				LOGD_IF(DebugPrint) << "Entity " << currentComponent->entity
				                    << " had its actor destroyed "
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