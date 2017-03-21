// The MIT License (MIT) Copyright (c) 2016 Macoy Madson

#pragma once

#include "GameFramework/Actor.h"
#include "SimplePickup.generated.h"

UCLASS()
class GALAVANTUNREAL_API ASimplePickup : public AActor
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere)
	USphereComponent *SphereComponent;

public:
	// Sets default values for this actor's properties
	ASimplePickup();

	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	// Called every frame
	virtual void Tick(float DeltaSeconds) override;
};
