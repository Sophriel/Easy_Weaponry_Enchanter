// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "EWEPreloadSubsystem.generated.h"

/**
 * Preload Core Assets on Gameinstance Initialized
 */
UCLASS()
class EWE_API UEWEPreloadSubsystem : public UGameInstanceSubsystem
{
    GENERATED_BODY()

public:
    virtual void Initialize(FSubsystemCollectionBase &Collection) override;
    virtual void Deinitialize() override;

	bool IsUILoaded() const;

protected:
    UPROPERTY(Transient)
    TObjectPtr<class UEWEPreloadAsset> PreloadConfigAsset;

	UPROPERTY(Transient)
    TObjectPtr<class UEWEUIAsset> UIConfigAsset;
};
