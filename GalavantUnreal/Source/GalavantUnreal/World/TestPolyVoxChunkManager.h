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

	float TimeSinceLastUpdate;
	FVector LastUpdatedPosition;

	bool PropertiesChanged;

	// All chunks within this radius around the ChunkManager will be spawned
	UPROPERTY(EditAnywhere)
	float ChunkSpawnRadius;

	// The maximum number of chunks that this ChunkManager can have exist at one time
	UPROPERTY(EditAnywhere)
	int32 MaxNumChunks;

	// Generate chunks using 3D simplex noise rather than 2D heightmap nosie
	UPROPERTY(EditAnywhere)
	bool Use3dNoise;

	FVector ChunkSize;

public:

	// Completely enable/disable procedural chunk generation
	UPROPERTY(EditAnywhere)
	bool EnableChunkCreation;

	// Sets default values for this actor's properties
	ATestPolyVoxChunkManager();

	virtual void Destroyed();

	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	// Called every frame
	virtual void Tick(float DeltaSeconds) override;

	// Make sure Tick() is called in the editor
	virtual bool ShouldTickIfViewportsOnly() const;

	virtual ATestPolyVoxChunk *CreateChunk(FVector &location, FRotator &rotation);

#if WITH_EDITOR
	// Ensure that if properties are changed, the manager immediately updates
	virtual void PostEditChangeProperty(struct FPropertyChangedEvent &e);
#endif

	void SetChunkSpawnRadius(float radius);

	FVector GetChunkManagerLocation();

private:
	void DestroyUnneededChunks();

	void DrawDebugVisualizations(TArray<FVector> *chunkPositions);
};
