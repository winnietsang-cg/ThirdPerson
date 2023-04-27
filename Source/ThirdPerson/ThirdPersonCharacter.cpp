// Copyright Epic Games, Inc. All Rights Reserved.

#include "ThirdPersonCharacter.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/InputComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Controller.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/SpringArmComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"

#include "InteractableInterface.h"
//////////////////////////////////////////////////////////////////////////
// AThirdPersonCharacter

AThirdPersonCharacter::AThirdPersonCharacter()
{
	// Set size for collision capsule
	GetCapsuleComponent()->InitCapsuleSize(42.f, 96.0f);
		
	// Don't rotate when the controller rotates. Let that just affect the camera.
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	// Configure character movement
	GetCharacterMovement()->bOrientRotationToMovement = true; // Character moves in the direction of input...	
	GetCharacterMovement()->RotationRate = FRotator(0.0f, 500.0f, 0.0f); // ...at this rotation rate

	// Note: For faster iteration times these variables, and many more, can be tweaked in the Character Blueprint
	// instead of recompiling to adjust them
	GetCharacterMovement()->JumpZVelocity = 700.f;
	GetCharacterMovement()->AirControl = 0.35f;
	GetCharacterMovement()->MaxWalkSpeed = 500.f;
	GetCharacterMovement()->MinAnalogWalkSpeed = 20.f;
	GetCharacterMovement()->BrakingDecelerationWalking = 2000.f;

	// Create a camera boom (pulls in towards the player if there is a collision)
	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->TargetArmLength = 400.0f; // The camera follows at this distance behind the character	
	CameraBoom->bUsePawnControlRotation = true; // Rotate the arm based on the controller

	// Create a follow camera
	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName); // Attach the camera to the end of the boom and let the boom adjust to match the controller orientation
	FollowCamera->bUsePawnControlRotation = false; // Camera does not rotate relative to arm

	// Note: The skeletal mesh and anim blueprint references on the Mesh component (inherited from Character) 
	// are set in the derived blueprint asset named ThirdPersonCharacter (to avoid direct content references in C++)
}

void AThirdPersonCharacter::BeginPlay()
{
	// Call the base class  
	Super::BeginPlay();

	//Add Input Mapping Context
	if (APlayerController* PlayerController = Cast<APlayerController>(Controller))
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PlayerController->GetLocalPlayer()))
		{
			Subsystem->AddMappingContext(DefaultMappingContext, 0);
		}
	}
}

//////////////////////////////////////////////////////////////////////////
// Input

void AThirdPersonCharacter::SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent)
{
	// Set up action bindings
	if (UEnhancedInputComponent* EnhancedInputComponent = CastChecked<UEnhancedInputComponent>(PlayerInputComponent)) {
		
		//Jumping
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Triggered, this, &ACharacter::Jump);
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Completed, this, &ACharacter::StopJumping);

		//Moving
		EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &AThirdPersonCharacter::Move);

		//Looking
		EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Triggered, this, &AThirdPersonCharacter::Look);

		//Interact
		EnhancedInputComponent->BindAction(InteractAction, ETriggerEvent::Triggered, this, &AThirdPersonCharacter::Interact);
	}

}

void AThirdPersonCharacter::Move(const FInputActionValue& Value)
{
	// input is a Vector2D
	FVector2D MovementVector = Value.Get<FVector2D>();

	if (Controller != nullptr)
	{
		// find out which way is forward
		const FRotator Rotation = Controller->GetControlRotation();
		const FRotator YawRotation(0, Rotation.Yaw, 0);

		// get forward vector
		const FVector ForwardDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
	
		// get right vector 
		const FVector RightDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

		// add movement 
		AddMovementInput(ForwardDirection, MovementVector.Y);
		AddMovementInput(RightDirection, MovementVector.X);
	}
}

void AThirdPersonCharacter::Look(const FInputActionValue& Value)
{
	// input is a Vector2D
	FVector2D LookAxisVector = Value.Get<FVector2D>();

	if (Controller != nullptr)
	{
		// add yaw and pitch input to controller
		AddControllerYawInput(LookAxisVector.X);
		AddControllerPitchInput(LookAxisVector.Y);
	}
}

 void AThirdPersonCharacter::Interact(const FInputActionValue& Value)
 {
	// Interact with actors that are attached to you first (i.e. drop it)
	// Alternatively, you can keep an array of things you pick up also
	// You could implement a UInventoryComponent for the player, see the code in PickupActor::OnInteract_Implementation()
	bool bIsInteractHandled = false;
	TArray<AActor*> AttachedActors;
	GetAttachedActors(AttachedActors);
	for (AActor* AttachActor : AttachedActors)
	{
		if (AttachActor != nullptr && AttachActor->Implements<UInteractableInterface>())
		{
			IInteractableInterface::Execute_OnInteract(AttachActor, this);
			bIsInteractHandled = true;
		}
	}

	if (!bIsInteractHandled)
	{
		// If you look at DefaultEngine.ini you can see that ECC_GameTraceChannel1 is mapped to Interactable
		FCollisionResponseParams ResponseParams(ECR_Ignore);
		ResponseParams.CollisionResponse.SetResponse(ECC_GameTraceChannel1, ECR_Block);

		// I used a capsule so that matches my character's radius and height
		float CapsuleRight, CapsuleHeight;
		GetSimpleCollisionCylinder(CapsuleRight, CapsuleHeight);
		// I made it slightly larger than the pawn's capsule to test for overlap
		const FCollisionShape CollisionShape = FCollisionShape::MakeCapsule(CapsuleRight * 1.01f, CapsuleHeight);
		FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(InteractdableCollisionQuery), false);

		// There are many different physics traces: line, sweep, overlap.  I will use overlap here
		TArray<FOverlapResult> Overlaps;
		// Test for overlap 
		GetWorld()->OverlapMultiByChannel(Overlaps, GetActorLocation(), FQuat::Identity, ECC_Pawn, CollisionShape, QueryParams, ResponseParams);
		for (const FOverlapResult& OverlapResult : Overlaps)
		{
			// My Character overlapped with an interactable
			if (OverlapResult.GetActor()->Implements<UInteractableInterface>())
			{
				UE_LOG(LogTemp, Log, TEXT("Interact with: %s"), *GetNameSafe(OverlapResult.GetActor()));
				// Blueprint that implement the interface cannot be cast, it is for C++ class that implement IInteractableInterface only
				IInteractableInterface* Interactable = Cast<IInteractableInterface>(OverlapResult.GetActor());
				if (Interactable == nullptr || Interactable->IsInteractable())
				{
					// This calls the blueprint native event OnInterface
					IInteractableInterface::Execute_OnInteract(OverlapResult.GetActor(), this);
				}
			}
		}

		// Player didn't overlap with any object in the Interactable channel, do a trace to see if the camera is point at any interacts in the distance
		if (Overlaps.Num() == 0)
		{
			APlayerController* const PC = Cast<APlayerController>(GetController());
			const APlayerCameraManager* PlayerCameraManager = PC->PlayerCameraManager;
			FVector POVLocation; FRotator POVRotation;
			PlayerCameraManager->GetCameraViewPoint(POVLocation, POVRotation);
			FHitResult HitResult;
			GetWorld()->SweepSingleByChannel(HitResult, POVLocation, POVLocation + POVRotation.Vector() * InteractDistance, FQuat::Identity, ECC_Pawn, CollisionShape, QueryParams, ResponseParams);
			if (HitResult.bBlockingHit && HitResult.GetActor()->Implements<UInteractableInterface>())
			{
				// Blueprint that implement the interface cannot be cast, it is for C++ class that implement IInteractableInterface only
				IInteractableInterface* Interactable = Cast<IInteractableInterface>(HitResult.GetActor());
				if (Interactable == nullptr || Interactable->IsInteractable())
				{
					UE_LOG(LogTemp, Log, TEXT("Interact with: %s"), *GetNameSafe(HitResult.GetActor()));
					// This calls the blueprint native event OnInterface
					IInteractableInterface::Execute_OnInteract(HitResult.GetActor(), this);
				}
			}
		}
	}
	
 }

