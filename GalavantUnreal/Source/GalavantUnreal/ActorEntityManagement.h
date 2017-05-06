#pragma once

#include "entityComponentSystem/EntityTypes.hpp"
#include "entityComponentSystem/EntityComponentManager.hpp"

// This module tracks Actor-Entity pairs and ensures that if an Actor was destroyed the associated
// Entity will also get destroyed. This is to fix the problem where Unreal can destroy an Actor
// without our Entity system knowing about it
namespace ActorEntityManager
{
void Clear();
void AddActorEntity(AActor* actor, gv::Entity entity);
void DestroyEntitiesWithDestroyedActors(UWorld* world,
                                        gv::EntityComponentManager* entityComponentSystem);
}