// The MIT License (MIT) Copyright (c) 2016 Macoy Madson

#include "GalavantUnreal.h"
#include "AgentAIController.h"

AAgentAIController::AAgentAIController()
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if
	// you don't need it.
	PrimaryActorTick.bCanEverTick = true;
}

void AAgentAIController::BeginPlay()
{
	Super::BeginPlay();
}

void AAgentAIController::Tick(float deltaTime)
{
	Super::Tick(deltaTime);
}
