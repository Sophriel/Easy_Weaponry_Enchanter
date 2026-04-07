// Fill out your copyright notice in the Description page of Project Settings.

#include "EWEUIAsset.h"
#include "EWE/UI/EWELocalUIManageSubsystem.h"
#include "Engine/AssetManager.h"
#include "EWE/EWELog.h"

void UEWEUIAsset::AsyncLoadWidgetClass(UEWELocalUIManageSubsystem *UIManageSubsystem)
{
    CachedUIManager = UIManageSubsystem;

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
        OnWidgetLoaded();
        return;
    }

    StreamableManager.RequestAsyncLoad(WidgetPaths,
                                       FStreamableDelegate::CreateUObject(this, &UEWEUIAsset::OnWidgetLoaded));
}

void UEWEUIAsset::OnWidgetLoaded()
{
    if (!CachedUIManager)
    {
        UE_LOG(LogEWE, Error, TEXT("Missing UIManager!"));
        return;
    }

    CachedUIManager->CreateUIWidgets();
}