#pragma once

#include "game/agent/combat/CombatComponentManager.hpp"

typedef class UAnimInstance UAnimInstance;
typedef class UAnimMontage UAnimMontage;

namespace gv
{
// @LatelinkDef
struct CombatFx
{
	// TODO: @Stability: Make these TWeakPtrs? TWeakObjectPtrs?
	UAnimInstance* AnimInstance = nullptr;
	UAnimMontage* AnimMontage = nullptr;

	enum class RagdollState
	{
		Unaffected = 0,
		Disable,
		Enable
	};
	RagdollState SetRagdollState = RagdollState::Unaffected;
};
}

class UnrealCombatFxHandler : public gv::CombatFxHandler
{
public:
	virtual ~UnrealCombatFxHandler() = default;

	virtual void OnActivateCombatAction(gv::Entity combatant, gv::CombatAction& action);
};