#include "GalavantUnreal.h"

#include "ActorEntityManagement.h"

#include "util/Logging.hpp"

#include <vector>
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

void Clear()
{
	s_ActorEntities.clear();
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
}