// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "EasyWeaponryEnchanter/Weapon/EWEWeaponBase.h"
#include "EWEPlayerWeapon.generated.h"

class UEWEWeaponInstance;
class UEWEWeaponMeshComponent;

/**
 * A weapon actor for the player's quick slots. Kept for the future equipped-weapon drop /
 * pickup feature; it is no longer spawned — the hand is shown by the character's
 * UEWEWeaponMeshComponent instead. The policies below belong to the v11 quick-slot
 * pre-spawn approach and are kept as-is; revisit when drop / pickup is designed.
 */
UCLASS()
class EWE_API AEWEPlayerWeapon : public AEWEWeaponBase
{
	GENERATED_BODY()

public:
	AEWEPlayerWeapon(const FObjectInitializer& ObjectInitializer);

	/**
	 * Server-only: widens or narrows this actor's relevancy for the equipped state.
	 * Equipped = visible to all clients; unequipped = owner-only. No call site while this
	 * actor is unspawned (see class comment).
	 */
	void SetEquippedRelevancy_ServerOnly(bool bEquipped);

	UEWEWeaponInstance* GetInstance() const { return Instance; }

	void SetInstance(UEWEWeaponInstance* InInstance);

protected:
	/** The weapon instance this actor visualizes. nullptr = no weapon. */
	UPROPERTY(ReplicatedUsing = OnRep_Instance, VisibleAnywhere, BlueprintReadOnly, Category = Weapon,
	          meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UEWEWeaponInstance> Instance;

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UFUNCTION()
	void OnRep_Instance();

private:
	/** The mesh component this actor visualizes through, or nullptr if the cast fails. */
	UEWEWeaponMeshComponent* GetWeaponMeshComponent() const;
};
