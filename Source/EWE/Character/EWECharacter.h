// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Logging/LogMacros.h"
#include "EasyWeaponryEnchanter/Interface/EWECharacterInterface.h"
#include "EWEAttackAnimationInterface.h"
#include "AbilitySystemInterface.h"
#include "EWECharacter.generated.h"

struct FInputActionValue;
class UEWEWeaponMeshComponent;

UENUM(BlueprintType)
enum class ECameraType : uint8
{
    Shoulder UMETA(DisplayName = "Shoulder"),
    Quarter UMETA(DisplayName = "Quarter"),
    FirstPerson UMETA(DisplayName = "FirstPerson"),
};

UCLASS(config = Game)
class AEWECharacter : public ACharacter,
                      public IEWECharacterInterface,
                      public IEWEAttackAnimationInterface,
                      public IAbilitySystemInterface
{
    GENERATED_BODY()

public:
    AEWECharacter();

protected:
    virtual void PossessedBy(AController *NewController) override;
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
    virtual void Tick(float DeltaTime) override;

#pragma region Input

protected:
    virtual void NotifyControllerChanged() override;
    virtual void SetupPlayerInputComponent(class UInputComponent *PlayerInputComponent) override;

    /** Default input mapping context. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
    TObjectPtr<class UInputMappingContext> DefaultMappingContext;

    /** Quick slot input mapping context. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
    TObjectPtr<class UInputMappingContext> QuickSlotMappingContext;

    void Move(const FInputActionValue &Value);

    void Look(const FInputActionValue &Value);

    /** Jump Input Action */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
    TObjectPtr<class UInputAction> JumpAction;

    /** Move Input Action */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
    TObjectPtr<class UInputAction> MoveAction;

    /** Look Input Action */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
    TObjectPtr<class UInputAction> LookAction;

    /** Change Camera Input Action */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
    TObjectPtr<class UInputAction> ChangeCameraAction;

    /** Select Item Input Action */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
    TArray<TObjectPtr<class UInputAction>> SelectItemActions;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
    TObjectPtr<class UInputAction> ScrollItemAction;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
    TObjectPtr<class UInputAction> AttackAction;

    /** Toggle Inventory Input Action */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
    TObjectPtr<class UInputAction> ToggleInventoryAction;

#pragma endregion

#pragma region Camera

public:
    FORCEINLINE class USpringArmComponent *GetCameraBoom() const { return CameraBoom; }
    FORCEINLINE class UCameraComponent *GetFollowCamera() const { return FollowCamera; }

protected:
    void ChangeCamera(const FInputActionValue &Value);
    void SetCharacterControl(ECameraType CameraType);
    void SetCharacterControlData(const class UEWEControlData *ControlData);

    /** Camera boom positioning the camera behind the character */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
    TObjectPtr<class USpringArmComponent> CameraBoom;

    /** Follow camera */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
    TObjectPtr<class UCameraComponent> FollowCamera;

    ECameraType CurrentCameraType;

    UPROPERTY(EditAnywhere, Category = Camera, Meta = (AllowPrivateAccess = "true"))
    TMap<ECameraType, TObjectPtr<class UEWEControlData>> CameraControl;

#pragma endregion

#pragma region Weapon

public:
    virtual class UAbilitySystemComponent *GetAbilitySystemComponent() const override;

    /** Server-only equip gate. See IEWECharacterInterface. */
    virtual void EquipWeapon_ServerOnly(class UEWEWeaponInstance *Instance) override;

    virtual void SetEquippedWeaponInstance_ServerOnly(class UEWEWeaponInstance *Instance) override;

    virtual class UEWEWeaponInstance *GetEquippedWeaponInstance() const override;

    /** The inventory component (the quick slot Model). Set in the constructor. */
    class UEWEInventoryComponent *GetInventoryComponent() const { return InventoryComponent; }

protected:
    void SelectWeapon(const uint8 SlotIndex);
    void ScrollWeapon(const FInputActionValue &Value);

    UFUNCTION(BlueprintImplementableEvent, Category = Weapon, Meta = (DisplayName = "OnAttack"))
    void K2_Attack();

    void Attack(const FInputActionValue &Value);
    virtual void AttackHold() override;
    virtual void AttackHitCheck() override;
    virtual void AttackReleaseCheck() override;

    /** Grants the instance's loaded ability classes (SourceObject = instance). */
    void GrantWeaponAbilities(class UEWEWeaponInstance *Instance);

    /** Drops the pending deferred-grant subscription, if any. */
    void ClearPendingEquipSubscription();

    /** Reacts to a pending equip target finishing loading: grants GAs only if it is still equipped. */
    void OnPendingEquipLoadFinished(class UEWEWeaponInstance *FinishedInstance);

    /**
     * RepNotify: the equipped instance changed. Refreshes the hand visual on this machine.
     */
    UFUNCTION()
    void OnRep_EquippedWeaponInstance();

    /** Toggle Inventory UI */
    UFUNCTION(BlueprintCallable, Category = Inventory)
    void ToggleInventory();

    // Inventory Event Handlers
    UFUNCTION()
    void HandleWeaponRemoved(class UEWEWeaponInstance *Weapon, int32 Index);

    void SetupInventorySubscriptions();
    void ClearInventorySubscriptions();

protected:
    UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = Weapon, meta = (AllowPrivateAccess = "true"))
    bool IsAttack;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Weapon, meta = (AllowPrivateAccess = "true"))
    TObjectPtr<USkeletalMeshComponent> SubWeaponMesh;

    /** Displays the equipped weapon's mesh on the hand. Display-only (no collision). */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Weapon, meta = (AllowPrivateAccess = "true"))
    TObjectPtr<UEWEWeaponMeshComponent> WeaponMesh;

    /**
     * The equipped weapon instance. Replicated to all clients: the server equip gate is the
     * single writer (via SetEquippedWeaponInstance_ServerOnly), clients only receive it. nullptr = unarmed.
     */
    UPROPERTY(ReplicatedUsing = OnRep_EquippedWeaponInstance, VisibleAnywhere, BlueprintReadOnly, Category = Weapon,
              meta = (AllowPrivateAccess = "true"))
    TObjectPtr<class UEWEWeaponInstance> EquippedWeaponInstance;

    /**
     * The equip target whose load is still pending (not yet Ready/Failed). Weak: the instance
     * is owned by the inventory and outlives this reference. Cleared on re-equip or load finish.
     */
    TWeakObjectPtr<class UEWEWeaponInstance> PendingEquipInstance;

    FDelegateHandle PendingEquipHandle;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Weapon, meta = (AllowPrivateAccess = "true"))
    TObjectPtr<class UEWEInventoryComponent> InventoryComponent;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Weapon, meta = (AllowPrivateAccess = "true"))
    TObjectPtr<class UAbilitySystemComponent> WeaponAbilityComponent;

#pragma endregion

#pragma region Attribute

public:
    class UEWEAttributeBase *GetCharacterAttribute();

protected:
    UPROPERTY(EditDefaultsOnly, Category = Attribute, meta = (AllowPrivateAccess = "true"))
    TSubclassOf<class UGameplayEffect> DefaultAttributeGE;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Attribute, meta = (AllowPrivateAccess = "true"))
    TObjectPtr<class UEWEAttributeBase> AttributeSet;

#pragma endregion

#pragma region TargetInfo

public:
    AActor *GetFocusedTarget() const { return FocusedTarget.Get(); }

    void RefreshTargetInfo();

    /** Trace range for target acquisition */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Target Info")
    float TargetTraceRange = 5000.f;

    /** Trace frequency in seconds (how often to perform target trace) */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Target Info")
    float TargetTraceInterval = 0.1f;

protected:
    void UpdateTargetAcquisition(float DeltaTime);

    void NotifyTargetChanged(AActor *NewTarget);

    /** Weak: the character only observes it. */
    UPROPERTY(Transient)
    TWeakObjectPtr<AActor> FocusedTarget;

    float TargetTraceAccumulator = 0.f;

#pragma endregion
};
