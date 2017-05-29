#include "GalavantUnreal.h"

#include "ActorEntityManagement.h"

#include "util/Logging.hpp"

#include <vector>
#include <map>
#include <iterator>

namespace ActorEntityManager
{
struct ActorEntity
{
	AActor* Actor;
	gv::Entity Entity;
};

typedef std::vector<ActorEntity> ActorEntityList;
ActorEntityList s_ActorEntities;

typedef std::vector<TrackActorLifetimeCallback> TrackActorLifetimeCallbackList;
typedef std::map<const AActor*, TrackActorLifetimeCallbackList> ActorLifetimeCallbacks;
ActorLifetimeCallbacks s_ActorLifetimeCallbacks;

void Clear()
{
	s_ActorEntities.clear();
	s_ActorLifetimeCallbacks.clear();
}

void AddActorEntity(AActor* actor, gv::Entity entity)
{
	s_ActorEntities.push_back({actor, entity});
}

void DestroyEntitiesWithDestroyedActors(UWorld* world,
                                        gv::EntityComponentManager* entityComponentSystem)
{
	if (world && entityComponentSystem)
	{
		const TArray<class ULevel*>& levels = world->GetLevels();
		gv::EntityList entitiesToDestroy;

		// This O(n * m) through all actors and entities is brutal. I should hook into Actor
		// destruction (i.e. by adding some callback or something to AActor base class), but
		// for now this is okay
		int lastGoodActorEntityIndex = s_ActorEntities.size() - 1;
		for (int i = lastGoodActorEntityIndex; i >= 0; i--)
		{
			ActorEntity& actorEntity = s_ActorEntities[i];
			bool found = false;
			for (const ULevel* level : levels)
			{
				for (const AActor* actor : level->Actors)
				{
					if (actorEntity.Actor == actor)
					{
						found = true;
						break;
					}
				}
				if (found)
					break;
			}

			if (!found)
			{
				actorEntity = s_ActorEntities[lastGoodActorEntityIndex--];
				entitiesToDestroy.push_back(actorEntity.Entity);
			}
		}

		if (!entitiesToDestroy.empty())
		{
			LOGI << "Destroying " << entitiesToDestroy.size()
			     << " entities because their Actors were destroyed";

			s_ActorEntities.resize(lastGoodActorEntityIndex + 1);
			entityComponentSystem->MarkDestroyEntities(entitiesToDestroy);
		}
	}
}

void TrackActorLifetime(AActor* actor, TrackActorLifetimeCallback callback)
{
	if (!actor)
		return;

	ActorLifetimeCallbacks::iterator findIt = s_ActorLifetimeCallbacks.find(actor);
	if (findIt != s_ActorLifetimeCallbacks.end())
		findIt->second.push_back(callback);
	else
		s_ActorLifetimeCallbacks[actor] = {callback};
}

void UpdateNotifyOnActorDestroy(UWorld* world)
{
	if (!world)
		return;

	const TArray<class ULevel*>& levels = world->GetLevels();

	for (const ULevel* level : levels)
	{
		if (!level)
			continue;

		for (const AActor* actor : level->Actors)
		{
			if (!actor)
				continue;
			
			if (actor->IsPendingKillPending())
			{
				ActorLifetimeCallbacks::iterator findIt =
				    s_ActorLifetimeCallbacks.find(actor);
				if (findIt == s_ActorLifetimeCallbacks.end())
					continue;

				for (TrackActorLifetimeCallback callbackOnDestroy : findIt->second)
					callbackOnDestroy(actor);

				// The actor has been removed, so there's no reason to track its lifetime
				s_ActorLifetimeCallbacks.erase(findIt);
			}
		}
	}
}
}