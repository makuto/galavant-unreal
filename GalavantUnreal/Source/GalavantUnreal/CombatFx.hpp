#pragma once

#include "game/agent/combat/CombatComponentManager.hpp"

typedef class UAnimInstance UAnimInstance;
typedef class UAnimMontage UAnimMontage;

namespace gv
{
struct CombatFx
{
	UAnimInstance* AnimInstance;
	UAnimMontage* AnimMontage;
};
}

class UnrealCombatFxHandler : public gv::CombatFxHandler
{
public:
	virtual ~UnrealCombatFxHandler() = default;

	virtual void OnActivateCombatAction(gv::Entity combatant, gv::CombatAction& action);
};