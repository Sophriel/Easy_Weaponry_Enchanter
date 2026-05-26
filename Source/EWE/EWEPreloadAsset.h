// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "EWEPreloadAsset.generated.h"

/**
 *
 */
UCLASS()
class EWE_API UEWEPreloadAsset : public UDataAsset
{
    GENERATED_BODY()

public:
    virtual void PostLoad() override;
    void AsyncLoadAssetClass();

    bool IsAssetsLoaded() const { return bIsLoaded; }

protected:
    void OnAssetLoaded();

public:
    UPROPERTY(EditDefaultsOnly, Category = "Preload")
    TArray<TSoftClassPtr<class UObject>> TargetPreloadClasses;

protected:
    TSharedPtr<struct FStreamableHandle> ASyncLoadHandle;

    bool bIsLoaded = false;
};
