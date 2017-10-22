#include "GalavantUnreal.h"

#include "CharacterManager.hpp"

#include "game/agent/AgentComponentManager.hpp"

#include "ActorEntityManagement.h"

#include "util/CppHelperMacros.hpp"

namespace CharacterManager
{
struct CharacterManager
{
	typedef std::vector<TWeakObjectPtr<ACharacter>> CharacterList;
	CharacterList Characters;
};

static CharacterManager s_Characters;

void Update(float deltaTime)
{
	const gv::EntityList& unconsciousAgents = gv::g_AgentComponentManager.GetUnconsciousAgents();

	FOR_ITERATE_NO_INCR(CharacterManager::CharacterList, s_Characters.Characters, it)
	{
		if ((*it).IsValid())
		{
			// Ragdoll if unconscious
			{
				const FName Ragdoll_ProfileName = FName(TEXT("Ragdoll"));
				const FName CharacterMesh_ProfileName = FName(TEXT("CharacterMesh"));

				UMeshComponent* mesh =
				    (UMeshComponent*)(*it)->FindComponentByClass<UMeshComponent>();
				if (!mesh)
					continue;

				bool IsConscious = !gv::EntityListFindEntity(unconsciousAgents, (*it)->Entity);
				bool IsRagdoll = mesh->GetCollisionProfileName() == Ragdoll_ProfileName;
				if (!IsConscious && !IsRagdoll)
				{
					LOGD << "Setting ragdoll on entity " << (*it)->Entity;

					mesh->SetSimulatePhysics(true);
					mesh->SetCollisionProfileName(Ragdoll_ProfileName);
				}
				// Back to normal
				else if (IsConscious && IsRagdoll)
				{
					LOGD << "Disabling ragdoll on entity " << (*it)->Entity;

					mesh->SetSimulatePhysics(false);
					mesh->SetCollisionProfileName(CharacterMesh_ProfileName);
				}
			}

			++it;
		}
		else
		{
			it = s_Characters.Characters.erase(it);
		}
	}
}

TWeakObjectPtr<ACharacter> CreateCharacterForEntity(
    UWorld* world, TSubclassOf<ACharacter> characterType, gv::Entity entity,
    const gv::Position& position, ActorEntityManager::TrackActorLifetimeCallback callback)
{
	if (!world)
		return nullptr;

	TWeakObjectPtr<ACharacter> newCharacter = ActorEntityManager::CreateActorForEntity<ACharacter>(
	    world, characterType, entity, position, callback);

	s_Characters.Characters.push_back(newCharacter);

	return newCharacter;
}
}