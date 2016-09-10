// The MIT License (MIT) Copyright (c) 2016 Macoy Madson

#pragma once

#include "GameFramework/Actor.h"

#include "GalavantMain.hpp"

#include "entityComponentSystem/EntityComponentManager.hpp"

// Testing
#include "GalaEntityComponents/TestMovementComponent.hpp"

#include "GalavantUnrealMain.generated.h"

UCLASS()
class GALAVANTUNREAL_API AGalavantUnrealMain : public AActor
{
	GENERATED_BODY()

	gv::GalavantMain GalavantMain;

	EntityComponentManager EntityComponentSystem;
	TestMovementComponent TestMovementComponentManager;

	void InitializeGalavant(void);

public:
	// Sets default values for this actor's properties
	AGalavantUnrealMain();

	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	// Called every frame
	virtual void Tick(float DeltaSeconds) override;
};
