// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "GalavantUnreal.h"

#include "GalavantUnrealFPCharacter.h"

#include "Animation/AnimInstance.h"
#include "GameFramework/InputSettings.h"

#include "Utilities/ConversionHelpers.h"
#include "HUDMinimapActor.h"
#include "CombatFx.hpp"

#include "util/Logging.hpp"
#include "entityComponentSystem/EntityComponentManager.hpp"
#include "game/agent/AgentComponentManager.hpp"
#include "game/agent/combat/CombatComponentManager.hpp"
#include "game/InteractComponentManager.hpp"
#include "entityComponentSystem/EntitySharedData.hpp"
#include "game/agent/Needs.hpp"
#include "util/Time.hpp"

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

	LeftFist = CreateDefaultSubobject<USphereComponent>(TEXT("LeftFist"));
	LeftFist->SetSphereRadius(25.f);
	LeftFist->SetNotifyRigidBodyCollision(true);
	LeftFist->OnComponentHit.AddDynamic(this, &AGalavantUnrealFPCharacter::OnLeftFistHit);
	LeftFist->SetupAttachment(Mesh1P, TEXT("GripPoint_l"));

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
		gv::EntityList newEntities;
		gv::g_EntityComponentManager.GetNewEntities(newEntities, 1);
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
			// Agent component
			{
				gv::AgentComponentManager::AgentComponentList newAgentComponents(1);

				newAgentComponents[0].entity = PlayerEntity;

				gv::Need hungerNeed(RESKEY("Hunger"));
				gv::Need bloodNeed(RESKEY("Blood"));
				newAgentComponents[0].data.Needs.push_back(hungerNeed);
				newAgentComponents[0].data.Needs.push_back(bloodNeed);

				LOGD << "Registering Player AgentComponent";
				gv::g_AgentComponentManager.SubscribeEntities(newAgentComponents);
			}

			// Combat component
			{
				gv::CombatantRefList newCombatants;
				gv::g_CombatComponentManager.CreateCombatants(newEntities, newCombatants);

				if (newCombatants.empty())
					LOGE << "Could not create combatant component for player; maybe it has already "
					        "been created?";
				else
				{
					// No setup needed
				}
			}
		}

		// Register resource as player
		{
			gv::WorldResourceLocator::AddResource(gv::WorldResourceType::Player, PlayerEntity,
			                                      PlayerPosition);
		}
	}

	LOGI << "Initializing Player done";

	// Create HUD
	{
		/*template< class T >
		T* CreateWidget(APlayerController* OwningPlayer, UClass* UserWidgetClass = T::StaticClass())
		{
		    return Cast<T>(UUserWidget::CreateWidgetOfClass(UserWidgetClass, nullptr, nullptr,
		OwningPlayer));
		}*/
	}
}

void AGalavantUnrealFPCharacter::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);

	// Remove the position we were tracking
	gv::EntityPlayerUnregisterPosition();

	gv::EntityList entitiesToDestroy = {PlayerEntity};
	gv::g_EntityComponentManager.MarkDestroyEntities(entitiesToDestroy);
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

	// Game bindings (not gameplay related)
	{
		InputComponent->BindAction("ExitGalavant", IE_Pressed, this,
		                           &AGalavantUnrealFPCharacter::ExitGalavant);
		InputComponent->BindAction("TogglePlayGalavant", IE_Pressed, this,
		                           &AGalavantUnrealFPCharacter::TogglePlayGalavant);
		InputComponent->BindAction("ToggleMouseLock", IE_Pressed, this,
		                           &AGalavantUnrealFPCharacter::ToggleMouseLock);
	}
}

void AGalavantUnrealFPCharacter::ExitGalavant()
{
	LOGI << "Requesting exit";
	FGenericPlatformMisc::RequestExit(/*Force=*/false);
}

void AGalavantUnrealFPCharacter::TogglePlayGalavant()
{
	LOGI << "Setting Galavant IsPlaying to " << !gv::GameIsPlaying();
	gv::GameSetPlaying(!gv::GameIsPlaying());
}

void AGalavantUnrealFPCharacter::ToggleMouseLock()
{
	// TODO: @Broken: This doesn't seem to actually release the mouse
	if (GEngine && GEngine->GameViewport && GEngine->GameViewport->Viewport)
	{
		bool isMouseCaptured = GEngine->GameViewport->Viewport->HasMouseCapture();
		LOGI << "Setting mouse capture to " << !isMouseCaptured;
		GEngine->GameViewport->Viewport->CaptureMouse(!isMouseCaptured);
	}
}

void AGalavantUnrealFPCharacter::OnLeftFistHit(UPrimitiveComponent* component, AActor* otherActor,
                                               UPrimitiveComponent* otherComponent,
                                               FVector normalImpulse, const FHitResult& hit)
{
	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(85005, 1.f, FColor::Red,
		                                 FString::Printf(TEXT("Left Fist Hit")));
	}

	LOGI << "Player left fist hit";
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
		gv::g_InteractComponentManager.PickupDirect(nearestFood->entity, PlayerEntity);
	}
}

static float s_lastActionTime = 0.f;

bool AGalavantUnrealFPCharacter::CombatAttemptAction(PlayerCombatAction& playerAction)
{
	UWorld* world = GetWorld();
	if (!world)
		return false;

	float currentTime = world->GetTimeSeconds();

	// Somewhat stupid special case: If you've done play in editor twice, the second time will have
	// the static time way larger than current time, so you'll never be able to use any actions.
	// Detect that here and reset the last action time
	if (s_lastActionTime > currentTime)
		s_lastActionTime = 0.f;

	// Can only do so many actions per second
	if (currentTime - s_lastActionTime < 0.5f)
		return false;

	s_lastActionTime = currentTime;

	// TODO: This is not sustainable. Figure out CombatFx
	static gv::CombatFx fx;
	fx.AnimInstance = Mesh1P->GetAnimInstance();
	fx.AnimMontage = playerAction.Animation;
	gv::CombatAction action = {gv::g_CombatActionDefDictionary.GetResource(RESKEY("Punch")), &fx,
	                           0.f};

	gv::g_CombatComponentManager.ActivateCombatAction(PlayerEntity, action);

	return true;
}

void AGalavantUnrealFPCharacter::OnUsePrimary()
{
	PlayerCombatAction attemptAction = {PlayerCombatAction::ActionType::MeleeAttack,
	                                    TempPrimaryAttackAnimation};
	CombatAttemptAction(attemptAction);
}

void AGalavantUnrealFPCharacter::OnUseSecondary()
{
	PlayerCombatAction attemptAction = {PlayerCombatAction::ActionType::MeleeBlock,
	                                    TempSecondaryAttackAnimation};
	CombatAttemptAction(attemptAction);
}

void AGalavantUnrealFPCharacter::OnUseTertiary()
{
	PlayerCombatAction attemptAction = {PlayerCombatAction::ActionType::MeleeAttack,
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