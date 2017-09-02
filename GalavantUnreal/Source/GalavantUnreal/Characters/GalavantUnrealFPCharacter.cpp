// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "GalavantUnreal.h"

#include "GalavantUnrealFPCharacter.h"

#include "Animation/AnimInstance.h"
#include "GameFramework/InputSettings.h"

#include "Utilities/ConversionHelpers.h"
#include "HUDMinimapActor.h"

#include "util/Logging.hpp"
#include "entityComponentSystem/EntityComponentManager.hpp"
#include "game/agent/AgentComponentManager.hpp"
#include "game/InteractComponentManager.hpp"
#include "entityComponentSystem/EntitySharedData.hpp"
#include "game/agent/Needs.hpp"

DEFINE_LOG_CATEGORY_STATIC(LogFPChar, Warning, All);

//////////////////////////////////////////////////////////////////////////
// AGalavantUnrealFPCharacter

AGalavantUnrealFPCharacter::AGalavantUnrealFPCharacter()
{
	// Set size for collision capsule
	GetCapsuleComponent()->InitCapsuleSize(42.f, 96.0f);

	// set our turn rates for input
	BaseTurnRate = 45.f;
	BaseLookUpRate = 45.f;

	// Create a CameraComponent
	FirstPersonCameraComponent =
	    CreateDefaultSubobject<UCameraComponent>(TEXT("FirstPersonCamera"));
	FirstPersonCameraComponent->SetupAttachment(GetCapsuleComponent());
	FirstPersonCameraComponent->RelativeLocation = FVector(0, 0, 64.f);  // Position the camera
	FirstPersonCameraComponent->bUsePawnControlRotation = true;

	// Create a mesh component that will be used when being viewed from a '1st person' view (when
	// controlling this pawn)
	Mesh1P = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("CharacterMesh1P"));
	Mesh1P->SetOnlyOwnerSee(true);
	Mesh1P->SetupAttachment(FirstPersonCameraComponent);
	Mesh1P->bCastDynamicShadow = false;
	Mesh1P->CastShadow = false;

	// Create a gun mesh component
	FP_Gun = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("FP_Gun"));
	FP_Gun->SetOnlyOwnerSee(true);  // only the owning player will see this mesh
	FP_Gun->bCastDynamicShadow = false;
	FP_Gun->CastShadow = false;
	FP_Gun->SetupAttachment(Mesh1P, TEXT("GripPoint"));

	// Default offset from the character location for projectiles to spawn
	GunOffset = FVector(100.0f, 30.0f, 10.0f);

	ChunkManager = CreateDefaultSubobject<UChildActorComponent>(TEXT("ChunkManager"));
	ChunkManager->SetupAttachment(FirstPersonCameraComponent);

	Minimap = CreateDefaultSubobject<UChildActorComponent>(TEXT("Minimap"));
	Minimap->SetupAttachment(FirstPersonCameraComponent);
	Minimap->SetChildActorClass(AHUDMinimapActor::StaticClass());

	// Note: The ProjectileClass and the skeletal mesh/anim blueprints for Mesh1P are set in the
	// derived blueprint asset named MyCharacter (to avoid direct content references in C++)

	MaxInteractManhattanDistance = 600.f;
}

void AGalavantUnrealFPCharacter::BeginPlay()
{
	Super::BeginPlay();

	LOGI << "Initializing Player";

	// Setup player entity
	if (!PlayerEntity)
	{
		gv::EntityComponentManager* entityComponentManager =
		    gv::EntityComponentManager::GetSingleton();
		if (!entityComponentManager)
			return;

		gv::EntityList newEntities;
		entityComponentManager->GetNewEntities(newEntities, 1);
		PlayerEntity = newEntities[0];

		// Setup player position
		{
			USceneComponent* sceneComponent = GetRootComponent();
			FVector trueWorldPosition;
			if (sceneComponent)
				trueWorldPosition = sceneComponent->GetComponentLocation();
			PlayerPosition = ToPosition(trueWorldPosition);
			gv::EntityPlayerRegisterPosition(PlayerEntity, &PlayerPosition);
		}

		// Setup components
		{
			gv::AgentComponentManager* agentComponentManager =
			    gv::GetComponentManagerForType<gv::AgentComponentManager>(gv::ComponentType::Agent);
			if (!agentComponentManager)
				return;
			gv::AgentComponentManager::AgentComponentList newAgentComponents(1);

			newAgentComponents[0].entity = PlayerEntity;

			gv::Need hungerNeed(RESKEY("Hunger"));
			gv::Need bloodNeed(RESKEY("Blood"));
			newAgentComponents[0].data.Needs.push_back(hungerNeed);
			newAgentComponents[0].data.Needs.push_back(bloodNeed);

			LOGD << "Registering Player AgentComponent";
			agentComponentManager->SubscribeEntities(newAgentComponents);
		}

		// Register resource as player
		{
			gv::WorldResourceLocator::AddResource(gv::WorldResourceType::Player, PlayerEntity,
			                                      PlayerPosition);
		}
	}

	LOGI << "Initializing Player done";
}

void AGalavantUnrealFPCharacter::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);

	// Remove the position we were tracking
	gv::EntityPlayerUnregisterPosition();

	gv::EntityComponentManager* entityComponentManager = gv::EntityComponentManager::GetSingleton();
	if (entityComponentManager)
	{
		gv::EntityList entitiesToDestroy = {PlayerEntity};
		entityComponentManager->MarkDestroyEntities(entitiesToDestroy);
	}
}

void AGalavantUnrealFPCharacter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	USceneComponent* sceneComponent = GetRootComponent();
	gv::Position trueWorldPosition;
	if (sceneComponent)
		trueWorldPosition = ToPosition(sceneComponent->GetComponentLocation());

	// Give ResourceLocator our new position
	gv::WorldResourceLocator::MoveResource(gv::WorldResourceType::Player, PlayerEntity,
	                                       PlayerPosition, trueWorldPosition);

	// Updating PlayerPosition here updates it for anyone watching
	PlayerPosition = trueWorldPosition;
}

//////////////////////////////////////////////////////////////////////////
// Input

void AGalavantUnrealFPCharacter::SetupPlayerInputComponent(class UInputComponent* InputComponent)
{
	// set up gameplay key bindings
	check(InputComponent);

	InputComponent->BindAction("Jump", IE_Pressed, this, &ACharacter::Jump);
	InputComponent->BindAction("Jump", IE_Released, this, &ACharacter::StopJumping);

	InputComponent->BindAction("Fire", IE_Pressed, this, &AGalavantUnrealFPCharacter::OnFire);

	InputComponent->BindAxis("MoveForward", this, &AGalavantUnrealFPCharacter::MoveForward);
	InputComponent->BindAxis("MoveRight", this, &AGalavantUnrealFPCharacter::MoveRight);

	// We have 2 versions of the rotation bindings to handle different kinds of devices differently
	// "turn" handles devices that provide an absolute delta, such as a mouse.
	// "turnrate" is for devices that we choose to treat as a rate of change, such as an analog
	// joystick
	InputComponent->BindAxis("Turn", this, &AGalavantUnrealFPCharacter::AddControllerYawInput);
	InputComponent->BindAxis("TurnRate", this, &AGalavantUnrealFPCharacter::TurnAtRate);
	InputComponent->BindAxis("LookUp", this, &AGalavantUnrealFPCharacter::AddControllerPitchInput);
	InputComponent->BindAxis("LookUpRate", this, &AGalavantUnrealFPCharacter::LookUpAtRate);

	// Galavant-specific bindings
	{
		InputComponent->BindAction("Interact", IE_Pressed, this,
		                           &AGalavantUnrealFPCharacter::OnInteract);
		InputComponent->BindAction("UsePrimary", IE_Pressed, this,
		                           &AGalavantUnrealFPCharacter::OnUsePrimary);
		InputComponent->BindAction("UseSecondary", IE_Pressed, this,
		                           &AGalavantUnrealFPCharacter::OnUseSecondary);
		InputComponent->BindAction("UseTertiary", IE_Pressed, this,
		                           &AGalavantUnrealFPCharacter::OnUseTertiary);
	}
}

void AGalavantUnrealFPCharacter::OnFire()
{
	/*// try and fire a projectile
	if (ProjectileClass != nullptr)
	{
	    const FRotator SpawnRotation = GetControlRotation();
	    // MuzzleOffset is in camera space, so transform it to world space before offsetting from
	the character location to find the final muzzle position
	    const FVector SpawnLocation = GetActorLocation() + SpawnRotation.RotateVector(GunOffset);

	    UWorld* const World = GetWorld();
	    if (World != nullptr)
	    {
	        // spawn the projectile at the muzzle
	        World->SpawnActor<GalavantUnrealFPProjectile>(ProjectileClass, SpawnLocation,
	SpawnRotation);
	    }
	}

	// try and play the sound if specified
	if (FireSound != nullptr)
	{
	    UGameplayStatics::PlaySoundAtLocation(this, FireSound, GetActorLocation());
	}

	// try and play a firing animation if specified
	if(FireAnimation != nullptr)
	{
	    // Get the animation object for the arms mesh
	    UAnimInstance* AnimInstance = Mesh1P->GetAnimInstance();
	    if(AnimInstance != nullptr)
	    {
	        AnimInstance->Montage_Play(FireAnimation, 1.f);
	    }
	}
	*/
}

void AGalavantUnrealFPCharacter::OnInteract()
{
	LOGD << "Player interacting!";
	// TODO: Make interact system work based on proximity
	USceneComponent* sceneComponent = GetRootComponent();
	FVector trueWorldPosition = sceneComponent->GetComponentLocation();
	gv::Position worldPosition(ToPosition(trueWorldPosition));
	float manhattanTo = 0.f;
	gv::WorldResourceLocator::Resource* nearestFood =
	    gv::WorldResourceLocator::FindNearestResource(gv::WorldResourceType::Food, worldPosition,
	                                                  /*allowSameLocation*/ true, manhattanTo);
	if (nearestFood && manhattanTo < MaxInteractManhattanDistance)
	{
		gv::InteractComponentManager* interactComponentManager =
		    gv::GetComponentManagerForType<gv::InteractComponentManager>(
		        gv::ComponentType::Interact);

		if (interactComponentManager)
		{
			interactComponentManager->PickupDirect(nearestFood->entity, PlayerEntity);
		}
	}
}

static float s_lastActionTime = 0.f;

bool AGalavantUnrealFPCharacter::CombatAttemptAction(CombatAction& action)
{
	UWorld* world = GetWorld();
	if (!world)
		return false;

	float currentTime = world->GetTimeSeconds();

	// Can only do so many actions per second
	if (currentTime - s_lastActionTime < 0.5f)
		return false;

	LOGD << "Performing combat action of type " << (int)action.Type;

	s_lastActionTime = currentTime;

	if (action.Animation)
	{
		// Get the animation object for the arms mesh
		UAnimInstance* AnimInstance = Mesh1P->GetAnimInstance();
		if (AnimInstance != nullptr)
		{
			AnimInstance->Montage_Play(action.Animation, 1.f);
		}
	}

	return true;
}

void AGalavantUnrealFPCharacter::OnUsePrimary()
{
	LOGD << "Player Use Primary!";
	CombatAction attemptAction = {CombatAction::ActionType::MeleeAttack,
	                              TempPrimaryAttackAnimation};
	CombatAttemptAction(attemptAction);
}

void AGalavantUnrealFPCharacter::OnUseSecondary()
{
	LOGD << "Player Use Secondary!";
	CombatAction attemptAction = {CombatAction::ActionType::MeleeBlock,
	                              TempSecondaryAttackAnimation};
	CombatAttemptAction(attemptAction);
}

void AGalavantUnrealFPCharacter::OnUseTertiary()
{
	LOGD << "Player Use Tertiary!";
	CombatAction attemptAction = {CombatAction::ActionType::MeleeAttack,
	                              TempTertiaryAttackAnimation};
	CombatAttemptAction(attemptAction);
}

void AGalavantUnrealFPCharacter::MoveForward(float Value)
{
	if (Value != 0.0f)
	{
		// add movement in that direction
		AddMovementInput(GetActorForwardVector(), Value);
	}
}

void AGalavantUnrealFPCharacter::MoveRight(float Value)
{
	if (Value != 0.0f)
	{
		// add movement in that direction
		AddMovementInput(GetActorRightVector(), Value);
	}
}

void AGalavantUnrealFPCharacter::TurnAtRate(float Rate)
{
	// calculate delta for this frame from the rate information
	AddControllerYawInput(Rate * BaseTurnRate * GetWorld()->GetDeltaSeconds());
}

void AGalavantUnrealFPCharacter::LookUpAtRate(float Rate)
{
	// calculate delta for this frame from the rate information
	AddControllerPitchInput(Rate * BaseLookUpRate * GetWorld()->GetDeltaSeconds());
}