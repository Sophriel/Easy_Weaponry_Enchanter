// Copyright Epic Games, Inc. All Rights Reserved.

#include "EWECharacter.h"
#include "Engine/LocalPlayer.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "GameFramework/Controller.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputActionValue.h"
#include "EWEControlData.h"
#include "EWEPlayerController.h"

DEFINE_LOG_CATEGORY(LogEWECharacter);

//////////////////////////////////////////////////////////////////////////
// AEWECharacter

AEWECharacter::AEWECharacter()
{
	// Set size for collision capsule
	GetCapsuleComponent()->InitCapsuleSize(42.f, 96.0f);

	// Don't rotate when the controller rotates. Let that just affect the camera.
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	CurrentCameraType = ECameraType::Shoulder;

	// Configure character movement
	GetCharacterMovement()->bOrientRotationToMovement = true;			 // Character moves in the direction of input...
	GetCharacterMovement()->RotationRate = FRotator(0.0f, 500.0f, 0.0f); // ...at this rotation rate

	// Note: For faster iteration times these variables, and many more, can be tweaked in the Character Blueprint
	// instead of recompiling to adjust them
	GetCharacterMovement()->JumpZVelocity = 700.f;
	GetCharacterMovement()->AirControl = 0.35f;
	GetCharacterMovement()->MaxWalkSpeed = 500.f;
	GetCharacterMovement()->MinAnalogWalkSpeed = 20.f;
	GetCharacterMovement()->BrakingDecelerationWalking = 2000.f;
	GetCharacterMovement()->BrakingDecelerationFalling = 1500.0f;

	// Create a camera boom (pulls in towards the player if there is a collision)
	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->TargetArmLength = 400.0f;		// The camera follows at this distance behind the character
	CameraBoom->bUsePawnControlRotation = true; // Rotate the arm based on the controller

	// Create a follow camera
	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName); // Attach the camera to the end of the boom and let the boom adjust to match the controller orientation
	FollowCamera->bUsePawnControlRotation = false;								// Camera does not rotate relative to arm

	// Note: The skeletal mesh and anim blueprint references on the Mesh component (inherited from Character)
	// are set in the derived blueprint asset named ThirdPersonCharacter (to avoid direct content references in C++)
}

//////////////////////////////////////////////////////////////////////////
// Input

void AEWECharacter::NotifyControllerChanged()
{
	Super::NotifyControllerChanged();

	// Add Input Mapping Context
	if (APlayerController* PlayerController = Cast<APlayerController>(Controller))
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PlayerController->GetLocalPlayer()))
		{
			Subsystem->AddMappingContext(DefaultMappingContext, 0);
			Subsystem->AddMappingContext(QuickSlotMappingContext, 1);
		}
	}
}

void AEWECharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	// Set up action bindings
	if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent))
	{
		// Jumping
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Started, this, &ACharacter::Jump);
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Completed, this, &ACharacter::StopJumping);

		// Moving
		EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &AEWECharacter::Move);

		// Looking
		EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Triggered, this, &AEWECharacter::Look);

		// Camera
		EnhancedInputComponent->BindAction(ChangeCameraAction, ETriggerEvent::Triggered, this, &AEWECharacter::ChangeCamera);

		// Quick Slot
		uint8 ItemIndex = 0;
		for (TObjectPtr<UInputAction> Action : SelectItemActions)
		{
			EnhancedInputComponent->BindAction(Action, ETriggerEvent::Triggered, this, &AEWECharacter::SelectItem, ItemIndex);
			++ItemIndex;
		}

		EnhancedInputComponent->BindAction(ScrollItemAction, ETriggerEvent::Triggered, this, &AEWECharacter::ScrollItem);
	}
	else
	{
		UE_LOG(LogEWECharacter, Error, TEXT("'%s' Failed to find an Enhanced Input component! This template is built to use the Enhanced Input system. If you intend to use the legacy system, then you will need to update this C++ file."), *GetNameSafe(this));
	}
}

void AEWECharacter::Move(const FInputActionValue& Value)
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

void AEWECharacter::Look(const FInputActionValue& Value)
{
	if (CurrentCameraType == ECameraType::Quarter)
	{
		return;
	}

	// input is a Vector2D
	FVector2D LookAxisVector = Value.Get<FVector2D>();

	if (Controller != nullptr)
	{
		// add yaw and pitch input to controller
		AddControllerYawInput(LookAxisVector.X);
		AddControllerPitchInput(LookAxisVector.Y);
	}
}

void AEWECharacter::ChangeCamera(const FInputActionValue& Value)
{
	if (CurrentCameraType == ECameraType::Quarter)
	{
		SetCharacterControl(ECameraType::Shoulder);
	}
	else if (CurrentCameraType == ECameraType::Shoulder)
	{
		Controller->SetControlRotation(FRotator::ZeroRotator);
		SetCharacterControl(ECameraType::Quarter);
	}
}

void AEWECharacter::SetCharacterControl(ECameraType CameraType)
{
	UEWEControlData* NewCharacterControl = CameraControl[CameraType];
	check(NewCharacterControl);

	SetCharacterControlData(NewCharacterControl);

	CurrentCameraType = CameraType;
}

void AEWECharacter::SetCharacterControlData(const UEWEControlData* ControlData)
{
	APlayerController* const PC = CastChecked<APlayerController>(Controller);
	ensure(PC);
	PC->SetShowMouseCursor(ControlData->bShowCursor);

	UCharacterMovementComponent* CharacterMovementComp = GetCharacterMovement();
	CharacterMovementComp->bOrientRotationToMovement = ControlData->bOrientRotationToMovement;
	CharacterMovementComp->bUseControllerDesiredRotation = ControlData->bUseControllerDesiredRotation;
	CharacterMovementComp->RotationRate = ControlData->RotationRate;

	CameraBoom->TargetArmLength = ControlData->TargetArmLength;
	CameraBoom->SetRelativeRotation(ControlData->RelativeRotation);
	CameraBoom->bUsePawnControlRotation = ControlData->bUsePawnControlRotation;
	CameraBoom->bInheritPitch = ControlData->bInheritPitch;
	CameraBoom->bInheritYaw = ControlData->bInheritYaw;
	CameraBoom->bInheritRoll = ControlData->bInheritRoll;
	CameraBoom->bDoCollisionTest = ControlData->bDoCollisionTest;
}

void AEWECharacter::SelectItem(const FInputActionValue& Value, const uint8 SlotIndex)
{
	AEWEPlayerController* PlayerController = Cast<AEWEPlayerController>(Controller);
	ensure(PlayerController);

	PlayerController->SelectSlot(SlotIndex);
}

void AEWECharacter::ScrollItem(const FInputActionValue& Value)
{
	AEWEPlayerController* PlayerController = Cast<AEWEPlayerController>(Controller);
	ensure(PlayerController);

	float ScrollDirection = Value.Get<float>();
	PlayerController->ScrollSlot(ScrollDirection);
}
