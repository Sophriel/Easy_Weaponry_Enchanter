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
#include "AbilitySystemComponent.h"
#include "EWE/EWELog.h"
#include "EasyWeaponryEnchanter/Weapon/EWEWeaponData.h"
#include "EasyWeaponryEnchanter/Attribute/EWEAttributeBase.h"
#include "EWEInventoryComponent.h"

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
    GetCharacterMovement()->bOrientRotationToMovement = true;            // Character moves in the direction of input...
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
    CameraBoom->TargetArmLength = 400.0f;       // The camera follows at this distance behind the character
    CameraBoom->bUsePawnControlRotation = true; // Rotate the arm based on the controller

    // Create a follow camera
    FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
    FollowCamera->SetupAttachment(
        CameraBoom, USpringArmComponent::SocketName); // Attach the camera to the end of the boom and let the boom
                                                      // adjust to match the controller orientation
    FollowCamera->bUsePawnControlRotation = false;    // Camera does not rotate relative to arm

    // Note: The skeletal mesh and anim blueprint references on the Mesh component (inherited from Character)
    // are set in the derived blueprint asset named ThirdPersonCharacter (to avoid direct content references in C++)

    InventoryComponent = CreateDefaultSubobject<UEWEInventoryComponent>(TEXT("Inventory Component"));

    WeaponMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("WeaponMesh"));
    WeaponMesh->SetupAttachment(GetMesh(), TEXT("WeaponSocket"));

    SubWeaponMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("SubWeaponMesh"));
    SubWeaponMesh->SetupAttachment(GetMesh(), TEXT("SubWeaponSocket"));

    WeaponAbilityComponent = CreateDefaultSubobject<UAbilitySystemComponent>(TEXT("Ability System Component"));
    WeaponAbilityComponent->SetIsReplicated(true);

    AttributeSet = CreateDefaultSubobject<UEWEAttributeBase>(TEXT("AttributeSet"));
}

void AEWECharacter::PossessedBy(AController *NewController)
{
    Super::PossessedBy(NewController);

    if (WeaponAbilityComponent)
    {
        WeaponAbilityComponent->InitAbilityActorInfo(this, this);

        // Set Attribute with GE after init ASC
        if (DefaultAttributeGE)
        {
            FGameplayEffectContextHandle Context = WeaponAbilityComponent->MakeEffectContext();
            Context.AddSourceObject(this);

            WeaponAbilityComponent->ApplyGameplayEffectToSelf(DefaultAttributeGE.GetDefaultObject(), 1.f, Context);
        }
    }

    SetOwner(NewController);
}

void AEWECharacter::OnRep_PlayerState()
{
    Super::OnRep_PlayerState();

    if (WeaponAbilityComponent)
    {
        WeaponAbilityComponent->InitAbilityActorInfo(this, this);
    }
}

void AEWECharacter::BeginPlay()
{
    Super::BeginPlay();

    // Setup inventory event subscriptions
    SetupInventorySubscriptions();

    // Enable ticking for target acquisition
    PrimaryActorTick.bCanEverTick = true;
}

void AEWECharacter::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    WeaponMesh->SetSkeletalMesh(nullptr);
    SubWeaponMesh->SetSkeletalMesh(nullptr);

    CurrentWeapon = nullptr;

    Super::EndPlay(EndPlayReason);
}

void AEWECharacter::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    // Only perform target acquisition on the local player's character
    if (!IsLocallyControlled())
    {
        return;
    }

    UpdateTargetAcquisition(DeltaTime);
}

#pragma region Input

void AEWECharacter::NotifyControllerChanged()
{
    Super::NotifyControllerChanged();

    // Add Input Mapping Context
    if (APlayerController *PlayerController = Cast<APlayerController>(Controller))
    {
        if (UEnhancedInputLocalPlayerSubsystem *Subsystem =
                ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PlayerController->GetLocalPlayer()))
        {
            Subsystem->AddMappingContext(DefaultMappingContext, 0);
            Subsystem->AddMappingContext(QuickSlotMappingContext, 1);
        }
    }
}

void AEWECharacter::SetupPlayerInputComponent(UInputComponent *PlayerInputComponent)
{
    // Set up action bindings
    if (UEnhancedInputComponent *EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent))
    {
        // Jumping
        EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Started, this, &ACharacter::Jump);
        EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Completed, this, &ACharacter::StopJumping);

        // Moving
        EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &AEWECharacter::Move);

        // Looking
        EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Triggered, this, &AEWECharacter::Look);

        // Camera
        EnhancedInputComponent->BindAction(ChangeCameraAction, ETriggerEvent::Triggered, this,
                                           &AEWECharacter::ChangeCamera);

        // Quick Slot
        uint8 ItemIndex = 0;
        for (TObjectPtr<UInputAction> Action : SelectItemActions)
        {
            EnhancedInputComponent->BindAction(Action, ETriggerEvent::Triggered, this, &AEWECharacter::SelectWeapon,
                                               ItemIndex);
            ++ItemIndex;
        }

        EnhancedInputComponent->BindAction(ScrollItemAction, ETriggerEvent::Triggered, this,
                                           &AEWECharacter::ScrollWeapon);

        EnhancedInputComponent->BindAction(AttackAction, ETriggerEvent::Triggered, this, &AEWECharacter::Attack);

        // Toggle Inventory
        EnhancedInputComponent->BindAction(ToggleInventoryAction, ETriggerEvent::Triggered, this,
                                           &AEWECharacter::ToggleInventory);
    }
    else
    {
        UE_LOG(LogEWECharacter, Error,
               TEXT("'%s' Failed to find an Enhanced Input component! This template is built to use the Enhanced Input "
                    "system. If you intend to use the legacy system, then you will need to update this C++ file."),
               *GetNameSafe(this));
    }
}

void AEWECharacter::Move(const FInputActionValue &Value)
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

void AEWECharacter::Look(const FInputActionValue &Value)
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

#pragma endregion

#pragma region Camera

void AEWECharacter::ChangeCamera(const FInputActionValue &Value)
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
    UEWEControlData *NewCharacterControl = CameraControl[CameraType];
    check(NewCharacterControl);

    SetCharacterControlData(NewCharacterControl);

    CurrentCameraType = CameraType;
}

void AEWECharacter::SetCharacterControlData(const UEWEControlData *ControlData)
{
    APlayerController *const PC = CastChecked<APlayerController>(Controller);
    ensure(PC);
    PC->SetShowMouseCursor(ControlData->bShowCursor);

    UCharacterMovementComponent *CharacterMovementComp = GetCharacterMovement();
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

#pragma endregion

#pragma region Weapon

UAbilitySystemComponent *AEWECharacter::GetAbilitySystemComponent() const { return WeaponAbilityComponent; }

void AEWECharacter::SetupInventorySubscriptions()
{
    if (!InventoryComponent)
    {
        EWE_LOG(LogEWECharacter, Warning, TEXT("InventoryComponent not found"));
        return;
    }

    InventoryComponent->OnWeaponAcquired.AddDynamic(this, &AEWECharacter::HandleWeaponAcquired);
    InventoryComponent->OnWeaponsSynced.AddDynamic(this, &AEWECharacter::HandleWeaponsSynced);
    InventoryComponent->OnWeaponRemoved.AddDynamic(this, &AEWECharacter::HandleWeaponRemoved);

    EWE_LOG(LogEWECharacter, Log, TEXT("Inventory subscriptions setup complete"));
}

void AEWECharacter::ClearInventorySubscriptions()
{
    if (InventoryComponent)
    {
        InventoryComponent->OnWeaponAcquired.RemoveAll(this);
        InventoryComponent->OnWeaponsSynced.RemoveAll(this);
        InventoryComponent->OnWeaponRemoved.RemoveAll(this);

        EWE_LOG(LogEWECharacter, Log, TEXT("Inventory subscriptions cleared"));
    }
}

void AEWECharacter::HandleWeaponAcquired(UEWEWeaponData *Weapon)
{
    AEWEPlayerController *PlayerController = Cast<AEWEPlayerController>(Controller);
    if (PlayerController)
    {
        PlayerController->AddWeapon(Weapon);
    }
}

void AEWECharacter::HandleWeaponsSynced(const TArray<UEWEWeaponData *> &Weapons, int32 Count)
{
    AEWEPlayerController *PlayerController = Cast<AEWEPlayerController>(Controller);
    if (PlayerController)
    {
        PlayerController->SyncInventoryWeapons(Weapons);
    }
}

void AEWECharacter::HandleWeaponRemoved(UEWEWeaponData *Weapon, int32 Index)
{
    EWE_LOG(LogEWECharacter, Log, TEXT("Weapon removed: %s at index %d"), *Weapon->GetName(), Index);
}

void AEWECharacter::ToggleInventory()
{
    AEWEPlayerController *PlayerController = Cast<AEWEPlayerController>(Controller);
    if (!PlayerController)
    {
        EWE_LOG(LogEWECharacter, Warning, TEXT("PlayerController not found"));
        return;
    }

    PlayerController->ToggleInventory();
}

void AEWECharacter::AcquireWeapon(UEWEWeaponData *NewWeapon)
{
    // Deprecated: Event-based system now handles this via InventoryComponent
    // This function is kept for interface compatibility
}

void AEWECharacter::EquipWeapon(UEWEWeaponData *Weapon)
{
    WeaponAbilityComponent->RemoveAllSpawnedAttributes();
    WeaponAbilityComponent->ClearAllAbilities();

    if (Weapon == nullptr)
    {
        WeaponMesh->SetSkeletalMesh(nullptr);
        return;
    }

    CurrentWeapon = Weapon;

    // Mesh
    if (!Weapon->WeaponMesh)
    {
        Weapon->LoadWeaponAssets();
    }

    WeaponMesh->SetSkeletalMesh(Weapon->WeaponMesh);

    // Ability
    int32 HoldAbilityIndex = CHoldAbilityIndex;
    for (const auto &Ability : CurrentWeapon->HoldAbilities)
    {
        FGameplayAbilitySpec AbilitySpec(Ability, 0, HoldAbilityIndex);
        WeaponAbilityComponent->GiveAbility(AbilitySpec);
        ++HoldAbilityIndex;
    }

    int32 HitAbilityIndex = CHitAbilityIndex;
    for (const auto &Ability : CurrentWeapon->HitAbilities)
    {
        FGameplayAbilitySpec AbilitySpec(Ability, 0, HitAbilityIndex);
        WeaponAbilityComponent->GiveAbility(AbilitySpec);
        ++HitAbilityIndex;
    }
}

void AEWECharacter::SelectWeapon(const uint8 SlotIndex)
{
    AEWEPlayerController *PlayerController = Cast<AEWEPlayerController>(Controller);
    ensure(PlayerController);

    PlayerController->SelectSlot(SlotIndex);
}

void AEWECharacter::ScrollWeapon(const FInputActionValue &Value)
{
    AEWEPlayerController *PlayerController = Cast<AEWEPlayerController>(Controller);
    ensure(PlayerController);

    float ScrollDirection = -Value.Get<float>();
    PlayerController->ScrollSlot(ScrollDirection);
}

void AEWECharacter::Attack(const FInputActionValue &Value) { K2_Attack(); }

void AEWECharacter::AttackHold()
{
    // Activate Hold Ability
    if (!WeaponAbilityComponent || !CurrentWeapon)
    {
        return;
    }

    // Activate Abilities from CHoldAbilityIndex
    int32 HoldAbilityCount = CurrentWeapon->HoldAbilities.Num();
    for (int32 AbilityIndex = CHoldAbilityIndex; AbilityIndex < HoldAbilityCount; ++AbilityIndex)
    {
        WeaponAbilityComponent->AbilityLocalInputPressed(AbilityIndex);
    }
}

void AEWECharacter::AttackHitCheck()
{
    // Release Hold Ability
    int32 HoldAbilityCount = CurrentWeapon->HoldAbilities.Num();
    for (int32 AbilityIndex = CHoldAbilityIndex; AbilityIndex < HoldAbilityCount; ++AbilityIndex)
    {
        WeaponAbilityComponent->AbilityLocalInputReleased(AbilityIndex);
    }

    // Activate Hit Ability
    int32 HitAbilityCount = CurrentWeapon->HitAbilities.Num();
    for (int32 AbilityIndex = CHitAbilityIndex; AbilityIndex < CHitAbilityIndex + HitAbilityCount; ++AbilityIndex)
    {
        WeaponAbilityComponent->AbilityLocalInputPressed(AbilityIndex);
    }
}

void AEWECharacter::AttackReleaseCheck()
{
    int32 HitAbilityCount = CurrentWeapon->HitAbilities.Num();
    for (int32 AbilityIndex = 0; AbilityIndex < HitAbilityCount; ++AbilityIndex)
    {
        WeaponAbilityComponent->AbilityLocalInputReleased(AbilityIndex);
    }
}

#pragma endregion

#pragma region Status

UEWEAttributeBase *AEWECharacter::GetCharacterAttribute() { return AttributeSet; }

#pragma endregion

#pragma region TargetAcquisition

void AEWECharacter::UpdateTargetAcquisition(float DeltaTime)
{
    TargetTraceAccumulator += DeltaTime;
    if (TargetTraceAccumulator < TargetTraceInterval)
    {
        return;
    }
    TargetTraceAccumulator = 0.f;

    if (!FollowCamera)
    {
        return;
    }

    const FVector CharacterLocation = GetActorLocation();
    const FVector CameraLocation = FollowCamera->GetComponentLocation();
    const FRotator CameraRotation = FollowCamera->GetComponentRotation();
    const FVector TraceEnd = CharacterLocation + (CameraRotation.Vector() * TargetTraceRange);

    FHitResult HitResult;
    FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(TargetTrace), true, this);
    QueryParams.bReturnPhysicalMaterial = false;

    const bool bHit =
        GetWorld()->LineTraceSingleByChannel(HitResult, CameraLocation, TraceEnd, ECC_Visibility, QueryParams);

#if ENABLE_DRAW_DEBUG

    FColor DrawColor = bHit ? FColor::Green : FColor::Red;

    DrawDebugLine(GetWorld(), CameraLocation, TraceEnd, DrawColor, false, 0.05f);

#endif

    AActor *NewTarget = nullptr;

    if (bHit && HitResult.GetActor())
    {
        AActor *HitActor = HitResult.GetActor();

        // Don't target yourself
        if (HitActor != this)
        {
            NewTarget = HitActor;
        }
    }

    // Only notify if the target has actually changed
    if (NewTarget != FocusedTarget)
    {
        FocusedTarget = NewTarget;
        NotifyTargetChanged(NewTarget);
    }
}

void AEWECharacter::NotifyTargetChanged(AActor *NewTarget)
{
    AEWEPlayerController *PC = Cast<AEWEPlayerController>(GetController());
    if (!PC)
    {
        return;
    }

    if (NewTarget)
    {
        AEWECharacter *TargetChar = Cast<AEWECharacter>(NewTarget);
        if (TargetChar)
        {
            UEWEAttributeBase *TargetAttr = TargetChar->GetCharacterAttribute();
            FText TargetName = FText::FromString(NewTarget->GetActorLabel());

            PC->UpdateTargetInfo(TargetName, TargetAttr);
        }
        else
        {
            FText TargetName = FText::FromString(NewTarget->GetActorLabel());

            PC->UpdateTargetName(TargetName);
        }
    }
    else
    {
        PC->ClearTargetInfo();
    }
}

#pragma endregion