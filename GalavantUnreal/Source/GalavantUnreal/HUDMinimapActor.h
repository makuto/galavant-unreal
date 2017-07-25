// The MIT License (MIT) Copyright (c) 2016 Macoy Madson

#pragma once

#include "GameFramework/Actor.h"

#include "Graphics/DynamicTexture.h"

#include "HUDMinimapActor.generated.h"

UCLASS()
class GALAVANTUNREAL_API AHUDMinimapActor : public AActor
{
	GENERATED_BODY()

	DynamicTexture MinimapTexture;

	UPROPERTY(EditAnywhere)
	UStaticMeshComponent* MinimapMesh;

	int32 TextureWidth;
	int32 TextureHeight;

public:
	// Sets default values for this actor's properties
	AHUDMinimapActor();

	void UpdateMinimap(FVector& worldPosition, FRotator& rotation);

	virtual void PostInitializeComponents() override;

	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	// Called every frame
	virtual void Tick(float DeltaSeconds) override;
};
