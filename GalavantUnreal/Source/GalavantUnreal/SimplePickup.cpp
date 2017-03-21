// The MIT License (MIT) Copyright (c) 2016 Macoy Madson

#include "GalavantUnreal.h"
#include "SimplePickup.h"

// Sets default values
ASimplePickup::ASimplePickup()
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if
	// you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	SphereComponent = CreateDefaultSubobject<USphereComponent>(TEXT("SphereComponent"));

	// I have no fucking clue why this doesn't actually enable it. I had to check the box in the
	//  blueprint in order to actually get physics
	SphereComponent->SetSimulatePhysics(true);

	// Establish component relationships
	RootComponent = SphereComponent;
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
