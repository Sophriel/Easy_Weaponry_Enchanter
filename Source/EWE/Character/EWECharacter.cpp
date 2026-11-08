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
#include "Abilities/GameplayAbility.h"
#include "GameplayAbilitySpec.h"
#include "GameplayEffect.h"
#include "GameplayEffectTypes.h"
#include "Net/UnrealNetwork.h"
#include "EWE/EWELog.h"
#include "EasyWeaponryEnchanter/Weapon/EWEWeaponData.h"
#include "EasyWeaponryEnchanter/Weapon/EWEWeaponInstance.h"
#include "EWEWeaponMeshComponent.h"
#include "EasyWeaponryEnchanter/Attribute/EWEAttributeBase.h"
#include "EWEInventoryComponent.h"

AEWECharacter::AEWECharacter()
{
    GetCapsuleComponent()->InitCapsuleSize(42.f, 96.0f);

    // Don't rotate when the controller rotates. Let that just affect the camera.
    bUseControllerRotationPitch = false;
    bUseControllerRotationYaw = false;
    bUseControllerRotationRoll = false;

    CurrentCameraType = ECameraType::Shoulder;

    // The inventory component registers weapon instances as replicated sub-objects.
    // The owner actor must use the registered sub-object list for them to replicate.
    bReplicateUsingRegisteredSubObjectList = true;

    GetCharacterMovement()->bOrientRotationToMovement = true;
    GetCharacterMovement()->RotationRate = FRotator(0.0f, 500.0f, 0.0f);

    // Note: For faster iteration times these variables, and many more, can be tweaked in the Character Blueprint
    // instead of recompiling to adjust them
    GetCharacterMovement()->JumpZVelocity = 700.f;
    GetCharacterMovement()->AirControl = 0.35f;
    GetCharacterMovement()->MaxWalkSpeed = 500.f;
    GetCharacterMovement()->MinAnalogWalkSpeed = 20.f;
    GetCharacterMovement()->BrakingDecelerationWalking = 2000.f;
    GetCharacterMovement()->BrakingDecelerationFalling = 1500.0f;

    CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
    CameraBoom->SetupAttachment(RootComponent);
    CameraBoom->TargetArmLength = 400.0f;
    CameraBoom->bUsePawnControlRotation = true;

    FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
    FollowCamera->SetupAttachment(
        CameraBoom, USpringArmComponent::SocketName);
    FollowCamera->bUsePawnControlRotation = false;

    // Note: The skeletal mesh and anim blueprint references on the Mesh component (inherited from Character)
    // are set in the derived blueprint asset named ThirdPersonCharacter (to avoid direct content references in C++)

    InventoryComponent = CreateDefaultSubobject<UEWEInventoryComponent>(TEXT("Inventory Component"));

    SubWeaponMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("SubWeaponMesh"));
    SubWeaponMesh->SetupAttachment(GetMesh(), TEXT("SubWeaponSocket"));

    // Display-only hand weapon: no collision, attached to the hand socket.
    WeaponMesh = CreateDefaultSubobject<UEWEWeaponMeshComponent>(TEXT("WeaponMesh"));
    WeaponMesh->SetupAttachment(GetMesh(), TEXT("WeaponSocket"));
    WeaponMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

    WeaponAbilityComponent = CreateDefaultSubobject<UAbilitySystemComponent>(TEXT("Ability System Component"));
    WeaponAbilityComponent->SetIsReplicated(true);

    AttributeSet = CreateDefaultSubobject<UEWEAttributeBase>(TEXT("AttributeSet"));
}

void AEWECharacter::PossessedBy(AController *NewController)
{
    Super::PossessedBy(NewController);

    if (WeaponAbilityComponent)
    {
        // Apply the default attribute GE after Super::PossessedBy, which has already run
        // NotifyControllerChanged and finished InitAbilityActorInfo, so the ASC is ready.
        // Re-possessing resets Health to full; this is accepted behavior, not a bug.
        if (DefaultAttributeGE)
        {
            FGameplayEffectContextHandle Context = WeaponAbilityComponent->MakeEffectContext();
            Context.AddSourceObject(this);
            WeaponAbilityComponent->ApplyGameplayEffectToSelf(DefaultAttributeGE.GetDefaultObject(), 1.f, Context);
        }
    }

    SetOwner(NewController);

    // Initial equip gate: the character is now possessed (server-only path). The inventory
    // checks its own gates (initial seed done, possession done) and no-ops if unmet.
    if (InventoryComponent)
    {
        InventoryComponent->TryInitialEquip_ServerOnly();
    }
}

void AEWECharacter::BeginPlay()
{
    // Subscribe before Super::BeginPlay(), which dispatches component BeginPlay.
    // The inventory can broadcast its initial sync from there.
    SetupInventorySubscriptions();

    Super::BeginPlay();

    // Enable ticking for target acquisition
    PrimaryActorTick.bCanEverTick = true;
}

void AEWECharacter::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    ClearInventorySubscriptions();

    SubWeaponMesh->SetSkeletalMesh(nullptr);

    ClearPendingEquipSubscription();
    EquippedWeaponInstance = nullptr;

    Super::EndPlay(EndPlayReason);
}

void AEWECharacter::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

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

    // Convergence point for PossessedBy / UnPossessed / OnRep_Controller. The ASC does
    // not detect controller changes on its own, so refresh its cached PlayerController here.
    // Avatar is always `this` (invariant). Temporary location until the ASC moves to PlayerState.
    if (WeaponAbilityComponent)
    {
        WeaponAbilityComponent->InitAbilityActorInfo(this, this);
    }

    APlayerController *PlayerController = Cast<APlayerController>(Controller);
    if (!PlayerController)
    {
        return;
    }

    UEnhancedInputLocalPlayerSubsystem *Subsystem =
        ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PlayerController->GetLocalPlayer());
    if (!Subsystem)
    {
        return;
    }

    Subsystem->AddMappingContext(DefaultMappingContext, 0);
    Subsystem->AddMappingContext(QuickSlotMappingContext, 1);
}

void AEWECharacter::SetupPlayerInputComponent(UInputComponent *PlayerInputComponent)
{
    UEnhancedInputComponent *EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent);
    if (EnhancedInputComponent)
    {
        EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Started, this, &ACharacter::Jump);
        EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Completed, this, &ACharacter::StopJumping);

        EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &AEWECharacter::Move);

        EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Triggered, this, &AEWECharacter::Look);

        EnhancedInputComponent->BindAction(ChangeCameraAction, ETriggerEvent::Triggered, this,
                                           &AEWECharacter::ChangeCamera);

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

        EnhancedInputComponent->BindAction(ToggleInventoryAction, ETriggerEvent::Triggered, this,
                                           &AEWECharacter::ToggleInventory);
    }
    else
    {
        EWE_LOG(LogEWECharacter, Error,
                TEXT("'%s' Failed to find an Enhanced Input component! This template is built to use the Enhanced Input "
                     "system. If you intend to use the legacy system, then you will need to update this C++ file."),
                *GetNameSafe(this));
    }
}

void AEWECharacter::Move(const FInputActionValue &Value)
{
    FVector2D MovementVector = Value.Get<FVector2D>();

    if (Controller != nullptr)
    {
        const FRotator Rotation = Controller->GetControlRotation();
        const FRotator YawRotation(0, Rotation.Yaw, 0);

        const FVector ForwardDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);

        const FVector RightDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

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

    FVector2D LookAxisVector = Value.Get<FVector2D>();

    if (Controller != nullptr)
    {
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

    // The quick slot view model and the inventory widget observe the inventory Model
    // directly (see UEWELocalUIManageSubsystem). The character only keeps the removal log.
    InventoryComponent->OnWeaponRemoved.AddDynamic(this, &AEWECharacter::HandleWeaponRemoved);

    EWE_LOG(LogEWECharacter, Log, TEXT("Inventory subscriptions setup complete"));
}

void AEWECharacter::ClearInventorySubscriptions()
{
    if (InventoryComponent)
    {
        InventoryComponent->OnWeaponRemoved.RemoveAll(this);

        EWE_LOG(LogEWECharacter, Log, TEXT("Inventory subscriptions cleared"));
    }
}

void AEWECharacter::HandleWeaponRemoved(UEWEWeaponInstance *Weapon, int32 Index)
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

void AEWECharacter::EquipWeapon_ServerOnly(UEWEWeaponInstance *Instance)
{
    if (!HasAuthority())
    {
        EWE_LOG(LogEWECharacter, Warning, TEXT("EquipWeapon: no authority"));
        return;
    }

    if (!WeaponAbilityComponent)
    {
        return;
    }

    // Removes weapon abilities. OnEquip GAs (UEWEGA_DefaultOnEquip) remove their GEs in
    // EndAbility, which the engine triggers via OnRemoveAbility when ClearAllAbilities runs.
    WeaponAbilityComponent->ClearAllAbilities();

    // Drop any pending deferred-grant subscription from a previous equip target.
    ClearPendingEquipSubscription();

    if (!Instance)
    {
        // Unequip: no new grant.
        return;
    }

    if (Instance->IsLoadFinished())
    {
        GrantWeaponAbilities(Instance);
    }
    else
    {
        // The target is still loading. Subscribe to its load-finished notification; the grant
        // happens in OnPendingEquipLoadFinished if it is still the equipped instance then.
        PendingEquipInstance = Instance;
        PendingEquipHandle = Instance->OnLoadFinished.AddUObject(this, &AEWECharacter::OnPendingEquipLoadFinished);
    }
}

void AEWECharacter::SetEquippedWeaponInstance_ServerOnly(UEWEWeaponInstance *Instance)
{
    if (!HasAuthority())
    {
        EWE_LOG(LogEWECharacter, Warning, TEXT("SetEquippedWeaponInstance: no authority"));
        return;
    }

    EquippedWeaponInstance = Instance;

    // Refresh the hand visual on the server / listening host.
    WeaponMesh->SetInstance(Instance);
}

UEWEWeaponInstance *AEWECharacter::GetEquippedWeaponInstance() const
{
    return EquippedWeaponInstance;
}

void AEWECharacter::OnRep_EquippedWeaponInstance()
{
    // The replicated value is already in EquippedWeaponInstance — just apply it to the hand.
    WeaponMesh->SetInstance(EquippedWeaponInstance);
}

void AEWECharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty> &OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(AEWECharacter, EquippedWeaponInstance);
}

void AEWECharacter::GrantWeaponAbilities(UEWEWeaponInstance *Instance)
{
    if (!WeaponAbilityComponent || !Instance)
    {
        return;
    }

    const TArray<TSubclassOf<UGameplayAbility>> &HoldAbilities = Instance->GetHoldAbilities();
    const TArray<TSubclassOf<UGameplayAbility>> &HitAbilities = Instance->GetHitAbilities();
    const TArray<TSubclassOf<UGameplayAbility>> &OnEquipAbilities = Instance->GetOnEquipAbilities();

    if (HoldAbilities.Num() + HitAbilities.Num() + OnEquipAbilities.Num() == 0)
    {
        EWE_LOG(LogEWECharacter, Warning, TEXT("EquipWeapon: instance %s has no loaded abilities"), *Instance->GetName());
        return;
    }

    // Per-category InputID so the activation side (AttackHold / AttackHitCheck /
    // AttackReleaseCheck) presses and releases each GA by its own index. Level 0 matches
    // the original grant; the instance is the source object so the grant tracks it for
    // deferred load finish and removal.
    for (int32 i = 0; i < HoldAbilities.Num(); ++i)
    {
        if (!HoldAbilities[i])
        {
            continue;
        }
        FGameplayAbilitySpec Spec(HoldAbilities[i], 0, CHoldAbilityIndex + i, Instance);
        WeaponAbilityComponent->GiveAbility(Spec);
    }

    for (int32 i = 0; i < HitAbilities.Num(); ++i)
    {
        if (!HitAbilities[i])
        {
            continue;
        }
        FGameplayAbilitySpec Spec(HitAbilities[i], 0, CHitAbilityIndex + i, Instance);
        WeaponAbilityComponent->GiveAbility(Spec);
    }

    for (int32 i = 0; i < OnEquipAbilities.Num(); ++i)
    {
        if (!OnEquipAbilities[i])
        {
            continue;
        }
        FGameplayAbilitySpec Spec(OnEquipAbilities[i], 0, INDEX_NONE, Instance);
        WeaponAbilityComponent->GiveAbility(Spec);
    }
}

void AEWECharacter::ClearPendingEquipSubscription()
{
    UEWEWeaponInstance *Pending = PendingEquipInstance.Get();
    if (Pending)
    {
        Pending->OnLoadFinished.Remove(PendingEquipHandle);
    }
    PendingEquipInstance = nullptr;
    PendingEquipHandle.Reset();
}

void AEWECharacter::OnPendingEquipLoadFinished(UEWEWeaponInstance *FinishedInstance)
{
    // Only grant if this is still the equipped instance. If the selection changed in the
    // meantime, the new equip path already handled the grant (or subscribed itself).
    if (FinishedInstance != EquippedWeaponInstance.Get())
    {
        return;
    }

    ClearPendingEquipSubscription();
    GrantWeaponAbilities(FinishedInstance);
}

void AEWECharacter::SelectWeapon(const uint8 SlotIndex)
{
    // Slot selection is server-authoritative. Local input sends the request to the server
    // through the inventory's RPC; the result comes back via OnRep_SelectedQuickSlotIndex.
    if (InventoryComponent)
    {
        InventoryComponent->Server_SelectQuickSlot(SlotIndex);
    }
}

void AEWECharacter::ScrollWeapon(const FInputActionValue &Value)
{
    const float ScrollDirection = -Value.Get<float>();
    if (InventoryComponent)
    {
        InventoryComponent->Server_ScrollQuickSlot(ScrollDirection);
    }
}

void AEWECharacter::Attack(const FInputActionValue &Value) { K2_Attack(); }

void AEWECharacter::AttackHold()
{
    if (!WeaponAbilityComponent || !EquippedWeaponInstance)
    {
        return;
    }

    const int32 HoldAbilityCount = EquippedWeaponInstance->GetHoldAbilities().Num();
    for (int32 i = 0; i < HoldAbilityCount; ++i)
    {
        WeaponAbilityComponent->AbilityLocalInputPressed(CHoldAbilityIndex + i);
    }
}

void AEWECharacter::AttackHitCheck()
{
    if (!WeaponAbilityComponent || !EquippedWeaponInstance)
    {
        return;
    }

    const int32 HoldAbilityCount = EquippedWeaponInstance->GetHoldAbilities().Num();
    for (int32 i = 0; i < HoldAbilityCount; ++i)
    {
        WeaponAbilityComponent->AbilityLocalInputReleased(CHoldAbilityIndex + i);
    }

    const int32 HitAbilityCount = EquippedWeaponInstance->GetHitAbilities().Num();
    for (int32 i = 0; i < HitAbilityCount; ++i)
    {
        WeaponAbilityComponent->AbilityLocalInputPressed(CHitAbilityIndex + i);
    }
}

void AEWECharacter::AttackReleaseCheck()
{
    if (!WeaponAbilityComponent || !EquippedWeaponInstance)
    {
        return;
    }

    const int32 HitAbilityCount = EquippedWeaponInstance->GetHitAbilities().Num();
    for (int32 i = 0; i < HitAbilityCount; ++i)
    {
        WeaponAbilityComponent->AbilityLocalInputReleased(CHitAbilityIndex + i);
    }
}

#pragma endregion

#pragma region Status

UEWEAttributeBase *AEWECharacter::GetCharacterAttribute() { return AttributeSet; }

#pragma endregion

#pragma region TargetInfo

void AEWECharacter::RefreshTargetInfo()
{
    NotifyTargetChanged(GetFocusedTarget());
}

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

        if (HitActor != this)
        {
            NewTarget = HitActor;
        }
    }

    // A destroyed target reads as null (== "no hit"), so IsStale() is needed to
    // still clear the HUD; it fires once because FocusedTarget is reset or set live after notify.
    if (NewTarget != FocusedTarget.Get() || FocusedTarget.IsStale())
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

    // TODO: Replace GetActorLabel() with a proper display name source.
    // GetActorLabel() is declared inside #if WITH_EDITOR, so this code does not
    // compile for non-editor targets (Shipping / Game). It also returns the level
    // editor label (e.g. "BP_Enemy_2"), which is not player-facing text.
    // Once a character display name exists, read the localizable FText from it and
    // drop the FString -> FText conversion here.
    if (NewTarget)
    {
        AEWECharacter *TargetChar = Cast<AEWECharacter>(NewTarget);
        if (TargetChar)
        {
            FText TargetName = FText::FromString(NewTarget->GetActorLabel());

            PC->UpdateTargetInfo(TargetName, TargetChar->GetAbilitySystemComponent());
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