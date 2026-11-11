// Fill out your copyright notice in the Description page of Project Settings.

#include "EWEPlayerController.h"
#include "EWECharacter.h"
#include "EWE/Character/EWEInventoryComponent.h"
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
        CachedUIManager = LocalPlayer->GetSubsystem<UEWELocalUIManageSubsystem>();
    }

    // Relay the inventory Model. This is a local-PC-only hook; the subscribers are the
    // local UI. Re-possession of the same pawn is a no-op (Old == New).
    UEWEInventoryComponent *NewModel = nullptr;
    // Named CharacterPawn: a local 'Character' would hide AController::Character.
    AEWECharacter *CharacterPawn = Cast<AEWECharacter>(P);
    if (CharacterPawn)
    {
        NewModel = CharacterPawn->GetInventoryComponent();
    }

    UEWEInventoryComponent *OldModel = InventoryModel.Get();
    if (OldModel != NewModel)
    {
        InventoryModel = NewModel;
        OnInventoryModelChanged.Broadcast(OldModel, NewModel);
    }

    if (CachedUIManager)
    {
        CachedUIManager->BindStatus();
        CachedUIManager->BindTargetInfo();
    }
}

#pragma region Inventory

// Delegates to UEWELocalUIManageSubsystem::ToggleInventory.
void AEWEPlayerController::ToggleInventory()
{
    if (CachedUIManager)
    {
        CachedUIManager->ToggleInventory();
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
