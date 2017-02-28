// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "GalavantUnreal.h"
#include "GalavantUnrealCharacter.h"

AGalavantUnrealCharacter::AGalavantUnrealCharacter()
{
	// Set size for player capsule
	GetCapsuleComponent()->InitCapsuleSize(42.f, 96.0f);

	// Don't rotate character to camera direction
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	// Configure character movement
	GetCharacterMovement()->bOrientRotationToMovement =
	    true;  // Rotate character to moving direction
	GetCharacterMovement()->RotationRate = FRotator(0.f, 640.f, 0.f);
	GetCharacterMovement()->bConstrainToPlane = true;
	GetCharacterMovement()->bSnapToPlaneAtStart = true;

	// Create a camera boom...
	{
		FAttachmentTransformRules attachmentRules(EAttachmentRule::SnapToTarget,
		                                          EAttachmentRule::SnapToTarget,
		                                          EAttachmentRule::SnapToTarget, false);
		CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
		CameraBoom->AttachToComponent(RootComponent, attachmentRules, TEXT("CameraBoom"));
		CameraBoom->bAbsoluteRotation = true;  // Don't want arm to rotate when character does
		CameraBoom->TargetArmLength = 800.f;
		CameraBoom->RelativeRotation = FRotator(-60.f, 0.f, 0.f);
		CameraBoom->bDoCollisionTest =
		    false;  // Don't want to pull camera in when it collides with level
	}

	// Create a camera...
	{
		FAttachmentTransformRules attachmentRules(EAttachmentRule::SnapToTarget,
		                                          EAttachmentRule::SnapToTarget,
		                                          EAttachmentRule::SnapToTarget, false);
		TopDownCameraComponent = CreateDefaultSubobject<UCameraComponent>(TEXT("TopDownCamera"));
		TopDownCameraComponent->AttachToComponent(CameraBoom, attachmentRules, TEXT("CameraBoom"));
		TopDownCameraComponent->bUsePawnControlRotation =
		    false;  // Camera does not rotate relative to arm
	}
}
