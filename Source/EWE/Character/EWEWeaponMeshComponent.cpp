// Fill out your copyright notice in the Description page of Project Settings.

#include "EWEWeaponMeshComponent.h"
#include "EasyWeaponryEnchanter/Weapon/EWEWeaponInstance.h"

void UEWEWeaponMeshComponent::SetInstance(UEWEWeaponInstance* InInstance)
{
	// No-op only when switching to the same valid instance. A null request always clears,
	// even if the displayed instance was already destroyed (weak ref now null).
	if (InInstance && InInstance == DisplayedInstance.Get())
	{
		return;
	}

	// Drop any previous subscription before switching targets.
	ClearLoadSubscription();

	DisplayedInstance = InInstance;

	if (!InInstance)
	{
		SetSkeletalMesh(nullptr);
		return;
	}

	// Subscribe so we configure once it finishes on this machine. The subscription is kept
	// after the load finishes: a runtime reload of the shown instance reconfigures through
	// the same path.
	LoadFinishedHandle = InInstance->OnLoadFinished.AddUObject(this, &UEWEWeaponMeshComponent::OnInstanceLoadFinished);

	if (InInstance->IsLoadFinished())
	{
		ConfigureFromInstance();
	}
	// Still loading: keep the current mesh and configure when the load finishes.
}

void UEWEWeaponMeshComponent::OnInstanceLoadFinished(UEWEWeaponInstance* FinishedInstance)
{
	// Only react if this is still the instance this component displays.
	if (FinishedInstance != DisplayedInstance.Get())
	{
		return;
	}

	ConfigureFromInstance();
}

void UEWEWeaponMeshComponent::ConfigureFromInstance()
{
	UEWEWeaponInstance* Instance = DisplayedInstance.Get();
	if (!Instance)
	{
		return;
	}

	// Apply the mesh only — no socket alignment. A null mesh (load failed or no mesh asset)
	// is applied as-is so the previous weapon does not linger.
	SetSkeletalMesh(Instance->GetWeaponMesh());
}

void UEWEWeaponMeshComponent::ClearLoadSubscription()
{
	UEWEWeaponInstance* Displayed = DisplayedInstance.Get();
	if (Displayed)
	{
		Displayed->OnLoadFinished.Remove(LoadFinishedHandle);
	}
	LoadFinishedHandle.Reset();
}

void UEWEWeaponMeshComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	ClearLoadSubscription();
	Super::EndPlay(EndPlayReason);
}
