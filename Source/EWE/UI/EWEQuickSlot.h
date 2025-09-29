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

public:
	UFUNCTION(BlueprintImplementableEvent, Category = QuickSlot, Meta = (DisplayName = "OnInitSlot"))
	void K2_InitSlot();

	UFUNCTION(BlueprintImplementableEvent, Category = QuickSlot, Meta = (DisplayName = "OnSelectSlot"))
	void K2_SelectSlot(int32 SlotIndex);

	UFUNCTION(BlueprintImplementableEvent, Category = QuickSlot, Meta = (DisplayName = "OnScrollSlot"))
	void K2_ScrollSlot(float ScrollDirection);

protected:
	UEWEQuickSlot(const FObjectInitializer& ObjectInitializer);
	virtual void NativeConstruct() override;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = QuickSlot, Meta = (AllowPrivateAccess = "true"))
	int32 MaxSlotSize;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = QuickSlot, Meta = (AllowPrivateAccess = "true"))
	int32 CurrentSlotIndex;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = QuickSlot, Meta = (AllowPrivateAccess = "true"), Meta = (BindWidget))
	TObjectPtr<class UHorizontalBox> Slots;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = QuickSlot, Meta = (AllowPrivateAccess = "true"))
	TSubclassOf<UUserWidget> SlotReference;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = QuickSlot, Meta = (AllowPrivateAccess = "true"))
	TArray<TObjectPtr<UUserWidget>> WeaponSlots;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = QuickSlot, meta = (AllowPrivateAccess = "true"))
	TArray<TObjectPtr<class UEWEWeaponData>> Weapons;

public:
	void InitWeaponSlots();
	void AddWeapon(class UEWEWeaponData* Weapon);
	void SetWeaponSlot(int32 SlotIndex, class UEWEWeaponData* Weapon);
};
