// The MIT License (MIT) Copyright (c) 2016 Macoy Madson

#include "GalavantUnreal.h"
#include "TestAgent.h"

// Sets default values
ATestAgent::ATestAgent()
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if
	// you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	SceneComponent = CreateDefaultSubobject<USceneComponent>(TEXT("SceneComponent"));
	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));

	// Establish component relationships
	RootComponent = SceneComponent;
	if (SceneComponent->CanAttachAsChild(Mesh, "Mesh"))
	{
		Mesh->SetupAttachment(SceneComponent, TEXT("Mesh"));
	}

	// Set default mesh and material
	static ConstructorHelpers::FObjectFinder<UStaticMesh> mesh(
	    TEXT("/Game/StarterContent/Props/SM_MatPreviewMesh_02.SM_MatPreviewMesh_02"));
	// TEXT("/Game/StarterContent/Architecture/Pillar_50x500.Pillar_50x500"));
	Mesh->SetStaticMesh(mesh.Object);

	static ConstructorHelpers::FObjectFinder<UMaterialInterface> material(
	    TEXT("/Game/Materials/SolidTestMaterial.SolidTestMaterial"));
	Mesh->SetMaterial(0, material.Object);
}

// Called when the game starts or when spawned
void ATestAgent::BeginPlay()
{
	Super::BeginPlay();
}

// Called every frame
void ATestAgent::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// FVector worldPosition = SceneComponent->GetComponentLocation();
	// FVector deltaLocation(0.f, 0.f, 0.f);

	// static float fLifetime = 0.f;

	// deltaLocation.Y = DeltaTime * 100.f;

	// SceneComponent->AddLocalOffset(deltaLocation, false, NULL, ETeleportType::None);
}

ATestAgent* ATestAgent::Clone(FVector& location, FRotator& rotation,
                              FActorSpawnParameters& spawnParams)
{
	ATestAgent* newTestAgent =
	    (ATestAgent*)GetWorld()->SpawnActor<ATestAgent>(location, rotation, spawnParams);
	return newTestAgent;
}