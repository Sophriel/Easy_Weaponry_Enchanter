// Fill out your copyright notice in the Description page of Project Settings.

#include "EWEQuickSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Kismet/GameplayStatics.h"
#include "View/MVVMView.h"
#include "EWE/UI/EWEQuickSlotViewModel.h"
#include "EWE/UI/EWEQuickSlotItemViewModel.h"
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

void UEWEQuickSlot::SetViewModel(UEWEQuickSlotViewModel *InViewModel)
{
	if (!InViewModel)
		return;

	// The widget owns the slot count and sizes the view model to match.
	InViewModel->InitializeSlots(MaxSlotSize);
	InitWeaponSlots(MaxSlotSize);

	for (int32 i = 0; i < WeaponSlots.Num(); ++i)
	{
		UUserWidget *SlotWidget = WeaponSlots[i];
		if (!SlotWidget)
			continue;

		UMVVMView *SlotView = SlotWidget->GetExtension<UMVVMView>();
		if (!SlotView)
		{
			EWE_LOG(LogEWE, Warning,
					TEXT("Quick slot entry widget has no MVVMView extension. Enable ViewModel in WBP Class Settings."));
			continue;
		}

		// The Blueprint-side view model name stays inside this widget. The subsystem
		// must not need to know it.
		SlotView->SetViewModel(FName(TEXT("EWEQuickSlotItemViewModel")), InViewModel->GetSlotItem(i));
	}
}

void UEWEQuickSlot::InitWeaponSlots(int32 SlotCount)
{
	if (SlotCount < 1)
		return;

	if (!SlotReference)
	{
		EWE_LOG(LogEWE, Warning, TEXT("InitWeaponSlots: SlotReference is not set"));
		return;
	}

	// Clear existing slots if reinitializing
	Slots->ClearChildren();
	WeaponSlots.Reset();

	WeaponSlots.SetNum(SlotCount);

	for (int32 i = 0; i < SlotCount; ++i)
	{
		TObjectPtr<UUserWidget> SlotWidget = CreateWidget<UUserWidget>(GetWorld(), SlotReference);
		WeaponSlots[i] = SlotWidget;

		TObjectPtr<UHorizontalBoxSlot> SlotHandle = Slots->AddChildToHorizontalBox(SlotWidget);

		FSlateChildSize NewSize;
		NewSize.SizeRule = ESlateSizeRule::Fill;
		SlotHandle->SetSize(NewSize);
		SlotHandle->SetHorizontalAlignment(HAlign_Center);
		SlotHandle->SetVerticalAlignment(VAlign_Center);
	}
}
