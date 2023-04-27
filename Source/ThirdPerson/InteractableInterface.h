// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "InteractableInterface.generated.h"

class APawn;

// Any blueprint can implement this interface so player can Interact with it.  To do this
// 1. Open your blueprint
// 2. Click on the class setting tab
// 3. Add InteractableInterface in the details panel for Inherited Interface
// 4. Click the Add button and add Override Function and pick OnInteract 
// 5. See InteractableCube Blueprint as an example
// see https://docs.unrealengine.com/5.1/en-US/interfaces-in-unreal-engine/
UINTERFACE(MinimalAPI, Blueprintable)
class UInteractableInterface : public UInterface
{
    GENERATED_BODY()
};

// Any C++ class can inherit from this interface so player can Interact with it
class IInteractableInterface
{
    GENERATED_BODY()

public:
    // Here I implement the interact function as a Blueprint Callable Interface Functions
    
    // your blueprint can implement this function to respond to player interact
    // For example: Door Actor will open/close on interact
    // For example: A Pickup item will be attached to pawn on interact
    // See InteractableCube Blueprint as an example
    UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Interact")
    void OnInteract(APawn* InstigatorPawn);

    // Your C++ class can override this function to respond to player interact
    // For example: Door Actor will open/close on interact
    // For example: A Pickup item will be attached to pawn on interact
    // See: APickableActor C++ class
    // virtual void OnInteract_Implementation(APawn* InstigatorPawn);

    // You can also do C++ Only Interface Functions
    // Your C++ can override this if it doesn't want to be interacted with
    // You call it like so: Cast<IInteractableInterface>(YourActor)
    virtual bool IsInteractable() { return true; }
};
