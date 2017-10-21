#include "GalavantUnreal.h"

#include "CharacterManager.hpp"

#include "game/agent/AgentComponentManager.hpp"

#include "ActorEntityManagement.h"

namespace CharacterManager
{
struct CharacterManager
{
	gv::EntityList Subscribers;

	typedef std::vector<TWeakObjectPtr<ACharacter>> CharacterList;
	CharacterList Characters;
};

static CharacterManager s_Characters;

// void UnsubscribeEntities(const gv::EntityList& entitiesToUnsubscribe)
// {
// 	// TODO: Why did I make this callback thing if I just copy paste ComponentManager
// 	// UnsubscribeEntities? This is dumb
// 	if (!entities.empty())
// 	{
// 		// Copy for modification
// 		EntityList entitiesToUnsubscribe;
// 		EntityListAppendList(entitiesToUnsubscribe, entities);

// 		// Make sure they're actually subscribed
// 		EntityListRemoveUniqueEntitiesInSuspect(Subscribers, entitiesToUnsubscribe);

// 		if (!entitiesToUnsubscribe.empty())
// 		{
// 			UnsubscribeEntitiesInternal(entitiesToUnsubscribe);

// 			// Remove from subscribers
// 			EntityListRemoveNonUniqueEntitiesInSuspect(entitiesToUnsubscribe, Subscribers);

// 			LOGD << "Manager "
// 			     << " unsubscribed " << entitiesToUnsubscribe.size() << " entities";
// 		}
// 	}

// 	UnsubscribeEntities(Subscribers, UnsubscribeEntitiesInternal);
// }

void Update(float deltaTime)
{
	gv::AgentConsciousStateList subscriberConsciousStates;

	gv::g_AgentComponentManager.GetAgentConsciousStates(s_Characters.Subscribers,
	                                                    subscriberConsciousStates);

	for (TWeakObjectPtr<ACharacter>& character : s_Characters.Characters)
	{
		if (character.IsValid())
		{
		}
		else
			character = nullptr;
	}
}

TWeakObjectPtr<ACharacter> CreateCharacterForEntity(
    UWorld* world, TSubclassOf<ACharacter> characterType, gv::Entity entity,
    const gv::Position& position, ActorEntityManager::TrackActorLifetimeCallback callback)
{
	if (!world)
		return nullptr;

	/*gv::EntityListIterator findIt = std::find(s_Characters.Subscribers.begin(), s_Characters.Subscribers.end(), entity);

	TWeakObjectPtr<ACharacter>* entityAssociatedCharacter = nullptr; 

	if (findIt != s_Characters.Subscribers.end())
	{
		for (TWeakObjectPtr<ACharacter>* character : s_Characters.Characters)
		{
			if (character->IsValid())
			{
				if ((*character)->Entity == entity)
					return character;
				else
				{
					entityAssociatedCharacter = character;
					break;
				}
			}
		}
	}*/

	TWeakObjectPtr<ACharacter> newCharacter = ActorEntityManager::CreateActorForEntity<ACharacter>(
	    world, characterType, entity, position, callback);

	/*if (entityAssociatedCharacter)
		find

	newCharacter*/

	return newCharacter;
}
}