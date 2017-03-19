// The MIT License (MIT) Copyright (c) 2016 Macoy Madson

#include "GalavantUnreal.h"
#include "SimplePickup.h"

// Sets default values
ASimplePickup::ASimplePickup()
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if
	// you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	SceneComponent = CreateDefaultSubobject<USceneComponent>(TEXT("SceneComponent"));

	// Establish component relationships
	RootComponent = SceneComponent;
}

// Called when the game starts or when spawned
void ASimplePickup::BeginPlay()
{
	Super::BeginPlay();
}

// Called every frame
void ASimplePickup::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}
