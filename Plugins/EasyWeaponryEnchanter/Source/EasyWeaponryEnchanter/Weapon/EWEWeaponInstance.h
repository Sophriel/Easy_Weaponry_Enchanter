// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/StreamableManager.h"
#include "EWEWeaponInstance.generated.h"

class UEWEWeaponData;

DECLARE_MULTICAST_DELEGATE_OneParam(FOnWeaponInstanceLoadFinished, class UEWEWeaponInstance *);

/** Load state of a weapon instance. */
UENUM()
enum class EEWEWeaponLoadState : uint8
{
    NotLoaded,
    LoadingData,
    LoadingAssets,
    Ready,
    Failed,
};

/**
 * One owned weapon: a single weapon definition plus its loaded assets.
 * Created by the inventory component (Outer), which owns its replication condition:
 * owner-only while unequipped, all clients while equipped. Every machine that receives
 * the instance loads its own data and assets.
 */
UCLASS()
class EASYWEAPONRYENCHANTER_API UEWEWeaponInstance : public UObject
{
    GENERATED_BODY()

public:
    /** Broadcast when the instance finishes loading (Ready or Failed). */
    FOnWeaponInstanceLoadFinished OnLoadFinished;

    /**
     * Sets the weapon definition. Called once by the inventory right after creation; a
     * second call is a no-op with a Warning — the definition is immutable afterwards.
     */
    void InitializeWeaponData(TSoftObjectPtr<UEWEWeaponData> InWeaponData);

    TSoftObjectPtr<UEWEWeaponData> GetWeaponData() const { return WeaponData; }

    /** Loaded definition. Filled on the server and on every client. */
    class UEWEWeaponData *GetLoadedWeaponData() const { return LoadedWeaponData; }

    class USkeletalMesh *GetWeaponMesh() const { return WeaponMesh; }

    class UTexture2D *GetWeaponIcon() const { return WeaponIcon; }

    const TArray<TSubclassOf<class UGameplayAbility>> &GetHoldAbilities() const { return HoldAbilities; }

    const TArray<TSubclassOf<class UGameplayAbility>> &GetHitAbilities() const { return HitAbilities; }

    const TArray<TSubclassOf<class UGameplayAbility>> &GetOnEquipAbilities() const { return OnEquipAbilities; }

    EEWEWeaponLoadState GetLoadState() const { return LoadState; }

    bool IsLoadFinished() const
    {
        return LoadState == EEWEWeaponLoadState::Ready || LoadState == EEWEWeaponLoadState::Failed;
    }

    /**
     * Starts the load pipeline (data, then sub-assets). Idempotent: a second call while
     * already loading or finished is a no-op. The server calls it right after
     * InitializeWeaponData; clients enter through OnRep_WeaponData.
     */
    void BeginLoad();

protected:
    /** The weapon definition. Set once at creation; immutable afterwards. */
    UPROPERTY(ReplicatedUsing = OnRep_WeaponData)
    TSoftObjectPtr<UEWEWeaponData> WeaponData;

    /** Loaded definition. Filled on the server and on every client. */
    UPROPERTY(Transient)
    TObjectPtr<UEWEWeaponData> LoadedWeaponData;

    UPROPERTY(Transient)
    TObjectPtr<class USkeletalMesh> WeaponMesh;

    UPROPERTY(Transient)
    TObjectPtr<class UTexture2D> WeaponIcon;

    UPROPERTY(Transient)
    TArray<TSubclassOf<class UGameplayAbility>> HoldAbilities;

    UPROPERTY(Transient)
    TArray<TSubclassOf<class UGameplayAbility>> HitAbilities;

    UPROPERTY(Transient)
    TArray<TSubclassOf<class UGameplayAbility>> OnEquipAbilities;

    UFUNCTION()
    void OnRep_WeaponData();

    virtual bool IsSupportedForNetworking() const override { return true; }

    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty> &OutLifetimeProps) const override;

    void OnWeaponDataLoaded();
    void OnSubAssetsLoaded();
    void FinishLoad(EEWEWeaponLoadState FinalState);
    bool LoadAbilities(const TArray<TSoftClassPtr<class UGameplayAbility>> &Source,
                       TArray<TSubclassOf<class UGameplayAbility>> &Out,
                       const TCHAR *Category);

    TSharedPtr<FStreamableHandle> DataLoadHandle;
    TSharedPtr<FStreamableHandle> SubAssetLoadHandle;

    EEWEWeaponLoadState LoadState = EEWEWeaponLoadState::NotLoaded;
};
