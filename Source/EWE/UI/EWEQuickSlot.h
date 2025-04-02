// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "EWEQuickSlot.generated.h"

/**
 *
 */
UCLASS()
class EWE_API UEWEQuickSlot : public UUserWidget
{
	GENERATED_BODY()

protected:
	UEWEQuickSlot(const FObjectInitializer& ObjectInitializer);
	virtual void NativeConstruct() override;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = QuickSlot, Meta = (AllowPrivateAccess = "true"))
	int32 CurrentSlotIndex;

public:
	UFUNCTION(BlueprintImplementableEvent, Category = QuickSlot, Meta = (DisplayName = "OnSelectSlot"))
	void K2_SelectSlot(int32 SlotIndex);

	UFUNCTION(BlueprintImplementableEvent, Category = QuickSlot, Meta = (DisplayName = "OnScrollSlot"))
	void K2_ScrollSlot(float ScrollDirection);
};
