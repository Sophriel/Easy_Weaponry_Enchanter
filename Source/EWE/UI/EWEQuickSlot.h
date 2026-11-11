// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "EWEUserWidgetBase.h"
#include "FieldNotificationId.h"
#include "EWEQuickSlot.generated.h"

class UEWEQuickSlotViewModel;
class UEWESlotItem;

/**
 * Quick slot bar widget (View only). Data lives in UEWEQuickSlotViewModel.
 */
UCLASS()
class EWE_API UEWEQuickSlot : public UEWEUserWidgetBase
{
	GENERATED_BODY()

public:
	/** Injects the view model, binds it to this widget's own view, and creates the slot
	 *  widgets. Subscribes to the view model's GetSlotCount so the slots are created when
	 *  the count becomes known. */
	void SetViewModel(UEWEQuickSlotViewModel *InViewModel);

protected:
	UEWEQuickSlot(const FObjectInitializer& ObjectInitializer);
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = QuickSlot, Meta = (AllowPrivateAccess = "true"), Meta = (BindWidget))
	TObjectPtr<class UHorizontalBox> Slots;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = QuickSlot, Meta = (AllowPrivateAccess = "true"))
	TSubclassOf<UEWESlotItem> SlotReference;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = QuickSlot, Meta = (AllowPrivateAccess = "true"))
	TArray<TObjectPtr<UEWESlotItem>> WeaponSlots;

	/** The injected view model. Weak: the subsystem owns it and may tear it down. Kept to
	 *  unsubscribe from GetSlotCount and to create the slot widgets. */
	TWeakObjectPtr<UEWEQuickSlotViewModel> ViewModel;

	/** Handle for the view model's GetSlotCount field-change subscription. */
	FDelegateHandle SlotCountChangedHandle;

private:
	void CreateSlotWidgets();
	void OnSlotCountChanged(UObject *InViewModel, UE::FieldNotification::FFieldId InFieldId);
};
