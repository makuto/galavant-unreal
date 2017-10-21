#pragma once

#include "ActorEntityManagement.h"

#include "entityComponentSystem/EntityTypes.hpp"
#include "entityComponentSystem/EntityComponentManager.hpp"
#include "world/Position.hpp"

// Character Manager - Handle Unreal Characters (movement, animation, ragdoll, etc.)

namespace CharacterManager
{
void Update(float deltaTime);

// void UnsubscribeEntities(const gv::EntityList& entitiesToUnsubscribe);

TWeakObjectPtr<ACharacter> CreateCharacterForEntity(
    UWorld* world, TSubclassOf<ACharacter> characterType, gv::Entity entity,
    const gv::Position& position, ActorEntityManager::TrackActorLifetimeCallback callback);
}