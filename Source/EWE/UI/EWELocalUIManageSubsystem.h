// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/LocalPlayerSubsystem.h"
#include "EWELocalUIManageSubsystem.generated.h"

// Forward declarations
class UEWEQuickSlotViewModel;
class UEWEInventoryViewModel;
class UEWEStatusViewModel;
class UEWETargetInfoViewModel;
class UEWEInventoryComponent;
class UEWEWeaponInstance;

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
    virtual void Initialize(FSubsystemCollectionBase &Collection) override;
    virtual void Deinitialize() override;
    virtual void PlayerControllerChanged(APlayerController *NewPlayerController) override;

    void AddUIWidgets();

protected:
    /** Loads and returns the UIConfigAsset if available */
    const class UEWEUIAsset *GetUIConfigAsset() const;

    /**
     * True when every always-on HUD widget already exists and sits in the viewport.
     * The inventory widget is created on demand and is deliberately excluded.
     */
    bool AreEssentialWidgetsInViewport() const;

    /** Drops the pending OnUILoaded subscription, if any. */
    void ClearUILoadedSubscription();

    /** Returns the current inventory Model, or nullptr when the controller is not an EWE controller. */
    class UEWEInventoryComponent *GetCurrentInventoryModel() const;

protected:
    TSoftObjectPtr<class UEWEUIAsset> UIConfigAsset;

    /** Handle for the pending UEWEUIAsset::OnUILoaded subscription. */
    FDelegateHandle UILoadedHandle;

    UPROPERTY(Transient)
    TObjectPtr<APlayerController> CurrentPlayerController;

#pragma region Inventory
public:
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

    /** Gets the inventory widget instance (may be nullptr) */
    UFUNCTION(BlueprintCallable, Category = "UI|Inventory")
    UEWEInventory *GetInventoryWidget() const { return InventoryWidget; }

protected:
    /**
     * Subscribes to the new player controller's Model-change relay and pulls the current
     * Model. Drops the previous controller's subscription first.
     */
    void BindInventoryModel(APlayerController *NewPlayerController);

    /** Forwards the Model change to the quick slot view model and re-drives the widget. */
    void HandleInventoryModelChanged(class UEWEInventoryComponent *Old, class UEWEInventoryComponent *New);

    /** Creates the inventory widget if it doesn't exist */
    void CreateInventoryWidget();

    /** Creates the inventory view model. Called from CreateInventoryWidget(). */
    void CreateInventoryViewModel();

protected:
    UPROPERTY(Transient)
    TObjectPtr<class UEWEInventory> InventoryWidget;

    UPROPERTY(Transient)
    TObjectPtr<class UEWEInventoryViewModel> InventoryViewModel;

#pragma endregion

#pragma region QuickSlot
public:
    /** Gets the quick slot widget instance (may be nullptr) */
    UFUNCTION(BlueprintCallable, Category = "UI|QuickSlot")
    UEWEQuickSlot *GetQuickSlotWidget() const { return QuickSlotWidget; }

protected:
    /** Creates the view model. Called from CreateQuickSlotWidget(). */
    void CreateQuickSlotViewModel();

    void CreateQuickSlotWidget();

    UPROPERTY(Transient)
    TObjectPtr<class UEWEQuickSlotViewModel> QuickSlotViewModel;

    /** Handle for the player controller's OnInventoryModelChanged subscription. */
    FDelegateHandle InventoryModelChangedHandle;

    UPROPERTY(Transient)
    TObjectPtr<class UEWEQuickSlot> QuickSlotWidget;

#pragma endregion

#pragma region Status

public:
    void BindStatus();

protected:
    void CreateStatusWidget();

    /** Creates the status view model. Called from CreateStatusWidget(). */
    void CreateStatusViewModel();

    UPROPERTY(Transient)
    TObjectPtr<class UEWEStatus> StatusWidget;

    UPROPERTY(Transient)
    TObjectPtr<class UEWEStatusViewModel> StatusViewModel;

#pragma endregion

#pragma region TargetInfo
public:
    void BindTargetInfo();

    /** Updates target info with a new target's AttributeSet */
    UFUNCTION(BlueprintCallable, Category = "UI|TargetInfo")
    void UpdateTargetName(const FText &TargetName);

    UFUNCTION(BlueprintCallable, Category = "UI|TargetInfo")
    void UpdateTargetInfo(const FText &TargetName, class UAbilitySystemComponent *TargetASC);

    /** Clears the target info view model */
    UFUNCTION(BlueprintCallable, Category = "UI|TargetInfo")
    void ClearTargetInfo();

    /** Gets the target info widget instance (may be nullptr) */
    UFUNCTION(BlueprintCallable, Category = "UI|TargetInfo")
    class UEWETargetInfo *GetTargetInfoWidget() const { return TargetInfoWidget; }

protected:
    /** Creates the target info widget */
    void CreateTargetInfoWidget();

    /** Creates the target info view model. Called from CreateTargetInfoWidget(). */
    void CreateTargetInfoViewModel();

    UPROPERTY(Transient)
    TObjectPtr<class UEWETargetInfo> TargetInfoWidget;

    UPROPERTY(Transient)
    TObjectPtr<class UEWETargetInfoViewModel> TargetInfoViewModel;

#pragma endregion
};
