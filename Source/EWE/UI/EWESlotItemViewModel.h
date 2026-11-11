// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "MVVMViewModelBase.h"
#include "EWESlotItemViewModel.generated.h"

class UEWEWeaponInstance;

/** One slot item — shows one weapon (or empty) and selection highlight state. Shared by the quick slot bar and the inventory grid. */
UCLASS(BlueprintType)
class EWE_API UEWESlotItemViewModel : public UMVVMViewModelBase
{
    GENERATED_BODY()

public:
    /** Unsubscribes from the instance's notifications so no stale binding lingers on it. */
    virtual void BeginDestroy() override;

    /** Assigns the instance shown in this slot. Pass nullptr to clear it. */
    void SetInstance(class UEWEWeaponInstance *InInstance);
    void SetSelected(bool bInSelected);

    /** Fixed position of this slot in the quick slot bar or inventory grid. Set once at creation. */
    void SetSlotIndex(int32 InSlotIndex);

    UFUNCTION(BlueprintPure, FieldNotify) bool IsEmpty() const { return WeaponInstance == nullptr; }
    UFUNCTION(BlueprintPure, FieldNotify) bool IsSelected() const { return bSelected; }
    UFUNCTION(BlueprintPure, FieldNotify) FText GetWeaponName() const;

    UFUNCTION(BlueprintPure, FieldNotify)
    class UEWEWeaponInstance *GetInstance() const { return WeaponInstance; }

    UFUNCTION(BlueprintPure, FieldNotify)
    int32 GetSlotIndex() const { return SlotIndex; }

    // TODO: Expose the weapon icon (UEWEWeaponInstance::GetWeaponIcon) as a FieldNotify
    // getter so a slot can display it. Re-broadcast on load-finished and item-state
    // change, like GetWeaponName.

protected:
    UPROPERTY(BlueprintReadOnly, FieldNotify)
    TObjectPtr<class UEWEWeaponInstance> WeaponInstance;

    UPROPERTY(BlueprintReadOnly, FieldNotify)
    bool bSelected = false;

    UPROPERTY(BlueprintReadOnly, FieldNotify)
    int32 SlotIndex = 0;

    /** The instance this slot is subscribed to for load-finished and item-state changes (if any). */
    TWeakObjectPtr<class UEWEWeaponInstance> SubscribedInstance;

    FDelegateHandle LoadFinishedHandle;
    FDelegateHandle ItemChangedHandle;

protected:
    void HandleLoadFinished(class UEWEWeaponInstance *FinishedInstance);
    void HandleItemChanged(class UEWEWeaponInstance *ChangedInstance);
    void UnsubscribeInstance();
};
