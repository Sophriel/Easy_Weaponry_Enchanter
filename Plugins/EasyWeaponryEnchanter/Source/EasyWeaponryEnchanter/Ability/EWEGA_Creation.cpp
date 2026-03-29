// Fill out your copyright notice in the Description page of Project Settings.

#include "EWEGA_Creation.h"
#include "EasyWeaponryEnchanter/Public/EasyWeaponryEnchanter.h"
#include "EasyWeaponryEnchanter/AbilityTask/EWEAT_SpawnActor.h"

void UEWEGA_Creation::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
                                      const FGameplayAbilityActorInfo *ActorInfo,
                                      const FGameplayAbilityActivationInfo ActivationInfo,
                                      const FGameplayEventData *TriggerEventData)
{
    Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

    if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
    {
        return;
    }

    if (ActorToSpawn == nullptr)
    {
        EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
        return;
    }

    SetActorLocation();
    UEWEAT_SpawnActor *SpawnTask = UEWEAT_SpawnActor::SpawnActor(this, TargetData, ActorToSpawn);
    check(SpawnTask);

    SpawnTask->Success.AddDynamic(this, &UEWEGA_Creation::OnActorSpawned);
    SpawnTask->DidNotSpawn.AddDynamic(this, &UEWEGA_Creation::OnActorSpawnFailed);

    SpawnTask->ReadyForActivation();

    if (ActorInfo != NULL && ActorInfo->AvatarActor != NULL)
    {
        EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
    }
}

void UEWEGA_Creation::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo *ActorInfo,
                                 const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility,
                                 bool bWasCancelled)
{
    Super::EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
}

void UEWEGA_Creation::OnActorSpawned(AActor *SpawnedActor)
{
    check(SpawnedActor);

    EWE_LOG(LogEWE, Log, TEXT("EWEGA_Creation: Spawned Actor"));
}

void UEWEGA_Creation::OnActorSpawnFailed(AActor *SpawnedActor)
{
    EWE_LOG(LogEWE, Error, TEXT("EWEGA_Creation: Actor Spawn Failed"));
}

void UEWEGA_Creation::SetActorLocation()
{
    FTransform SpawnTransform; // Initialize to identity transform
    AActor* OwnerCharacter = GetAvatarActorFromActorInfo();

    switch (LocationType)
    {
    case ELocationType::Character:
    {
        if (OwnerCharacter)
        {
            SpawnTransform = OwnerCharacter->GetActorTransform() + TargetLocation;
        }
        break;
    }

    case ELocationType::View:
    {
        if (OwnerCharacter)
        {
            FVector Loc = OwnerCharacter->GetActorLocation() + OwnerCharacter->GetActorForwardVector() * 200.f;
            SpawnTransform = FTransform(OwnerCharacter->GetActorRotation(), Loc);
        }
        break;
    }

    case ELocationType::Camera:
    {
        // Gets the camera position and rotation
        APlayerController* PC = Cast<APlayerController>(GetCurrentActorInfo()->PlayerController.Get());
        if (PC && PC->PlayerCameraManager && OwnerCharacter)
        {
            FVector CamLoc;
            FRotator CamRot;
            PC->GetPlayerViewPoint(CamLoc, CamRot);
            FVector CharLoc = OwnerCharacter->GetActorLocation();

            FVector SpawnLocation = CharLoc + CamRot.Vector() * TargetDistance;
            SpawnTransform = FTransform(CamRot, SpawnLocation);
        }
        break;
    }

    case ELocationType::Target:
    {
        // Target: Set by Blueprint or Ability system
        SpawnTransform = TargetLocation;
        break;
    }

    case ELocationType::AnchorObject:
    {
        // AnchorObject: To be implemented with anchor actor reference
        // if (AnchorActor)
        // 	SpawnTransform = AnchorActor->GetActorTransform();
        // else
        // 	SpawnTransform = TargetLocation; // fallback
        break;
    }
    }

    TSharedPtr<FGameplayAbilityTargetData_LocationInfo> TargetLocationData = MakeShareable(new FGameplayAbilityTargetData_LocationInfo());
    TargetLocationData->SourceLocation.SourceAbility = this;
    TargetLocationData->SourceLocation.LocationType = EGameplayAbilityTargetingLocationType::ActorTransform;
    TargetLocationData->SourceLocation.SourceActor = GetOwningActorFromActorInfo();
    TargetLocationData->TargetLocation.LiteralTransform = SpawnTransform;

    TargetData.Add(TargetLocationData.Get());
}
