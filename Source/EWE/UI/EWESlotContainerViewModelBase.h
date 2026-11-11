// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "MVVMViewModelBase.h"
#include "EWESlotContainerViewModelBase.generated.h"

class UEWEInventoryComponent;
class UEWEWeaponInstance;
class UEWESlotItemViewModel;

/**
 * Shared base for the slot container view models (quick slot bar, inventory grid).
 *
 * Owns the slot item view model array and the Model binding lifecycle. BindModel is a
 * non-virtual template method: it unsubscribes the currently bound Model, binds the new
 * one, and refreshes through the child's hooks. Children provide the Model accessors and
 * subscriptions; selection handling is a child concern.
 */
UCLASS(Abstract, BlueprintType)
class EWE_API UEWESlotContainerViewModelBase : public UMVVMViewModelBase
{
    GENERATED_BODY()

public:
    /**
     * Binds to a new inventory Model. Unsubscribes the currently bound Model, binds the
     * new one, and refreshes every slot. Safe to call more than once; a null New simply
     * unbinds (Verbose).
     */
    void BindModel(class UEWEInventoryComponent *Old, class UEWEInventoryComponent *New);

    UFUNCTION(BlueprintPure, FieldNotify) int32 GetSlotCount() const { return SlotItems.Num(); }

    UFUNCTION(BlueprintPure)
    UEWESlotItemViewModel *GetSlotItem(int32 SlotIndex) const;

protected:
    /** Number of slots the Model exposes. */
    virtual int32 GetModelSlotCount(const class UEWEInventoryComponent &Model) const
        PURE_VIRTUAL(UEWESlotContainerViewModelBase::GetModelSlotCount, return 0;);

    /** The Model's slot contents (instance per slot, nullptr = empty). */
    virtual const TArray<TObjectPtr<class UEWEWeaponInstance>> &GetModelSlots(const class UEWEInventoryComponent &Model) const
    {
        static const TArray<TObjectPtr<class UEWEWeaponInstance>> Empty;
        PURE_VIRTUAL(UEWESlotContainerViewModelBase::GetModelSlots, return Empty;);
    }

    /** Subscribes to the Model's broadcasts. */
    virtual void SubscribeModel(class UEWEInventoryComponent &Model)
        PURE_VIRTUAL(UEWESlotContainerViewModelBase::SubscribeModel, );

    /** Unsubscribes from the Model's broadcasts. */
    virtual void UnsubscribeModel(class UEWEInventoryComponent &Model)
        PURE_VIRTUAL(UEWESlotContainerViewModelBase::UnsubscribeModel, );

    /** Runs after the new Model is bound, before the null check. Inventory clears its
     *  selection here; the default is a no-op. */
    virtual void OnModelBound() {}

    /** Sizes the item array to the Model and mirrors its slot contents. Children override
     *  this to reflect their own display state (selection, etc.) after Super::UpdateFromModel(). */
    virtual void UpdateFromModel();

    UPROPERTY(BlueprintReadOnly, FieldNotify)
    TArray<TObjectPtr<UEWESlotItemViewModel>> SlotItems;

    /** The inventory Model being observed. Weak: the pawn may be replaced. */
    TWeakObjectPtr<class UEWEInventoryComponent> BoundModel;
};
