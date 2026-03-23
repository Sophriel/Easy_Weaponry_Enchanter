// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/LocalPlayerSubsystem.h"
#include "EWELocalUIManageSubsystem.generated.h"

class UEWEInventory;
class UEWEQuickSlot;

/**
 * Manages all UI widgets for local player.
 * - Inventory widget creation/open/close
 * - QuickSlot widget creation/management
 * - Future: HUD, enchant editor, etc.
 */
UCLASS()
class EWE_API UEWELocalUIManageSubsystem : public ULocalPlayerSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	void OnPlayerControllerReady();

	// ────────────────────────────────────────────────────────────────────────
	// Inventory
	// ────────────────────────────────────────────────────────────────────────

	/** Toggles inventory open/close state */
	UFUNCTION(BlueprintCallable, Category = "UI|Inventory")
	void ToggleInventory();

	/** Opens inventory UI */
	UFUNCTION(BlueprintCallable, Category = "UI|Inventory")
	void OpenInventory();

	/** Closes inventory UI */
	UFUNCTION(BlueprintCallable, Category = "UI|Inventory")
	void CloseInventory();

	/** Returns whether inventory is currently open */
	UFUNCTION(BlueprintCallable, Category = "UI|Inventory")
	bool IsInventoryOpen() const;

	/** Syncs weapons list to inventory UI */
	UFUNCTION(BlueprintCallable, Category = "UI|Inventory")
	void SyncInventoryWeapons(const TArray<class UEWEWeaponData*>& Weapons);

	/** Gets the inventory widget instance (may be nullptr) */
	UFUNCTION(BlueprintCallable, Category = "UI|Inventory")
	UEWEInventory* GetInventoryWidget() const { return InventoryWidget; }

	// ────────────────────────────────────────────────────────────────────────
	// QuickSlot
	// ────────────────────────────────────────────────────────────────────────

	/** Adds a weapon to the first available quick slot */
	UFUNCTION(BlueprintCallable, Category = "UI|QuickSlot")
	void AddWeaponToQuickSlot(class UEWEWeaponData* Weapon);

	/** Sets a weapon at the specified quick slot index */
	UFUNCTION(BlueprintCallable, Category = "UI|QuickSlot")
	void SetQuickSlot(int32 SlotIndex, class UEWEWeaponData* Weapon);

	/** Selects the specified quick slot */
	UFUNCTION(BlueprintCallable, Category = "UI|QuickSlot")
	void SelectQuickSlot(int32 SlotIndex);

	/** Scrolls to the next/previous quick slot */
	UFUNCTION(BlueprintCallable, Category = "UI|QuickSlot")
	void ScrollQuickSlot(float ScrollDirection);

	/** Gets the quick slot widget instance (may be nullptr) */
	UFUNCTION(BlueprintCallable, Category = "UI|QuickSlot")
	UEWEQuickSlot* GetQuickSlotWidget() const { return QuickSlotWidget; }

protected:
	TSoftObjectPtr<class UEWEUIAsset> UIConfigAsset;

	/** Loads and returns the UIConfigAsset if available */
	const class UEWEUIAsset* GetUIConfigAsset() const;

	/** Creates the inventory widget if it doesn't exist */
	void CreateInventoryWidget();

	/** Creates the quick slot widget */
	void CreateQuickSlotWidget();

	/** Currently active inventory widget instance */
	TObjectPtr<UEWEInventory> InventoryWidget;

	/** Currently active quick slot widget instance */
	TObjectPtr<UEWEQuickSlot> QuickSlotWidget;
};
