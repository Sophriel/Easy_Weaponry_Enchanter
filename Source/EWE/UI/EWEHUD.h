// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "EWEHUD.generated.h"

/**
 *
 */
UCLASS()
class EWE_API AEWEHUD : public AHUD
{
	GENERATED_BODY()

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = QuickSlot, Meta = (AllowPrivateAccess = "true"))
	TObjectPtr<class UEWEQuickSlot> QuickSlotWidget;

	AEWEHUD();

public:
	UFUNCTION(BlueprintCallable, Category = QuickSlot, Meta = (DisplayName = "SelectSlot"))
	void SelectSlot(int32 SlotIndex);

	UFUNCTION(BlueprintCallable, Category = QuickSlot, Meta = (DisplayName = "SelectSlot"))
	void ScrollSlot(float ScrollDirection);
};
