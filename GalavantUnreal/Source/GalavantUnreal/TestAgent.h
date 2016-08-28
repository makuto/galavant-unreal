// The MIT License (MIT) Copyright (c) 2016 Macoy Madson

#pragma once

#include "GameFramework/Actor.h"
#include "TestAgent.generated.h"

UCLASS()
class GALAVANTUNREAL_API ATestAgent : public AActor
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere)
	USceneComponent *SceneComponent;

	UPROPERTY(EditAnywhere)
	UStaticMeshComponent *Mesh;
	
public:	
	// Sets default values for this actor's properties
	ATestAgent();

	// Called when the game starts or when spawned
	virtual void BeginPlay() override;
	
	// Called every frame
	virtual void Tick( float DeltaSeconds ) override;
};
