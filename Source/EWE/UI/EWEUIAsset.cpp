// Fill out your copyright notice in the Description page of Project Settings.

#include "EWEUIAsset.h"
#include "Engine/AssetManager.h"
#include "EWE/EWELog.h"

void UEWEUIAsset::PostLoad()
{
    Super::PostLoad();

    if (bIsLoaded)
        return;

    AsyncLoadWidgetClass();
}

void UEWEUIAsset::AsyncLoadWidgetClass()
{
    if (ASyncLoadHandle.IsValid() && !ASyncLoadHandle->HasLoadCompleted())
        return;  // Load already in flight.

    FStreamableManager &StreamableManager = UAssetManager::GetStreamableManager();
    TArray<FSoftObjectPath> WidgetPaths;

    if (InventoryWidgetClass.IsPending())
        WidgetPaths.Add(InventoryWidgetClass.ToSoftObjectPath());

    if (QuickSlotWidgetClass.IsPending())
        WidgetPaths.Add(QuickSlotWidgetClass.ToSoftObjectPath());

    if (StatusWidgetClass.IsPending())
        WidgetPaths.Add(StatusWidgetClass.ToSoftObjectPath());

    if (TargetInfoWidgetClass.IsPending())
        WidgetPaths.Add(TargetInfoWidgetClass.ToSoftObjectPath());

    if (WidgetPaths.IsEmpty())
    {
        return;
    }

    ASyncLoadHandle = StreamableManager.RequestAsyncLoad(
        WidgetPaths, FStreamableDelegate::CreateUObject(this, &UEWEUIAsset::OnWidgetLoaded));
}

void UEWEUIAsset::OnWidgetLoaded()
{
    EWE_LOG(LogEWE, Log, TEXT("UIAssets loaded"));
    bIsLoaded = true;
    OnUILoaded.Broadcast();
}