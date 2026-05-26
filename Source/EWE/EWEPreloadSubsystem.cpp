// Fill out your copyright notice in the Description page of Project Settings.

#include "EWEPreloadSubsystem.h"
#include "EWE/EWELog.h"
#include "EWE/EWESettings.h"
#include "EWEPreloadAsset.h"
#include "UI/EWEUIAsset.h"

void UEWEPreloadSubsystem::Initialize(FSubsystemCollectionBase &Collection)
{
    Super::Initialize(Collection);

    // Pre-load UIAssets during initialization
    const UEWESettings *Settings = GetDefault<UEWESettings>();
    if (!Settings)
    {
        EWE_LOG(LogEWE, Error, TEXT("EWESettings not found!"));
        return;
    }

    PreloadConfigAsset = Settings->PreloadConfigAsset.LoadSynchronous();
    UIConfigAsset = Settings->UIConfigAsset.LoadSynchronous();
}

void UEWEPreloadSubsystem::Deinitialize() { Super::Deinitialize(); }

bool UEWEPreloadSubsystem::IsUILoaded() const
{
    if (!UIConfigAsset)
        return false;

    return UIConfigAsset->IsWidgetsLoaded();
}