// Fill out your copyright notice in the Description page of Project Settings.

#include "EWELocalUIManageSubsystem.h"
#include "EWEInventory.h"
#include "EWEQuickSlot.h"
#include "EWEStatus.h"
#include "EWEUIAsset.h"
#include "EWE/EWESettings.h"
#include "EWE/EWELog.h"
#include "EWE/Character/EWECharacter.h"
#include "EasyWeaponryEnchanter/Public/EasyWeaponryEnchanter.h"
#include "EasyWeaponryEnchanter/Attribute/EWEAttributeBase.h"

// Initialize subsystem
void UEWELocalUIManageSubsystem::Initialize(FSubsystemCollectionBase &Collection)
{
    Super::Initialize(Collection);

    // Pre-load UIAssets during initialization
    const UEWESettings *Settings = GetDefault<UEWESettings>();
    if (Settings && !Settings->UIConfigAsset.IsNull())
    {
        UIConfigAsset = Settings->UIConfigAsset.LoadSynchronous();
    }
}

// Cleanup on shutdown
void UEWELocalUIManageSubsystem::Deinitialize()
{
    // Destroy widgets
    if (InventoryWidget)
    {
        CloseInventory();
        InventoryWidget->RemoveFromParent();
        InventoryWidget = nullptr;
    }

    if (QuickSlotWidget)
    {
        QuickSlotWidget->RemoveFromParent();
        QuickSlotWidget = nullptr;
    }

    if (StatusWidget)
    {
        StatusWidget->RemoveFromParent();
        StatusWidget = nullptr;
    }

    Super::Deinitialize();
}

void UEWELocalUIManageSubsystem::PlayerControllerChanged(APlayerController *NewPlayerController)
{
    CurrentPlayerController = NewPlayerController;

    UIConfigAsset->AsyncLoadWidgetClass(this);
}

const UEWEUIAsset *UEWELocalUIManageSubsystem::GetUIConfigAsset() const
{
    const UEWEUIAsset *UIConfig = UIConfigAsset.Get();
    if (!UIConfig)
    {
        EWE_LOG(LogEWE, Error, TEXT("UIConfigAsset not set in UEWESettings"));
        return nullptr;
    }

    return UIConfig;
}

void UEWELocalUIManageSubsystem::CreateUIWidgets()
{
    // Create widgets when ASyncLoad is ready (always visible)
    CreateQuickSlotWidget();
    CreateStatusWidget();
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
    if (InventoryWidget)
        return;

    TSoftClassPtr<UUserWidget> WidgetClass = GetUIConfigAsset()->InventoryWidgetClass;
    if (!WidgetClass)
    {
        EWE_LOG(LogEWE, Error, TEXT("InventoryWidgetClass not set in UEWEUIAsset"));
        return;
    }

    InventoryWidget = CreateWidget<UEWEInventory>(GetWorld(), WidgetClass.Get());
    if (!InventoryWidget)
    {
        EWE_LOG(LogEWE, Error, TEXT("Failed to create Inventory widget"));
        return;
    }

    InventoryWidget->AddToViewport();
}

#pragma endregion

#pragma region QuickSlot

void UEWELocalUIManageSubsystem::CreateQuickSlotWidget()
{
    if (QuickSlotWidget)
        return;

    TSoftClassPtr<UUserWidget> WidgetClass = GetUIConfigAsset()->QuickSlotWidgetClass;
    if (!WidgetClass)
    {
        EWE_LOG(LogEWE, Error, TEXT("QuickSlotWidgetClass not set in UEWEUIAsset"));
        return;
    }

    QuickSlotWidget = CreateWidget<UEWEQuickSlot>(GetWorld(), WidgetClass.Get());
    if (!QuickSlotWidget)
    {
        EWE_LOG(LogEWE, Error, TEXT("Failed to create QuickSlot widget"));
        return;
    }

    QuickSlotWidget->AddToViewport();
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

#pragma region Status

void UEWELocalUIManageSubsystem::CreateStatusWidget()
{
    if (StatusWidget)
        return;

    TSoftClassPtr<UUserWidget> WidgetClass = GetUIConfigAsset()->StatusWidgetClass;
    if (!WidgetClass)
    {
        EWE_LOG(LogEWE, Error, TEXT("StatusWidgetClass not set in UEWEUIAsset"));
        return;
    }

    StatusWidget = CreateWidget<UEWEStatus>(GetWorld(), WidgetClass.Get());
    if (!StatusWidget)
    {
        EWE_LOG(LogEWE, Error, TEXT("Failed to create Status widget"));
        return;
    }

    StatusWidget->AddToViewport();

    // Bind AttributeSet to the widget
    if (!CurrentPlayerController)
    {
        return;
    }

    if (APawn *Pawn = CurrentPlayerController->GetPawn<APawn>())
    {
        if (AEWECharacter *EWEChar = Cast<AEWECharacter>(Pawn))
        {
            if (UEWEAttributeBase *AttributeSet = EWEChar->GetCharacterAttribute())
            {
                StatusWidget->BindAttributeSet(AttributeSet);
            }
        }
    }
}

#pragma endregion