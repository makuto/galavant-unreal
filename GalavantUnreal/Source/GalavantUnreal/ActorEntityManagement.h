#pragma once

#include "entityComponentSystem/EntityTypes.hpp"
#include "entityComponentSystem/EntityComponentManager.hpp"

#include <functional>

// This module tracks Actor-Entity pairs and ensures that if an Actor was destroyed the components
// for the associated Entity will know about it. This is to fix the problem where Unreal can destroy
// an Actor without our Entity system knowing about it
namespace ActorEntityManager
{
void Clear();

// Simple setup: if actor is destroyed, destroy entity
void AddActorEntity(AActor* actor, gv::Entity entity);
void DestroyEntitiesWithDestroyedActors(UWorld* world,
                                        gv::EntityComponentManager* entityComponentSystem);

// Advanced setup: if actor is destroyed, call callback
typedef std::function<void(const AActor*)> TrackActorLifetimeCallback;
void TrackActorLifetime(AActor* actor, TrackActorLifetimeCallback callback);
void UpdateNotifyOnActorDestroy(UWorld* world);
}