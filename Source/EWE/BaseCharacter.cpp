// Fill out your copyright notice in the Description page of Project Settings.


#include "BaseCharacter.h"
#include "Sword.h"

// Sets default values
ABaseCharacter::ABaseCharacter()
{
	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

}

// Called when the game starts or when spawned
void ABaseCharacter::BeginPlay()
{
	Super::BeginPlay();

	SpawnSword();
}

void ABaseCharacter::SpawnSword()
{
	FActorSpawnParameters SpawnInfo;

	ArmMeshes = GetComponentsByTag(USkeletalMeshComponent::StaticClass(), "Arm");

	if (ArmMeshes[0] != NULL && SwordMesh != NULL)
	{
		ASword* DefalutSword = GetWorld()->SpawnActor<ASword>(SwordMesh, FVector::ZeroVector, FRotator(0, 90, 0));

		DefalutSword->AttachToComponent((USceneComponent*)ArmMeshes[0], FAttachmentTransformRules::KeepRelativeTransform, TEXT("SwordSocket"));
		DefalutSword->SetOwner(this);
	}
}

// Called every frame
void ABaseCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

// Called to bind functionality to input
void ABaseCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

}

