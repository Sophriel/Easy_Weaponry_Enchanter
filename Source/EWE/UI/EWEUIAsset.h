// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "EWEUIAsset.generated.h"

DECLARE_MULTICAST_DELEGATE(FOnUIAssetsLoaded);

/**
 *
 */
UCLASS()
class EWE_API UEWEUIAsset : public UDataAsset
{
    GENERATED_BODY()

public:
    virtual void PostLoad() override;
    void AsyncLoadWidgetClass();

    bool IsWidgetsLoaded() const { return bIsLoaded; }

protected:
    void OnWidgetLoaded();

public:
    // Hardcoded for Essential Widgets
    UPROPERTY(EditDefaultsOnly, Category = "Inventory")
    TSoftClassPtr<class UUserWidget> InventoryWidgetClass;

    UPROPERTY(EditDefaultsOnly, Category = "HUD")
    TSoftClassPtr<class UUserWidget> QuickSlotWidgetClass;

    UPROPERTY(EditDefaultsOnly, Category = "HUD")
    TSoftClassPtr<class UUserWidget> StatusWidgetClass;

    UPROPERTY(EditDefaultsOnly, Category = "HUD")
    TSoftClassPtr<class UUserWidget> TargetInfoWidgetClass;

    FOnUIAssetsLoaded OnUILoaded;
    
protected:
    TSharedPtr<struct FStreamableHandle> ASyncLoadHandle;

    bool bIsLoaded = false;
};
