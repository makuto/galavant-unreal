// The MIT License (MIT) Copyright (c) 2016 Macoy Madson

#include "GalavantUnreal.h"
#include "AgentCharacter.h"

#include "AgentAIController.h"

#include "util/Logging.hpp"

// Sets default values
AAgentCharacter::AAgentCharacter()
{
	// Set this character to call Tick() every frame.  You can turn this off to improve performance
	// if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	AIControllerClass = AAgentAIController::StaticClass();
	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;

	// Configure character movement
	{
		// Character moves in the direction of input...
		GetCharacterMovement()->bOrientRotationToMovement = true;
		// ...at this rotation rate
		GetCharacterMovement()->RotationRate = FRotator(0.0f, 540.0f, 0.0f);
		GetCharacterMovement()->JumpZVelocity = 600.f;
		GetCharacterMovement()->AirControl = 0.2f;
	}
}

// Called when the game starts or when spawned
void AAgentCharacter::BeginPlay()
{
	Super::BeginPlay();
}

// Called every frame
void AAgentCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

// Called to bind functionality to input
void AAgentCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
	LOGD << "Player input set?";
}
