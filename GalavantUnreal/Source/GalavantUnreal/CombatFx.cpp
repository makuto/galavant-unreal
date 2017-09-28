#include "GalavantUnreal.h"

#include "CombatFx.hpp"

#include "util/Logging.hpp"

void UnrealCombatFxHandler::OnActivateCombatAction(gv::Entity combatant, gv::CombatAction& action)
{
	if (!action.Fx)
		return;

	gv::CombatFx* actionFx = action.Fx;

	if (actionFx->AnimInstance && actionFx->AnimMontage)
	{
		if (actionFx->AnimInstance->Montage_Play(actionFx->AnimMontage, 1.f))
			LOGD << "Played animation";
		else
			LOGD << "Animation failed to play";
	}
}