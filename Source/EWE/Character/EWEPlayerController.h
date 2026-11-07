// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "EWEPlayerController.generated.h"

class UEWEInventoryComponent;

// The inventory Model changed on this controller. Fired on the local PC (owning client /
// listen host) when possession changes the possessed pawn's inventory.
DECLARE_MULTICAST_DELEGATE_TwoParams(FOnInventoryModelChangedEvent, class UEWEInventoryComponent * /*Old*/,
                                     class UEWEInventoryComponent * /*New*/);

/**
 * Player controller for EWE.
 * Delegates UI management to UEWELocalUIManageSubsystem.
 *
 * Relays "which inventory Model is current" (D12): it does not forward individual
 * inventory events. Subscribers (the local UI) observe the Model directly and are told
 * only when the Model itself changes.
 */
UCLASS()
class EWE_API AEWEPlayerController : public APlayerController
{
    GENERATED_BODY()

public:
    class UEWELocalUIManageSubsystem *GetUIManager() { return CachedUIManager; }

    /** The inventory Model of the currently possessed pawn (nullptr if none). */
    class UEWEInventoryComponent *GetInventoryModel() const { return InventoryModel.Get(); }

    /** Fired on the local PC when the possessed pawn's inventory Model changes. */
    FOnInventoryModelChangedEvent OnInventoryModelChanged;

protected:
    virtual void AcknowledgePossession(APawn *P) override;

    UPROPERTY(Transient)
    TObjectPtr<class UEWELocalUIManageSubsystem> CachedUIManager;

    /** The inventory Model of the currently possessed pawn. Weak: the pawn may be replaced. */
    TWeakObjectPtr<class UEWEInventoryComponent> InventoryModel;

#pragma region Inventory
public:
    UFUNCTION(BlueprintCallable, Category = Inventory)
    void ToggleInventory();

#pragma endregion

#pragma region TargetInfo
public:
    void UpdateTargetName(const FText &TargetName);
    void UpdateTargetInfo(const FText &TargetName, class UAbilitySystemComponent *TargetASC);

    void ClearTargetInfo();

#pragma endregion
};
