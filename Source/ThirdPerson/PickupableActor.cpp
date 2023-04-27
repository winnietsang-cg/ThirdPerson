// Fill out your copyright notice in the Description page of Project Settings.


#include "PickupableActor.h"

// Sets default values
APickupableActor::APickupableActor()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

}

// Called when the game starts or when spawned
void APickupableActor::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void APickupableActor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

void APickupableActor::OnInteract_Implementation(APawn* InstigatorPawn)
{
	if (!bIsPickup)
	{
		// You can also implement a inventory to keep track of things, say your Pawn has a UInventoryComponent
		// UInventoryComponent* Inventory = InstigatorPawn->FindByComponent<UInventoryComponent>
		// Inventory->AddToInventory(this);
		
		// Pick up
		bIsPickup = true;
		// See note on this https://forums.unrealengine.com/t/set-simulate-physics-and-attachactortocomponent-problem/326677/8
		// The Primitive with simulate phyics to true needs to be the RootComponent 
		// You do this by dragging the component with physics over the DefaultSceneRoot in the Blueprint
		if (UPrimitiveComponent* RootPrimitiveCmponent = Cast<UPrimitiveComponent>(GetRootComponent()))
		{
			RootPrimitiveCmponent->SetSimulatePhysics(false);
		}
		for (auto Component : GetComponents())
		{
			// Disable collision
			if (UPrimitiveComponent* PrimitiveComp = Cast<UPrimitiveComponent>(Component))
			{
				PrimitiveComp->SetCollisionEnabled(ECollisionEnabled::NoCollision);
			}
		}
		AttachToActor(InstigatorPawn, FAttachmentTransformRules::SnapToTargetNotIncludingScale, "hand_lSocket");
	}
	else
	{
		// Drop the pickup
		bIsPickup = false;
		DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
		// See note on this https://forums.unrealengine.com/t/set-simulate-physics-and-attachactortocomponent-problem/326677/8
		// The Primitive with simulate phyics to true needs to be the RootComponent 
		if (UPrimitiveComponent* RootPrimitiveCmponent = Cast<UPrimitiveComponent>(GetRootComponent()))
		{
			RootPrimitiveCmponent->SetSimulatePhysics(true);
		}
		// Enable collision on all components
		for (auto Component : GetComponents())
		{
			if (UPrimitiveComponent* PrimitiveComp = Cast<UPrimitiveComponent>(Component))
			{
				PrimitiveComp->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
			}
		}
	}
}
