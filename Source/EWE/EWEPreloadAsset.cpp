// Fill out your copyright notice in the Description page of Project Settings.

#include "EWEPreloadAsset.h"
#include "Engine/AssetManager.h"
#include "EWE/EWELog.h"

void UEWEPreloadAsset::PostLoad()
{
    Super::PostLoad();

    if (bIsLoaded)
        return;

    AsyncLoadAssetClass();
}

void UEWEPreloadAsset::AsyncLoadAssetClass()
{
    if (ASyncLoadHandle.IsValid() && !ASyncLoadHandle->HasLoadCompleted())
        return; // Load already in flight.

    FStreamableManager &StreamableManager = UAssetManager::GetStreamableManager();
    TArray<FSoftObjectPath> WidgetPaths;

    for (const TSoftClassPtr<UObject> &TargetPreloadClass : TargetPreloadClasses)
    {
        if (TargetPreloadClass.IsPending())
            WidgetPaths.Add(TargetPreloadClass.ToSoftObjectPath());
    }

    if (WidgetPaths.IsEmpty())
    {
        return;
    }

    ASyncLoadHandle = StreamableManager.RequestAsyncLoad(
        WidgetPaths, FStreamableDelegate::CreateUObject(this, &UEWEPreloadAsset::OnAssetLoaded));
}

void UEWEPreloadAsset::OnAssetLoaded()
{
    EWE_LOG(LogEWE, Log, TEXT("Preload Assets loaded"));
    bIsLoaded = true;
}