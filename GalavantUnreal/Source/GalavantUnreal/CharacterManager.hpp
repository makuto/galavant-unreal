#pragma once

#include "ActorEntityManagement.h"

#include "entityComponentSystem/EntityTypes.hpp"
#include "entityComponentSystem/EntityComponentManager.hpp"
#include "world/Position.hpp"

// Character Manager - Handle Unreal Characters interaction with Galavant systems (movement,
// animation, ragdoll, etc.)

namespace CharacterManager
{
void Update(float deltaTime);

TWeakObjectPtr<ACharacter> CreateCharacterForEntity(
    UWorld* world, TSubclassOf<ACharacter> characterType, gv::Entity entity,
    const gv::Position& position, ActorEntityManager::TrackActorLifetimeCallback callback);
}