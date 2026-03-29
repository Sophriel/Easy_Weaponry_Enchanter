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
    void AsyncLoadWidgetClass(class UEWELocalUIManageSubsystem* UIManageSubsystem);

protected:
    void OnWidgetLoaded();

    TObjectPtr<class UEWELocalUIManageSubsystem> CachedUIManager;

public:
    // Hardcoded for Essential Widgets
    UPROPERTY(EditDefaultsOnly, Category = "Inventory")
    TSoftClassPtr<class UUserWidget> InventoryWidgetClass;

    UPROPERTY(EditDefaultsOnly, Category = "HUD")
    TSoftClassPtr<class UUserWidget> QuickSlotWidgetClass;

    UPROPERTY(EditDefaultsOnly, Category = "HUD")
    TSoftClassPtr<class UUserWidget> StatusWidgetClass;
};
