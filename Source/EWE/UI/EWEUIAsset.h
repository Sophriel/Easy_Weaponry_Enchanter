// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "EWEUIAsset.generated.h"

/**
 *
 */
UCLASS()
class EWE_API UEWEUIAsset : public UDataAsset
{
    GENERATED_BODY()

public:
    UPROPERTY(EditDefaultsOnly, Category = "Inventory")
    TSubclassOf<class UEWEInventory> InventoryWidgetClass;

    UPROPERTY(EditDefaultsOnly, Category = "HUD")
    TSubclassOf<class UEWEQuickSlot> QuickSlotWidgetClass;
};
