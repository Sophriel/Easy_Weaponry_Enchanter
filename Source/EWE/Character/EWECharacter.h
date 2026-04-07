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
    virtual void OnRep_PlayerState() override;
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
    virtual void Tick(float DeltaTime) override;

#pragma region Input

protected:
    virtual void NotifyControllerChanged() override;
    virtual void SetupPlayerInputComponent(class UInputComponent *PlayerInputComponent) override;

    /** MappingContext */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
    TObjectPtr<class UInputMappingContext> DefaultMappingContext;

    /** MappingContext */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
    TObjectPtr<class UInputMappingContext> QuickSlotMappingContext;

    /** Called for movement input */
    void Move(const FInputActionValue &Value);

    /** Called for looking input */
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
    /** Returns CameraBoom subobject **/
    FORCEINLINE class USpringArmComponent *GetCameraBoom() const { return CameraBoom; }
    /** Returns FollowCamera subobject **/
    FORCEINLINE class UCameraComponent *GetFollowCamera() const { return FollowCamera; }

protected:
    /** Called for Change Camera input */
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

    virtual void AcquireWeapon(class UEWEWeaponData *NewWeapon) override;

    UFUNCTION(BlueprintCallable, Category = QuickSlot, Meta = (DisplayName = "EquipWeapon"))
    virtual void EquipWeapon(class UEWEWeaponData *Weapon) override;

protected:
    void SelectWeapon(const uint8 SlotIndex);
    void ScrollWeapon(const FInputActionValue &Value);

    UFUNCTION(BlueprintImplementableEvent, Category = Weapon, Meta = (DisplayName = "OnAttack"))
    void K2_Attack();

    void Attack(const FInputActionValue &Value);
    virtual void AttackHold() override;
    virtual void AttackHitCheck() override;
    virtual void AttackReleaseCheck() override;

    /** Toggle Inventory UI */
    UFUNCTION(BlueprintCallable, Category = Inventory)
    void ToggleInventory();

    // Inventory Event Handlers
    UFUNCTION()
    void HandleWeaponAcquired(class UEWEWeaponData *Weapon);
    UFUNCTION()
    void HandleWeaponsSynced(const TArray<class UEWEWeaponData *> &Weapons, int32 Count);
    UFUNCTION()
    void HandleWeaponRemoved(class UEWEWeaponData *Weapon, int32 Index);

    void SetupInventorySubscriptions();
    void ClearInventorySubscriptions();

protected:
    UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = Weapon, meta = (AllowPrivateAccess = "true"))
    bool IsAttack;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Weapon, meta = (AllowPrivateAccess = "true"))
    TObjectPtr<USkeletalMeshComponent> WeaponMesh;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Weapon, meta = (AllowPrivateAccess = "true"))
    TObjectPtr<USkeletalMeshComponent> SubWeaponMesh;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Weapon, meta = (AllowPrivateAccess = "true"))
    TObjectPtr<class UEWEWeaponData> CurrentWeapon;

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

#pragma region TargetAcquisition

public:
    /** Returns the currently focused target actor, if any */
    AActor *GetFocusedTarget() const { return FocusedTarget; }

    /** Trace range for target acquisition */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Target Acquisition")
    float TargetTraceRange = 5000.f;

    /** Trace frequency in seconds (how often to perform target trace) */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Target Acquisition")
    float TargetTraceInterval = 0.1f;

protected:
    /** Performs a camera trace and updates focused target */
    void UpdateTargetAcquisition(float DeltaTime);

    /** Notifies the player controller about target change */
    void NotifyTargetChanged(AActor *NewTarget);

    /** Currently focused target actor */
    UPROPERTY()
    TObjectPtr<AActor> FocusedTarget;

    /** Accumulator for trace interval */
    float TargetTraceAccumulator = 0.f;

#pragma endregion
};
