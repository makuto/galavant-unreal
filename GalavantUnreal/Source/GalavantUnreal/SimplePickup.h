// The MIT License (MIT) Copyright (c) 2016 Macoy Madson

#pragma once

#include "GameFramework/Actor.h"

#include "entityComponentSystem/EntityTypes.hpp"

#include "SimplePickup.generated.h"

UCLASS()
class GALAVANTUNREAL_API ASimplePickup : public AActor
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere)
	USphereComponent *SphereComponent;

public:
	gv::Entity Entity;

	// Sets default values for this actor's properties
	ASimplePickup();

	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	// Called every frame
	virtual void Tick(float DeltaSeconds) override;
};
