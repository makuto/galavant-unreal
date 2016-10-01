// The MIT License (MIT) Copyright (c) 2016 Macoy Madson

#include "GalavantUnreal.h"
#include "GalavantUnrealMain.h"

#include "entityComponentSystem/EntityTypes.hpp"

// Sets default values
AGalavantUnrealMain::AGalavantUnrealMain()
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if
	// you don't need it.
	PrimaryActorTick.bCanEverTick = true;
}

void AGalavantUnrealMain::InitializeGalavant(void)
{
	FVector location(0.f, 0.f, 0.f);
	FRotator rotation(0.f, 0.f, 0.f);
	FActorSpawnParameters spawnParams;

	ATestAgent* testAgent =
	    (ATestAgent*)GetWorld()->SpawnActor<ATestAgent>(location, rotation, spawnParams);

	TestMovementComponentManager.Initialize(testAgent);

	EntityComponentSystem.AddComponentManager(&TestMovementComponentManager);

	// Create a couple test entities
	int numTestEntities = 20;
	EntityList testEntities;
	EntityComponentSystem.GetNewEntities(testEntities, numTestEntities);

	TestMovementComponent::TestMovementComponentList newEntityMovementComponents;
	newEntityMovementComponents.resize(numTestEntities);

	int i = 0;
	for (EntityListIterator it = testEntities.begin(); it != testEntities.end(); ++it)
	{
		newEntityMovementComponents[i].entity = (*it);
		newEntityMovementComponents[i].data.Actor = nullptr;
		newEntityMovementComponents[i].data.Position = FVector(0.f, i * 500.f, 0.f);
		i++;
	}

	TestMovementComponentManager.SubscribeEntities(newEntityMovementComponents);
}

// Called when the game starts or when spawned
void AGalavantUnrealMain::BeginPlay()
{
	Super::BeginPlay();

	// Should this be here or in the constructor?
	InitializeGalavant();
}

// Called every frame
void AGalavantUnrealMain::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	GalavantMain.Update(DeltaTime);

	TestMovementComponentManager.Update(DeltaTime);

	EntityComponentSystem.DestroyEntitiesPendingDestruction();
}
