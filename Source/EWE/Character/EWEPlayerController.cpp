// Fill out your copyright notice in the Description page of Project Settings.

#include "EWEPlayerController.h"
#include "EWECharacter.h"
#include "EWE/UI/EWEInventory.h"
#include "EWE/UI/EWELocalUIManageSubsystem.h"
#include "EasyWeaponryEnchanter/Public/EasyWeaponryEnchanter.h"
#include "AbilitySystemComponent.h"

void AEWEPlayerController::AcknowledgePossession(APawn *P)
{
    Super::AcknowledgePossession(P);

    ULocalPlayer *LocalPlayer = GetLocalPlayer();
    if (LocalPlayer)
    {
        // Cache UIManager reference for performance
        CachedUIManager = LocalPlayer->GetSubsystem<UEWELocalUIManageSubsystem>();
    }

    if (CachedUIManager)
    {
        CachedUIManager->BindStatus();
        CachedUIManager->BindQuickSlot();
        CachedUIManager->BindTargetInfo();
    }
}

#pragma region Inventory

void AEWEPlayerController::ToggleInventory()
{
    if (CachedUIManager)
    {
        CachedUIManager->ToggleInventory();
    }
}

void AEWEPlayerController::SyncInventoryWeapons(const TArray<class UEWEWeaponData *> &Weapons)
{
    if (CachedUIManager)
    {
        CachedUIManager->SyncInventoryWeapons(Weapons);
    }
}

UEWEWeaponData *AEWEPlayerController::GetSelectedWeaponDataFromInventory()
{
    if (CachedUIManager)
    {
        UEWEInventory *InventoryWidget = CachedUIManager->GetInventoryWidget();
        if (InventoryWidget)
        {
            TArray<UEWEWeaponData *> Weapons = InventoryWidget->GetWeapons();
            int32 SelectedIndex = InventoryWidget->GetSelectedItemIndex();

            if (SelectedIndex >= 0 && SelectedIndex < Weapons.Num())
            {
                return Weapons[SelectedIndex];
            }
        }
    }

    EWE_LOG(LogEWE, Warning, TEXT("No weapon selected in inventory"));
    return nullptr;
}

#pragma endregion

#pragma region QuickSlot

void AEWEPlayerController::AddWeapon(UEWEWeaponData *Weapon)
{
    if (!CachedUIManager)
    {
        EWE_LOG(LogEWE, Warning, TEXT("AddWeapon: UI manager not cached yet"));
        return;
    }

    CachedUIManager->AddWeaponToQuickSlot(Weapon);
}

void AEWEPlayerController::SelectSlot(const uint8 SlotIndex)
{
    if (!CachedUIManager)
    {
        EWE_LOG(LogEWE, Warning, TEXT("SelectSlot: UI manager not cached yet"));
        return;
    }

    CachedUIManager->SelectQuickSlot(SlotIndex);
}

void AEWEPlayerController::ScrollSlot(const float ScrollDirection)
{
    if (!CachedUIManager)
    {
        EWE_LOG(LogEWE, Warning, TEXT("ScrollSlot: UI manager not cached yet"));
        return;
    }

    CachedUIManager->ScrollQuickSlot(ScrollDirection);
}

void AEWEPlayerController::AssignSelectedInventoryItemToSlot(int32 SlotIndex)
{
    UEWEWeaponData *SelectedWeapon = GetSelectedWeaponDataFromInventory();
    if (!SelectedWeapon)
    {
        return;
    }

    if (CachedUIManager)
    {
        CachedUIManager->SetQuickSlot(SlotIndex, SelectedWeapon);
    }
}

#pragma endregion

#pragma region TargetInfo

void AEWEPlayerController::UpdateTargetName(const FText &TargetName)
{
    if (CachedUIManager)
    {
        CachedUIManager->UpdateTargetName(TargetName);
    }
}

void AEWEPlayerController::UpdateTargetInfo(const FText &TargetName, UAbilitySystemComponent *TargetASC)
{
    if (CachedUIManager)
    {
        CachedUIManager->UpdateTargetInfo(TargetName, TargetASC);
    }
}

void AEWEPlayerController::ClearTargetInfo()
{
    if (CachedUIManager)
    {
        CachedUIManager->ClearTargetInfo();
    }
}

#pragma endregion
