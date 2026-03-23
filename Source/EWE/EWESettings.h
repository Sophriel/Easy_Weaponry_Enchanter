// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "EWESettings.generated.h"

/**
 *
 */
UCLASS(Config=Game, DefaultConfig, meta=(DisplayName="EWE Settings"))
class EWE_API UEWESettings : public UDeveloperSettings
{
    GENERATED_BODY()
public:
    // UI config DataAsset
    UPROPERTY(Config, EditAnywhere, Category = "UI")
    TSoftObjectPtr<class UEWEUIAsset> UIConfigAsset;
};
