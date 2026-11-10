// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "EWETargetInfo.generated.h"

/**
 * Target info widget — implements visual processing only. No knowledge of data required.
 * Data binding is handled on the BP side via UEWETargetInfoViewModel; visibility is
 * driven by the view model's HasTarget binding.
 */
UCLASS()
class EWE_API UEWETargetInfo : public UUserWidget
{
	GENERATED_BODY()

public:
	/** Injects the target info view model and binds it to this widget's own view. */
	void SetViewModel(class UEWETargetInfoViewModel *InViewModel);
};
