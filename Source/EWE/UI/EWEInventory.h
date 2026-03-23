// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "EasyWeaponryEnchanter/Weapon/EWEWeaponData.h"
#include "EWEInventory.generated.h"

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

	UFUNCTION(BlueprintCallable, Category = Inventory)
	void SetWeapons(const TArray<class UEWEWeaponData*>& Weapons);

	UFUNCTION(BlueprintCallable, Category = Inventory)
	void SelectItem(int32 Index);

	UFUNCTION(BlueprintCallable, Category = Inventory)
	void AssignToQuickSlot(int32 SlotIndex);

	UFUNCTION(BlueprintCallable, Category = Inventory)
	TArray<class UEWEWeaponData*> GetWeapons() const { return Weapons; }

	UFUNCTION(BlueprintCallable, Category = Inventory)
	int32 GetSelectedItemIndex() const { return SelectedItemIndex; }

protected:
	/** Shows or hides the mouse cursor */
	void SetMouseCursorVisible(bool bShow);

	UEWEInventory(const FObjectInitializer& ObjectInitializer);
	virtual void NativeConstruct() override;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = Inventory)
	TArray<TSubclassOf<UUserWidget>> ItemSlotClasses;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Inventory)
	TArray<TObjectPtr<UUserWidget>> ItemSlots;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Inventory)
	TArray<TObjectPtr<class UEWEWeaponData>> Weapons;

	bool bIsInventoryOpen = false;
	int32 SelectedItemIndex = -1;
};
