// Fill out your copyright notice in the Description page of Project Settings.

#include "EWEPlayerController.h"
#include "EWECharacter.h"
#include "EWE/UI/EWEInventory.h"
#include "EWE/UI/EWELocalUIManageSubsystem.h"
#include "EasyWeaponryEnchanter/Public/EasyWeaponryEnchanter.h"
#include "AbilitySystemComponent.h"

void AEWEPlayerController::BeginPlay()
{
    Super::BeginPlay();

    if (ULocalPlayer* LocalPlayer = GetLocalPlayer())
    {
        // Cache UIManager reference for performance
        CachedUIManager = LocalPlayer->GetSubsystem<UEWELocalUIManageSubsystem>();

        if (CachedUIManager)
        {
            CachedUIManager->OnPlayerControllerReady();
        }
    }
}

void AEWEPlayerController::AddWeapon(UEWEWeaponData *Weapon)
{
    if (CachedUIManager)
    {
        CachedUIManager->AddWeaponToQuickSlot(Weapon);
    }
}

void AEWEPlayerController::SyncInventoryWeapons(const TArray<class UEWEWeaponData *> &Weapons)
{
    if (CachedUIManager)
    {
        CachedUIManager->SyncInventoryWeapons(Weapons);
    }
}

void AEWEPlayerController::SetSlot(const uint8 SlotIndex, UEWEWeaponData *Weapon)
{
    if (CachedUIManager)
    {
        CachedUIManager->SetQuickSlot(SlotIndex, Weapon);
    }
}

void AEWEPlayerController::SelectSlot(const uint8 SlotIndex)
{
    if (CachedUIManager)
    {
        CachedUIManager->SelectQuickSlot(SlotIndex);
    }
}

void AEWEPlayerController::ScrollSlot(const float ScrollDirection)
{
    if (CachedUIManager)
    {
        CachedUIManager->ScrollQuickSlot(ScrollDirection);
    }
}

void AEWEPlayerController::ToggleInventory()
{
    if (CachedUIManager)
    {
        CachedUIManager->ToggleInventory();
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

void AEWEPlayerController::AcknowledgePossession(APawn *P)
{
    Super::AcknowledgePossession(P);
    AEWECharacter *CharacterBase = Cast<AEWECharacter>(P);

    if (CharacterBase)
    {
        CharacterBase->GetAbilitySystemComponent()->InitAbilityActorInfo(CharacterBase, CharacterBase);
    }
}
