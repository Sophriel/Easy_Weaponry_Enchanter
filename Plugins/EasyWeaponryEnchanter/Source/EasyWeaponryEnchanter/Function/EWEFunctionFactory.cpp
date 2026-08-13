#include "EasyWeaponryEnchanter/Function/EWEFunctionFactory.h"

#include "Abilities/GameplayAbility.h"
#include "Camera/PlayerCameraManager.h"
#include "EasyWeaponryEnchanter/AbilityTask/EWEAT_SpawnActor.h"
#include "GameFramework/PlayerController.h"

namespace
{
    AActor *ResolveReferenceActor(UGameplayAbility *OwningAbility, const FEnchantContext &EnchantContext)
    {
        if (EnchantContext.AvatarActor.IsValid())
        {
            return EnchantContext.AvatarActor.Get();
        }

        if (EnchantContext.SourceActor.IsValid())
        {
            return EnchantContext.SourceActor.Get();
        }

        return OwningAbility ? OwningAbility->GetAvatarActorFromActorInfo() : nullptr;
    }

    AActor *ResolveSourceActor(UGameplayAbility *OwningAbility, const FEnchantContext &EnchantContext)
    {
        if (EnchantContext.SourceActor.IsValid())
        {
            return EnchantContext.SourceActor.Get();
        }

        return OwningAbility ? OwningAbility->GetOwningActorFromActorInfo() : nullptr;
    }
} // namespace

UEWEAT_Base *FEWEFunctionTaskFactory::CreateTask(UGameplayAbility *OwningAbility, const FEnchantFunction &FunctionData,
                                                 const FEnchantContext &EnchantContext)
{
    if (!OwningAbility)
    {
        return nullptr;
    }

    switch (FunctionData.FunctionType)
    {
    case EEnchantFunctionType::Creation:
        if (!FunctionData.ActorToSpawn)
        {
            return nullptr;
        }

        return UEWEAT_SpawnActor::SpawnActor(OwningAbility,
                                             BuildCreationTargetData(OwningAbility, FunctionData, EnchantContext),
                                             FunctionData.ActorToSpawn);

    default:
        return nullptr;
    }
}

FGameplayAbilityTargetDataHandle FEWEFunctionTaskFactory::BuildCreationTargetData(UGameplayAbility *OwningAbility,
                                                                                  const FEnchantFunction &FunctionData,
                                                                                  const FEnchantContext &EnchantContext)
{
    FTransform SpawnTransform = FunctionData.TargetLocation;
    AActor *const ReferenceActor = ResolveReferenceActor(OwningAbility, EnchantContext);

    switch (FunctionData.LocationType)
    {
    case ELocationType::Character:
    {
        if (ReferenceActor)
        {
            SpawnTransform = ReferenceActor->GetActorTransform() + FunctionData.TargetLocation;
        }
        break;
    }

    case ELocationType::View:
    {
        if (ReferenceActor)
        {
            const FVector SpawnLocation =
                ReferenceActor->GetActorLocation() + (ReferenceActor->GetActorForwardVector() * 200.f);
            SpawnTransform = FTransform(ReferenceActor->GetActorRotation(), SpawnLocation);
        }
        break;
    }

    case ELocationType::Camera:
    {
        if (!OwningAbility || !ReferenceActor)
        {
            break;
        }

        if (const FGameplayAbilityActorInfo *ActorInfo = OwningAbility->GetCurrentActorInfo())
        {
            if (APlayerController *PlayerController = Cast<APlayerController>(ActorInfo->PlayerController.Get()))
            {
                FVector CharacterLocation = ActorInfo->OwnerActor->GetActorLocation();
                FVector CameraLocation = FVector::ZeroVector;
                FRotator CameraRotation = FRotator::ZeroRotator;
                PlayerController->GetPlayerViewPoint(CameraLocation, CameraRotation);

                const FVector SpawnLocation = CharacterLocation + (CameraRotation.Vector() * FunctionData.TargetDistance);
                SpawnTransform = FTransform(CameraRotation, SpawnLocation);
            }
        }
        break;
    }

    case ELocationType::Target:
    case ELocationType::AnchorObject:
    default:
        break;
    }

    TSharedPtr<FGameplayAbilityTargetData_LocationInfo> TargetLocationData =
        MakeShared<FGameplayAbilityTargetData_LocationInfo>();
    TargetLocationData->SourceLocation.SourceAbility = OwningAbility;
    TargetLocationData->SourceLocation.LocationType = EGameplayAbilityTargetingLocationType::ActorTransform;
    TargetLocationData->SourceLocation.SourceActor = ResolveSourceActor(OwningAbility, EnchantContext);
    TargetLocationData->TargetLocation.LiteralTransform = SpawnTransform;

    FGameplayAbilityTargetDataHandle TargetData;
    TargetData.Add(TargetLocationData.Get());
    return TargetData;
}
