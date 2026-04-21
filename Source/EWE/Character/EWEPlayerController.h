// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "EWEPlayerController.generated.h"

/**
 * Player controller for EWE.
 * Delegates UI management to UEWELocalUIManageSubsystem.
 */
UCLASS()
class EWE_API AEWEPlayerController : public APlayerController
{
    GENERATED_BODY()

public:
    class UEWELocalUIManageSubsystem *GetUIManager() { return CachedUIManager; }

protected:
    virtual void BeginPlay() override;
    virtual void AcknowledgePossession(APawn *P) override;

    /** Cached reference to the UI management subsystem */
    UPROPERTY(Transient)
    TObjectPtr<class UEWELocalUIManageSubsystem> CachedUIManager;

#pragma region Inventory
public:
    /** Delegates to UEWELocalUIManageSubsystem::ToggleInventory */
    UFUNCTION(BlueprintCallable, Category = Inventory)
    void ToggleInventory();

    /** Delegates to UEWELocalUIManageSubsystem::SyncInventoryWeapons */
    void SyncInventoryWeapons(const TArray<class UEWEWeaponData *> &Weapons);

    /** Returns the selected weapon data from inventory widget */
    UFUNCTION(BlueprintCallable, Category = Inventory)
    class UEWEWeaponData *GetSelectedWeaponDataFromInventory();

#pragma endregion

#pragma region QuickSlot
public:
    /** Delegates to AEWEHUD::AddWeapon */
    void AddWeapon(class UEWEWeaponData *Weapon);

    /** Delegates to AEWEHUD::SetSlot */
    void SetSlot(const uint8 SlotIndex, class UEWEWeaponData *Weapon);

    /** Delegates to AEWEHUD::SelectSlot */
    void SelectSlot(const uint8 SlotIndex);

    /** Delegates to AEWEHUD::ScrollSlot */
    void ScrollSlot(const float ScrollDirection);

    /** Assigns the selected inventory item to the specified quick slot */
    UFUNCTION(BlueprintCallable, Category = Inventory)
    void AssignSelectedInventoryItemToSlot(int32 SlotIndex);

#pragma endregion

#pragma region TargetInfo
public:
    /** Updates the target info UI with a new target's data */
    void UpdateTargetName(const FText &TargetName);
    void UpdateTargetInfo(const FText &TargetName, class UEWEAttributeBase *TargetAttributeSet);

    /** Clears the target info UI */
    void ClearTargetInfo();

#pragma endregion
};
