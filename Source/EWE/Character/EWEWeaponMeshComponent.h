// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/SkeletalMeshComponent.h"
#include "EWEWeaponMeshComponent.generated.h"

class UEWEWeaponInstance;

/**
 * Displays a weapon instance's mesh. Owns the display state (the shown instance and its
 * load subscription) so the owner only calls SetInstance. The subscription is kept after
 * the load finishes: a runtime reload of the shown instance reconfigures through the same
 * path. Applies the mesh only — no socket alignment, and the relative transform is left
 * untouched.
 */
UCLASS()
class EWE_API UEWEWeaponMeshComponent : public USkeletalMeshComponent
{
	GENERATED_BODY()

public:
	/**
	 * Sets the instance this component displays. A no-op if it is already displayed.
	 * nullptr clears the mesh immediately.
	 */
	void SetInstance(UEWEWeaponInstance* InInstance);

	/** The instance currently displayed, or nullptr. */
	UEWEWeaponInstance* GetInstance() const { return DisplayedInstance.Get(); }

protected:
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	/** Reacts to the displayed instance finishing loading: configures if it is still the current one. */
	void OnInstanceLoadFinished(UEWEWeaponInstance* FinishedInstance);

	/** Applies the instance's mesh. A null mesh clears the component. */
	void ConfigureFromInstance();

	/** Drops the load-finished subscription, if any. */
	void ClearLoadSubscription();

	/** The instance currently displayed (weak: owned by the inventory). */
	TWeakObjectPtr<UEWEWeaponInstance> DisplayedInstance;

	FDelegateHandle LoadFinishedHandle;
};
