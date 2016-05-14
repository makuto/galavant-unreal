// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "GameFramework/Actor.h"

#include "CustomMeshComponent.h"
#include "Components/SceneComponent.h"

#include "TestPolyVoxChunk.generated.h"


UCLASS()
class GALAVANTUNREAL_API ATestPolyVoxChunk : public AActor
{
	GENERATED_BODY()

	TArray<FCustomMeshTriangle> Triangles;
	
	UCustomMeshComponent *Mesh;

	USceneComponent *SceneComponent;

	FVector LastUpdatedPosition;
	float 	TimeSinceLastUpdate;
	
public:	
	// Sets default values for this actor's properties
	ATestPolyVoxChunk();

	// Called when the game starts or when spawned
	virtual void BeginPlay() override;
	
	// Called every frame
	virtual void Tick( float DeltaSeconds ) override;

	// Make sure Tick() is called in the editor
	virtual bool ShouldTickIfViewportsOnly() const;

private:
	void ConstructForPosition(FVector Position, float noiseScale, int seed, float meshScale);
	
	
};
