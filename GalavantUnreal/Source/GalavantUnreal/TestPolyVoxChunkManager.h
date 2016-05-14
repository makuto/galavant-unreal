// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "GameFramework/Actor.h"

#include "TestPolyVoxChunkManager.generated.h"

class ATestPolyVoxChunk;

UCLASS()
class GALAVANTUNREAL_API ATestPolyVoxChunkManager : public AActor
{
	GENERATED_BODY()

	USceneComponent *SceneComponent;

	TArray<ATestPolyVoxChunk *> Chunks;
	
public:	
	// Sets default values for this actor's properties
	ATestPolyVoxChunkManager();

	// Called when the game starts or when spawned
	virtual void BeginPlay() override;
	
	// Called every frame
	virtual void Tick( float DeltaSeconds ) override;

	// Make sure Tick() is called in the editor
	virtual bool ShouldTickIfViewportsOnly() const;
	
};
