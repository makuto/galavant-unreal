// The MIT License (MIT) Copyright (c) 2016 Macoy Madson

#pragma once

#include "GameFramework/Actor.h"
#include "TestAgent.generated.h"

UCLASS()
class GALAVANTUNREAL_API ATestAgent : public AActor
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere)
	UStaticMeshComponent *Mesh;
	
public:	
	// Gross
	UPROPERTY(EditAnywhere)
	USceneComponent *SceneComponent;

	// Sets default values for this actor's properties
	ATestAgent();

	// Called when the game starts or when spawned
	virtual void BeginPlay() override;
	
	// Called every frame
	virtual void Tick( float DeltaSeconds ) override;

	ATestAgent* Clone(FVector& location, FRotator& rotation, FActorSpawnParameters& spawnParams);
};
