// Fill out your copyright notice in the Description page of Project Settings.


#include "EWEAnimNotify_Release.h"
#include "EWEAttackAnimationInterface.h"

void UEWEAnimNotify_Release::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference)
{
	Super::Notify(MeshComp, Animation, EventReference);

	if (MeshComp != nullptr)
	{
		IEWEAttackAnimationInterface* AttackPawn = Cast<IEWEAttackAnimationInterface>(MeshComp->GetOwner());
		if (AttackPawn != nullptr)
		{
			AttackPawn->AttackReleaseCheck();
		}
	}
}
