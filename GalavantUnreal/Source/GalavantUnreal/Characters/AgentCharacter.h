// The MIT License (MIT) Copyright (c) 2016 Macoy Madson

#pragma once

#include "GameFramework/Character.h"

#include "entityComponentSystem/EntityTypes.hpp"

#include "AgentCharacter.generated.h"

UCLASS(config = Game)
class GALAVANTUNREAL_API AAgentCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	gv::Entity Entity;

	// Sets default values for this character's properties
	AAgentCharacter();

	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	// Called every frame
	virtual void Tick(float DeltaSeconds) override;

	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;
};
