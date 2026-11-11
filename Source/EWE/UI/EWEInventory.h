// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "EWEUserWidgetBase.h"
#include "FieldNotificationId.h"
#include "EWEInventory.generated.h"

class UEWEInventoryViewModel;

UCLASS()
class EWE_API UEWEInventory : public UEWEUserWidgetBase
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintImplementableEvent, Category = Inventory, Meta = (DisplayName = "OnInventoryOpened"))
	void K2_OnInventoryOpened();

	UFUNCTION(BlueprintImplementableEvent, Category = Inventory, Meta = (DisplayName = "OnInventoryClosed"))
	void K2_OnInventoryClosed();

	UFUNCTION(BlueprintCallable, Category = Inventory)
	void OpenInventory();

	UFUNCTION(BlueprintCallable, Category = Inventory)
	void CloseInventory();

	UFUNCTION(BlueprintCallable, Category = Inventory)
	bool IsInventoryOpen() const { return bIsInventoryOpen; }

	/** Injects the view model, binds it to this widget's own view, and subscribes to its
	 *  GetSlotCount. Entry creation happens when the inventory grid is implemented. */
	void SetViewModel(UEWEInventoryViewModel *InViewModel);

protected:
	void SetMouseCursorVisible(bool bShow);

	UEWEInventory(const FObjectInitializer& ObjectInitializer);
	virtual void NativeDestruct() override;

	bool bIsInventoryOpen = false;

	/** The injected view model. Weak: the subsystem owns it and may tear it down. Kept to
	 *  unsubscribe from GetSlotCount, create slot widgets, and clear the selection on close. */
	TWeakObjectPtr<UEWEInventoryViewModel> ViewModel;

	/** Handle for the view model's GetSlotCount field-change subscription. */
	FDelegateHandle SlotCountChangedHandle;

private:
	void OnSlotCountChanged(UObject *InViewModel, UE::FieldNotification::FFieldId InFieldId);
};
