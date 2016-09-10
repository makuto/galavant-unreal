#include "GalavantUnreal.h"

#include "TestMovementComponent.hpp"

#include "RandomStream.h"

TestMovementComponent::TestMovementComponent(void)
    : PooledComponentManager<TestMovementComponentData>(100)
{
}

TestMovementComponent::~TestMovementComponent(void)
{
}

void TestMovementComponent::Initialize(ATestAgent* defaultTestActor)
{
	DefaultActor = defaultTestActor;
}

AActor* TestMovementComponent::CreateDefaultActor(FVector& location)
{
	if (DefaultActor)
	{
		FActorSpawnParameters defaultSpawnParams;
		FRotator defaultRotation(0.f, 0.f, 0.f);

		return DefaultActor->Clone(location, defaultRotation, defaultSpawnParams);
	}

	return nullptr;
}

void TestMovementComponent::Update(float deltaSeconds)
{
	static FRandomStream randomStream;
	const float SPEED = 500.f;

	// UE_LOG(LogGalavantUnreal, Log, TEXT("Update TestMovementComponent"));

	PooledComponentManager::FragmentedPoolIterator it = PooledComponentManager::NULL_POOL_ITERATOR;
	for (PooledComponent<TestMovementComponentData>* currentComponent = ActivePoolBegin(it);
	     currentComponent != nullptr && it != PooledComponentManager::NULL_POOL_ITERATOR;
	     currentComponent = GetNextActivePooledComponent(it))
	{
		AActor* currentActor = currentComponent->data.Actor;
		FVector& currentPosition = currentComponent->data.Position;

		USceneComponent* sceneComponent = currentActor->GetRootComponent();

		FVector worldPosition = sceneComponent->GetComponentLocation();
		FVector deltaLocation(0.f, 0.f, 0.f);

		// If the component Position doesn't match the actor's actual position, have the actor
		//  move towards the component Position. Otherwise, pick a random new target (show you're
		//  alive)
		if (worldPosition.Equals(currentPosition, 100.f))
		{
			float randomRange = 250.f;
			currentPosition += FVector(randomStream.FRandRange(-randomRange, randomRange),
			                           randomStream.FRandRange(-randomRange, randomRange),
			                           randomStream.FRandRange(-randomRange, randomRange));
		}

		// UE_LOG(LogGalavantUnreal, Log,
		//        TEXT("Entity '%d' change position to %s (world position %s)"),
		//        currentComponent->entity, *currentPosition.ToString(), *worldPosition.ToString());

		deltaLocation = currentPosition - worldPosition;

		FVector deltaVelocity = deltaLocation.GetSafeNormal(0.1f);
		deltaVelocity *= SPEED * deltaSeconds;

		sceneComponent->AddLocalOffset(deltaVelocity, false, NULL, ETeleportType::None);
	}
}

void TestMovementComponent::SubscribeEntity(PooledComponent<TestMovementComponentData>& component)
{
	if (!component.data.Actor)
		component.data.Actor = CreateDefaultActor(component.data.Position);
}

void TestMovementComponent::UnsubscribeEntity(PooledComponent<TestMovementComponentData>& component)
{
	if (component.data.Actor)
		component.data.Actor->Destroy();
}
