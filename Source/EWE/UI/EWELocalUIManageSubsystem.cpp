// Fill out your copyright notice in the Description page of Project Settings.

#include "EWELocalUIManageSubsystem.h"
#include "EWEInventory.h"
#include "EWEQuickSlot.h"
#include "EWEUIAsset.h"
#include "EWE/EWESettings.h"
#include "EWE/EWELog.h"
#include "EasyWeaponryEnchanter/Public/EasyWeaponryEnchanter.h"

// Initialize subsystem
void UEWELocalUIManageSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);

    // Pre-load UIConfigAsset during initialization
    const UEWESettings* Settings = GetDefault<UEWESettings>();
    if (Settings && !Settings->UIConfigAsset.IsNull())
    {
        UIConfigAsset = Settings->UIConfigAsset.LoadSynchronous();
    }
}

// Cleanup on shutdown
void UEWELocalUIManageSubsystem::Deinitialize()
{
    CloseInventory();
    InventoryWidget = nullptr;

    // Destroy QuickSlot widget
    if (QuickSlotWidget)
    {
        QuickSlotWidget->RemoveFromParent();
        QuickSlotWidget = nullptr;
    }

    Super::Deinitialize();
}

void UEWELocalUIManageSubsystem::OnPlayerControllerReady()
{
    // Create QuickSlot widget when PlayerController is ready (always visible)
    CreateQuickSlotWidget();
}

const UEWEUIAsset* UEWELocalUIManageSubsystem::GetUIConfigAsset() const
{
    return UIConfigAsset.Get();
}

#pragma region Inventory

void UEWELocalUIManageSubsystem::ToggleInventory()
{
    if (!InventoryWidget)
    {
        CreateInventoryWidget();
    }

    if (!InventoryWidget)
    {
        EWE_LOG(LogEWE, Warning, TEXT("Failed to create inventory widget"));
        return;
    }

    if (InventoryWidget->IsInventoryOpen())
    {
        CloseInventory();
    }
    else
    {
        OpenInventory();
    }
}

void UEWELocalUIManageSubsystem::OpenInventory()
{
    if (!InventoryWidget)
    {
        EWE_LOG(LogEWE, Warning, TEXT("InventoryWidget not created"));
        return;
    }

    InventoryWidget->OpenInventory();
}

void UEWELocalUIManageSubsystem::CloseInventory()
{
    if (!InventoryWidget)
    {
        return;
    }

    InventoryWidget->CloseInventory();
}

bool UEWELocalUIManageSubsystem::IsInventoryOpen() const
{
    return InventoryWidget && InventoryWidget->IsInventoryOpen();
}

void UEWELocalUIManageSubsystem::SyncInventoryWeapons(const TArray<class UEWEWeaponData *> &Weapons)
{
    if (!InventoryWidget)
    {
        EWE_LOG(LogEWE, Warning, TEXT("InventoryWidget not created, cannot sync weapons"));
        return;
    }

    InventoryWidget->SetWeapons(Weapons);
}

void UEWELocalUIManageSubsystem::CreateInventoryWidget()
{
    const UEWEUIAsset* UIConfig = GetUIConfigAsset();
    if (!UIConfig)
    {
        EWE_LOG(LogEWE, Error, TEXT("UIConfigAsset not set in UEWESettings"));
        return;
    }

    TSubclassOf<UEWEInventory> WidgetClass = UIConfig->InventoryWidgetClass;
    if (!WidgetClass)
    {
        EWE_LOG(LogEWE, Error, TEXT("InventoryWidgetClass not set in UEWEUIAsset"));
        return;
    }

    InventoryWidget = CreateWidget<UEWEInventory>(GetWorld(), WidgetClass);
    if (InventoryWidget)
    {
        InventoryWidget->AddToViewport();
    }
}

#pragma endregion

#pragma region QuickSlot
void UEWELocalUIManageSubsystem::CreateQuickSlotWidget()
{
    const UEWEUIAsset* UIConfig = GetUIConfigAsset();
    if (!UIConfig)
    {
        EWE_LOG(LogEWE, Error, TEXT("UIConfigAsset not set in UEWESettings"));
        return;
    }

    TSubclassOf<UEWEQuickSlot> WidgetClass = UIConfig->QuickSlotWidgetClass;
    if (!WidgetClass)
    {
        EWE_LOG(LogEWE, Error, TEXT("QuickSlotWidgetClass not set in UEWEUIAsset"));
        return;
    }

    QuickSlotWidget = CreateWidget<UEWEQuickSlot>(GetWorld(), WidgetClass);
    if (QuickSlotWidget)
    {
        QuickSlotWidget->AddToViewport();
        EWE_LOG(LogEWE, Log, TEXT("QuickSlot widget created and added to viewport"));
    }
}

void UEWELocalUIManageSubsystem::AddWeaponToQuickSlot(UEWEWeaponData *Weapon)
{
    if (!Weapon)
    {
        EWE_LOG(LogEWE, Warning, TEXT("AddWeaponToQuickSlot: Weapon is null"));
        return;
    }

    if (!QuickSlotWidget)
    {
        EWE_LOG(LogEWE, Warning, TEXT("AddWeaponToQuickSlot: QuickSlotWidget not created"));
        return;
    }

    QuickSlotWidget->AddWeapon(Weapon);
}

void UEWELocalUIManageSubsystem::SetQuickSlot(int32 SlotIndex, UEWEWeaponData *Weapon)
{
    if (!QuickSlotWidget)
    {
        EWE_LOG(LogEWE, Warning, TEXT("SetQuickSlot: QuickSlotWidget not created"));
        return;
    }

    QuickSlotWidget->SetWeaponSlot(SlotIndex, Weapon);
}

void UEWELocalUIManageSubsystem::SelectQuickSlot(int32 SlotIndex)
{
    if (!QuickSlotWidget)
    {
        EWE_LOG(LogEWE, Warning, TEXT("SelectQuickSlot: QuickSlotWidget not created"));
        return;
    }

    QuickSlotWidget->K2_SelectSlot(SlotIndex);
}

void UEWELocalUIManageSubsystem::ScrollQuickSlot(float ScrollDirection)
{
    if (!QuickSlotWidget)
    {
        EWE_LOG(LogEWE, Warning, TEXT("ScrollQuickSlot: QuickSlotWidget not created"));
        return;
    }

    QuickSlotWidget->K2_ScrollSlot(ScrollDirection);
}

#pragma endregion
