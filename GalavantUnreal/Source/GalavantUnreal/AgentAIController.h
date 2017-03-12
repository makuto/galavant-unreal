// The MIT License (MIT) Copyright (c) 2016 Macoy Madson

#pragma once

#include "AIController.h"
#include "AgentAIController.generated.h"

/**
 *
 */
UCLASS()
class GALAVANTUNREAL_API AAgentAIController : public AAIController
{
	GENERATED_BODY()

public:
	AAgentAIController();

	virtual void BeginPlay();
	virtual void Tick(float deltaTime);
};
