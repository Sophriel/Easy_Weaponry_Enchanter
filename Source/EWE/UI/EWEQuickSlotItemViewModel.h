// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "MVVMViewModelBase.h"
#include "EWEQuickSlotItemViewModel.generated.h"

class UEWEWeaponInstance;

/** One quick slot item — shows one weapon (or empty) and selection highlight state. */
UCLASS(BlueprintType)
class EWE_API UEWEQuickSlotItemViewModel : public UMVVMViewModelBase
{
    GENERATED_BODY()

public:
    /** Unsubscribes from the instance's load-finished notification so no stale binding lingers on it. */
    virtual void BeginDestroy() override;

    /** Assigns the instance shown in this slot. Pass nullptr to clear it. */
    void SetInstance(class UEWEWeaponInstance *InInstance);
    void SetSelected(bool bInSelected);

    /** Fixed position of this slot in the quick slot bar. Set once at creation. */
    void SetSlotIndex(int32 InSlotIndex);

    UFUNCTION(BlueprintPure, FieldNotify) bool IsEmpty() const { return WeaponInstance == nullptr; }
    UFUNCTION(BlueprintPure, FieldNotify) bool IsSelected() const { return bSelected; }
    UFUNCTION(BlueprintPure, FieldNotify) FText GetWeaponName() const;

    UFUNCTION(BlueprintPure, FieldNotify)
    class UEWEWeaponInstance *GetInstance() const { return WeaponInstance; }

    UFUNCTION(BlueprintPure, FieldNotify)
    int32 GetSlotIndex() const { return SlotIndex; }

protected:
    UPROPERTY(BlueprintReadOnly, FieldNotify)
    TObjectPtr<class UEWEWeaponInstance> WeaponInstance;

    UPROPERTY(BlueprintReadOnly, FieldNotify)
    bool bSelected = false;

    UPROPERTY(BlueprintReadOnly, FieldNotify)
    int32 SlotIndex = 0;

    /** The instance whose load-finished notification this slot is subscribed to (if any). */
    TWeakObjectPtr<class UEWEWeaponInstance> LoadSubscriptionInstance;

    FDelegateHandle LoadFinishedHandle;

    void HandleLoadFinished(class UEWEWeaponInstance *FinishedInstance);
    void UnsubscribeLoadFinished();
};
