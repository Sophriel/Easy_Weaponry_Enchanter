// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "EWESettings.generated.h"

/** EWE game settings: preload and UI configuration. */
UCLASS(Config = Game, DefaultConfig, meta = (DisplayName = "EWE Settings"))
class EWE_API UEWESettings : public UDeveloperSettings
{
    GENERATED_BODY()
public:
    /** The asset listing the classes to preload at startup. */
    UPROPERTY(Config, EditAnywhere, Category = "Preload")
    TSoftObjectPtr<class UEWEPreloadAsset> PreloadConfigAsset;

    /** The asset configuring the UI widget classes. */
    UPROPERTY(Config, EditAnywhere, Category = "UI")
    TSoftObjectPtr<class UEWEUIAsset> UIConfigAsset;
};
