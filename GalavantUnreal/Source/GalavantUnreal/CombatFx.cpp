#include "GalavantUnreal.h"

#include "CombatFx.hpp"

#include "util/Logging.hpp"

#include "ActorEntityManagement.h"

const FName Ragdoll_ProfileName = FName(TEXT("Ragdoll"));

void UnrealCombatFxHandler::OnActivateCombatAction(gv::Entity combatant, gv::CombatAction& action)
{
	gv::CombatFx* actionFx = action.Fx;

	if (actionFx && actionFx->AnimInstance && actionFx->AnimMontage)
	{
		if (actionFx->AnimInstance->Montage_Play(actionFx->AnimMontage, 1.f))
			LOGD << "Played animation";
		else
			LOGD << "Animation failed to play";
	}

	// This code no longer works now that CharacterManager handles ragdoll states. Should this still
	//  be an option in CombatFx? I'll figure that out going forwards
	/*TWeakObjectPtr<AActor> entityActor = ActorEntityManager::GetActorFromEntity(combatant);
	if (entityActor.IsValid())
	{
		if ((actionFx && actionFx->SetRagdollState != gv::CombatFx::RagdollState::Unaffected) ||
		    (action.Def && action.Def->Die))
		{
			UMeshComponent* mesh =
			    (UMeshComponent*)entityActor->FindComponentByClass<UMeshComponent>();
			if (mesh)
			{
				bool shouldRagdoll =
				    (actionFx ?
				         (actionFx->SetRagdollState == gv::CombatFx::RagdollState::Enable ? true :
				                                                                            false) :
				         action.Def->Die);
				LOGD << "Setting ragdoll on entity " << combatant;
				mesh->SetSimulatePhysics(shouldRagdoll);
				mesh->SetCollisionProfileName(shouldRagdoll ?
				                                  Ragdoll_ProfileName :
				                                  UCollisionProfile::NoCollision_ProfileName);
			}
		}
	}*/
}