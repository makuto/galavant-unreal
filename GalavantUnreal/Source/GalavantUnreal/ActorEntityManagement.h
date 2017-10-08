
#pragma once

#include "entityComponentSystem/EntityTypes.hpp"
#include "entityComponentSystem/EntityComponentManager.hpp"
#include "world/Position.hpp"
#include "Utilities/ConversionHelpers.h"

#include <functional>

#include <iostream>

// This module tracks Actor-Entity pairs and ensures that if an Actor was destroyed the components
// for the associated Entity will know about it. This is to fix the problem where Unreal can destroy
// an Actor without our Entity system knowing about it
namespace ActorEntityManager
{
void Clear();

// Advanced setup: if actor is destroyed, call callback
typedef std::function<void(gv::Entity)> TrackActorLifetimeCallback;
void TrackActorLifetime(AActor* actor, TrackActorLifetimeCallback callback);

bool IsActorActive(AActor* actor);

// I'm requiring the lifetime callback at the moment while I figure out the most stable way to
// manage lifetime
template <class T>
T* CreateActorForEntity(UWorld* world, TSubclassOf<T> actorType, gv::Entity entity,
                        const gv::Position& position, TrackActorLifetimeCallback callback)
{
	if (!world)
		return nullptr;

	std::cout << "UWorld* world " << world << " TSubclassOf<T> actorType " << actorType.Get()
	          << " gv::Entity entity " << entity << " const gv::Position& position " << position.X
	          << ", " << position.Y << ", " << position.Z << "\n";

	FActorSpawnParameters spawnParams;
	FVector positionVector(ToFVector(position));
	FRotator defaultRotation(0.f, 0.f, 0.f);
	T* actor = (T*)world->SpawnActor<T>(actorType, positionVector, defaultRotation, spawnParams);
	if (actor)
	{
		actor->Entity = (int)entity;
		TrackActorLifetime(actor, callback);
	}
	return actor;
}
}