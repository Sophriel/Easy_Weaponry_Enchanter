// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "EWEInventory.generated.h"

class UEWEInventoryViewModel;

UCLASS()
class EWE_API UEWEInventory : public UUserWidget
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

	/** Stores the injected view model. Entry creation happens when the widget is implemented. */
	void SetViewModel(class UEWEInventoryViewModel *InViewModel);

protected:
	void SetMouseCursorVisible(bool bShow);

	UEWEInventory(const FObjectInitializer& ObjectInitializer);

	bool bIsInventoryOpen = false;

	/** The injected view model. Weak: the subsystem owns it and may tear it down. */
	TWeakObjectPtr<UEWEInventoryViewModel> ViewModel;
};
