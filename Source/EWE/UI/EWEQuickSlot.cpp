// Fill out your copyright notice in the Description page of Project Settings.

#include "EWEQuickSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "View/MVVMView.h"
#include "INotifyFieldValueChanged.h"
#include "EWE/UI/EWEQuickSlotViewModel.h"
#include "EWE/UI/EWESlotItem.h"
#include "EWE/EWELog.h"

UEWEQuickSlot::UEWEQuickSlot(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

void UEWEQuickSlot::NativeConstruct()
{
	Super::NativeConstruct();
	// Slot widgets are created in SetViewModel(), not here.
}

void UEWEQuickSlot::NativeDestruct()
{
	// Unsubscribe from the view model's GetSlotCount before it is torn down.
	UEWEQuickSlotViewModel *VM = ViewModel.Get();
	if (VM)
	{
		VM->RemoveFieldValueChangedDelegate(
			UEWEQuickSlotViewModel::FFieldNotificationClassDescriptor::GetSlotCount,
			SlotCountChangedHandle);
	}
	SlotCountChangedHandle.Reset();

	Super::NativeDestruct();
}

void UEWEQuickSlot::SetViewModel(UEWEQuickSlotViewModel *InViewModel)
{
	if (!InViewModel)
	{
		EWE_LOG(LogEWE, Warning, TEXT("SetViewModel: quick slot view model is null"));
		return;
	}

	// Same view model — already bound. The subsystem injects once, but guard against a
	// duplicate subscription if it is called again.
	if (InViewModel == ViewModel.Get())
	{
		return;
	}

	// Unsubscribe from the previous view model before switching.
	UEWEQuickSlotViewModel *Previous = ViewModel.Get();
	if (Previous)
	{
		Previous->RemoveFieldValueChangedDelegate(
			UEWEQuickSlotViewModel::FFieldNotificationClassDescriptor::GetSlotCount,
			SlotCountChangedHandle);
	}
	SlotCountChangedHandle.Reset();

	ViewModel = InViewModel;

	// Bind this widget's own view to the view model. The name must match the view model
	// declared in the WBP (BP_QuickSlot), or the binding silently does nothing.
	UMVVMView *MVVMView = GetExtension<UMVVMView>();
	if (MVVMView)
	{
		MVVMView->SetViewModel(FName(TEXT("EWEQuickSlotViewModel")), InViewModel);
	}
	else
	{
		EWE_LOG(LogEWE, Warning, TEXT("Quick slot widget has no MVVMView extension. Enable ViewModel in WBP Class Settings."));
	}

	// Subscribe to the slot count so the slots are created when the count becomes known.
	SlotCountChangedHandle = InViewModel->AddFieldValueChangedDelegate(
		UEWEQuickSlotViewModel::FFieldNotificationClassDescriptor::GetSlotCount,
		INotifyFieldValueChanged::FFieldValueChangedDelegate::CreateUObject(this, &UEWEQuickSlot::OnSlotCountChanged));

	// The count may already be known — create the slots now.
	CreateSlotWidgets();
}

void UEWEQuickSlot::CreateSlotWidgets()
{
	UEWEQuickSlotViewModel *VM = ViewModel.Get();
	if (!VM)
		return;

	const int32 SlotCount = VM->GetSlotCount();
	if (SlotCount < 1)
		return;

	if (!SlotReference)
	{
		EWE_LOG(LogEWE, Warning, TEXT("CreateSlotWidgets: SlotReference is not set"));
		return;
	}

	// If the slots are already sized to this count, keep them — only re-pass the item
	// view models below. This matters when the view model is replaced: the kept widgets
	// must not stay bound to the previous view model's items.
	if (WeaponSlots.Num() != SlotCount)
	{
		// Clear existing slots if reinitializing
		Slots->ClearChildren();
		WeaponSlots.Reset();

		WeaponSlots.SetNum(SlotCount);

		for (int32 i = 0; i < SlotCount; ++i)
		{
			TObjectPtr<UEWESlotItem> SlotWidget = CreateWidget<UEWESlotItem>(GetWorld(), SlotReference);
			WeaponSlots[i] = SlotWidget;

			TObjectPtr<UHorizontalBoxSlot> SlotHandle = Slots->AddChildToHorizontalBox(SlotWidget);

			FSlateChildSize NewSize;
			NewSize.SizeRule = ESlateSizeRule::Fill;
			SlotHandle->SetSize(NewSize);
			SlotHandle->SetHorizontalAlignment(HAlign_Center);
			SlotHandle->SetVerticalAlignment(VAlign_Center);
		}
	}

	// Pass each slot widget its item view model. Runs on every call — a replaced view
	// model must rebind the kept widgets to its own items.
	for (int32 i = 0; i < WeaponSlots.Num(); ++i)
	{
		UEWESlotItem *SlotWidget = WeaponSlots[i];
		if (!SlotWidget)
			continue;

		SlotWidget->SetViewModel(VM->GetSlotItem(i));
	}
}

void UEWEQuickSlot::OnSlotCountChanged(UObject *InViewModel, UE::FieldNotification::FFieldId InFieldId)
{
	// The view model broadcast that its slot count changed — (re)create the slots.
	CreateSlotWidgets();
}
