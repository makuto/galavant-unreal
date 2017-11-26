// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.
#pragma once
#include "GameFramework/Character.h"

#include "entityComponentSystem/EntityTypes.hpp"
#include "game/agent/Needs.hpp"
#include "world/Position.hpp"

#include "GalavantUnrealFPCharacter.generated.h"

class UInputComponent;

struct PlayerCombatAction
{
	enum class ActionType
	{
		None = 0,

		MeleeAttack,
		MeleeBlock,

		ActionType_Count
	};
	ActionType Type;

	UAnimMontage* Animation;
};

UCLASS(config = Game)
class AGalavantUnrealFPCharacter : public ACharacter
{
	GENERATED_BODY()

	/** Pawn mesh: 1st person view (arms; seen only by self) */
	UPROPERTY(VisibleDefaultsOnly, Category = Mesh)
	class USkeletalMeshComponent* Mesh1P;

	/** Gun mesh: 1st person view (seen only by self) */
	UPROPERTY(VisibleDefaultsOnly, Category = Mesh)
	class USkeletalMeshComponent* FP_Gun;

	UPROPERTY(VisibleDefaultsOnly)
	class USphereComponent* LeftFist;

	/** First person camera */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera,
	          meta = (AllowPrivateAccess = "true"))
	class UCameraComponent* FirstPersonCameraComponent;

	/** Chunk Manager - this actor will spawn chunks within the radius of our character */
	UPROPERTY(VisibleDefaultsOnly)
	class UChildActorComponent* ChunkManager;

	/** Minimap */
	UPROPERTY(VisibleDefaultsOnly)
	class UChildActorComponent* Minimap;

	UPROPERTY(EditAnywhere, Category = Audio)
	class USoundBase* Footstep;

	UPROPERTY(EditAnywhere, Category = Audio)
	float FootstepPlayRate;

	gv::Entity PlayerEntity;

	// This will be the position referred to by the entire game when querying player position
	gv::Position PlayerPosition;

public:
	AGalavantUnrealFPCharacter();

	/** Base turn rate, in deg/sec. Other scaling may affect final turn rate. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera)
	float BaseTurnRate;

	/** Base look up/down rate, in deg/sec. Other scaling may affect final rate. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera)
	float BaseLookUpRate;

	/** Gun muzzle's offset from the characters location */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Gameplay)
	FVector GunOffset;

	/** Projectile class to spawn */
	UPROPERTY(EditDefaultsOnly, Category=Projectile)
	TSubclassOf<class AGalavantUnrealSimpleProjectile> ProjectileClass;

	/** Sound to play each time we fire */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Gameplay)
	class USoundBase* FireSound;

	/** AnimMontage to play each time we fire */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Gameplay)
	class UAnimMontage* FireAnimation;

	/** AnimMontage to play each time we use Primary attack */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Gameplay)
	class UAnimMontage* TempPrimaryAttackAnimation;

	/** AnimMontage to play each time we use Secondary attack */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Gameplay)
	class UAnimMontage* TempSecondaryAttackAnimation;

	/** AnimMontage to play each time we use Tertiary attack */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Gameplay)
	class UAnimMontage* TempTertiaryAttackAnimation;

	UPROPERTY(EditAnywhere)
	float MaxInteractManhattanDistance;

	// Set up player entity
	virtual void BeginPlay() override;

	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	virtual void Tick(float DeltaSeconds) override;

protected:
	UFUNCTION()
	void OnLeftFistHit(UPrimitiveComponent* component, AActor* otherActor,
	                   UPrimitiveComponent* otherComponent, FVector normalImpulse,
	                   const FHitResult& hit);

	/** Fires a projectile. */
	void OnFire();

	void OnInteract();
	void OnUsePrimary();
	void OnUseSecondary();
	void OnUseTertiary();

	/** Handles moving forward/backward */
	void MoveForward(float Val);

	/** Handles stafing movement, left and right */
	void MoveRight(float Val);

	/**
	 * Called via input to turn at a given rate.
	 * @param Rate	This is a normalized rate, i.e. 1.0 means 100% of desired turn rate
	 */
	void TurnAtRate(float Rate);

	/**
	 * Called via input to turn look up/down at a given rate.
	 * @param Rate	This is a normalized rate, i.e. 1.0 means 100% of desired turn rate
	 */
	void LookUpAtRate(float Rate);

	bool CombatAttemptAction(PlayerCombatAction& action);

protected:
	// APawn interface
	virtual void SetupPlayerInputComponent(UInputComponent* InputComponent) override;
	// End of APawn interface

	void ExitGalavant();
	void TogglePlayGalavant();
	void ToggleMouseLock();

public:
	/** Returns Mesh1P subobject **/
	FORCEINLINE class USkeletalMeshComponent* GetMesh1P() const
	{
		return Mesh1P;
	}
	/** Returns FirstPersonCameraComponent subobject **/
	FORCEINLINE class UCameraComponent* GetFirstPersonCameraComponent() const
	{
		return FirstPersonCameraComponent;
	}
};
