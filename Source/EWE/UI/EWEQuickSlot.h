// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "EWEQuickSlot.generated.h"

/**
 * Quick slot bar widget (View only). Data lives in UEWEQuickSlotViewModel.
 */
UCLASS()
class EWE_API UEWEQuickSlot : public UUserWidget
{
	GENERATED_BODY()

public:
	/** Injects the view model and pushes each entry into its slot widget. */
	void SetViewModel(class UEWEQuickSlotViewModel *InViewModel);

protected:
	UEWEQuickSlot(const FObjectInitializer& ObjectInitializer);
	virtual void NativeConstruct() override;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = QuickSlot, Meta = (AllowPrivateAccess = "true"), Meta = (BindWidget))
	TObjectPtr<class UHorizontalBox> Slots;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = QuickSlot, Meta = (AllowPrivateAccess = "true"))
	TSubclassOf<UUserWidget> SlotReference;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = QuickSlot, Meta = (AllowPrivateAccess = "true"))
	TArray<TObjectPtr<UUserWidget>> WeaponSlots;

	/** Number of quick slots to create. Drives the view model's slot array size. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = QuickSlot,
	          Meta = (AllowPrivateAccess = "true", ClampMin = "1", UIMin = "1"))
	int32 MaxSlotSize = 5;

private:
	void InitWeaponSlots(int32 SlotCount);
};
