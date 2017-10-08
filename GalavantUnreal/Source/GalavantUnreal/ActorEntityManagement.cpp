#include "GalavantUnreal.h"

#include "ActorEntityManagement.h"

#include "util/Logging.hpp"

#include <vector>
#include <map>
#include <iterator>

namespace ActorEntityManager
{
struct TrackedActor
{
	bool IsBeingDestroyed;
	TrackActorLifetimeCallback OnDestroyCallback;

	void operator=(const TrackedActor& other)
	{
		IsBeingDestroyed = other.IsBeingDestroyed;
		OnDestroyCallback = other.OnDestroyCallback;
	}
};

typedef std::map<const AActor*, TrackedActor> TrackedActorMap;
TrackedActorMap s_TrackedActors;

void Clear()
{
	s_TrackedActors.clear();
}

typedef std::vector<AActor*> ActiveActorList;
ActiveActorList s_ActiveActors;

bool IsActorActive(AActor* actor)
{
	for (AActor* currentActor : s_ActiveActors)
	{
		if (actor == currentActor)
			return true;
	}
	return false;
}

void TrackActorLifetime(AActor* actor, TrackActorLifetimeCallback callback)
{
	if (!actor)
		return;

	s_TrackedActors[actor] = {false, callback};
	s_ActiveActors.push_back(actor);
}

void UnrealActorEntityOnDestroy(AActor* actor, int entity)
{
	LOGD << "Actor assigned to entity " << entity << " was destroyed";

	TrackedActorMap::iterator findIt = s_TrackedActors.find(actor);
	if (findIt == s_TrackedActors.end())
	{
		LOGE << "Actor had entity but no lifetime callbacks! Something should be making sure their "
		        "Actors are valid but aren't. The Entity will be destroyed";
		gv::g_EntityComponentManager.MarkDestroyEntities({(unsigned int)entity});
		return;
	}

	findIt->second.IsBeingDestroyed = true;

	findIt->second.OnDestroyCallback((gv::Entity)entity);

	// The actor has been removed, so there's no reason to track its lifetime
	// Note that whatever happens in callbackOnDestroy() could spawn and track an Actor which has
	// the same address as the last one (maybe, I'm not sure how Unreal manages Actor memory). Only
	// destroy this TrackedActor if we are certain they didn't re-add the callback
	if (findIt->second.IsBeingDestroyed)
		s_TrackedActors.erase(findIt);
}
}