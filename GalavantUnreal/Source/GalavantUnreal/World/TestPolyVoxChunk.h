// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "GameFramework/Actor.h"

#include "CustomMeshComponent.h"
#include "Components/SceneComponent.h"

#include "Components/GeneratedMeshComponent.h"

#include "TestPolyVoxChunk.generated.h"

UCLASS()
class GALAVANTUNREAL_API ATestPolyVoxChunk : public AActor
{
	GENERATED_BODY()

	TArray<FGeneratedMeshTriangle> Triangles;

	UCustomMeshComponent *Mesh;
	USceneComponent *SceneComponent;

	FVector LastUpdatedPosition;
	float TimeSinceLastUpdate;
	bool PropertiesChanged;  // If any UProperties change, this is set to true

	// The length, width, and height of the Chunk in Unreal units (i.e. after mesh scaling)
	FVector ChunkSize;

	// Use 3D simplex noise to generate the chunk rather than 2D heightmap noise
	UPROPERTY(EditAnywhere)
	bool Use3dNoise;

public:
	// Allow viewing/changing the Material of the generated mesh in editor (if placed in a level at
	// construction)
	UPROPERTY(VisibleAnywhere, Category = Materials)
	UGeneratedMeshComponent *GeneratedMesh;

	// Sets default values for this actor's properties
	ATestPolyVoxChunk();

	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	// Called every frame
	virtual void Tick(float DeltaSeconds) override;

	// Make sure Tick() is called in the editor
	virtual bool ShouldTickIfViewportsOnly() const;

#if WITH_EDITOR
	// Ensure that if properties change, the chunk immediately updates
	virtual void PostEditChangeProperty(struct FPropertyChangedEvent &e);
#endif

	virtual void Destroyed();

	FVector &GetChunkSize();

	// Construct the chunk and geo for its current world position
	void Construct();

	void StopConstruction();

	void SetUse3dNoise(bool newValue);
	void SetAsyncConstruction(bool newValue);
};
