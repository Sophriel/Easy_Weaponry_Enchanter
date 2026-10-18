// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "EWEStatus.generated.h"

/**
 * Status widget — implements visual processing only. No knowledge of data required.
 * Data binding is handled on the BP side via UEWEStatusViewModel.
 */
UCLASS()
class EWE_API UEWEStatus : public UUserWidget
{
    GENERATED_BODY()
};
