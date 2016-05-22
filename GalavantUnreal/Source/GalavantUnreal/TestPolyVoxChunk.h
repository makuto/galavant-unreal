// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "GameFramework/Actor.h"

#include "CustomMeshComponent.h"
#include "Components/SceneComponent.h"

#include "TestPolyVoxChunk.generated.h"

/*struct ChunkConstructionParams
{
	float 	NoiseScale;
	float 	MeshScale;
	int 	Seed;
};*/


UCLASS()
class GALAVANTUNREAL_API ATestPolyVoxChunk : public AActor
{
	GENERATED_BODY()

	TArray<FCustomMeshTriangle> Triangles;
	
	UCustomMeshComponent *Mesh;

	USceneComponent *SceneComponent;

	FVector LastUpdatedPosition;
	float 	TimeSinceLastUpdate;

	// The length, width, and height of the Chunk in Unreal units (i.e. after mesh scaling)
	FVector ChunkSize;

	//ChunkConstructionParams ConstructionParams;
	
public:	
	// Sets default values for this actor's properties
	ATestPolyVoxChunk();

	// Called when the game starts or when spawned
	virtual void BeginPlay() override;
	
	// Called every frame
	virtual void Tick( float DeltaSeconds ) override;

	// Make sure Tick() is called in the editor
	virtual bool ShouldTickIfViewportsOnly() const;

	FVector& GetChunkSize();

	// Construct the chunk and geo for its current world position
	void Construct();

private:
	void ConstructForPosition(FVector Position, float noiseScale, int seed, float meshScale);
	
	
};
