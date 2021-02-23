// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "BaseCharacter.generated.h"

UCLASS()
class EWE_API ABaseCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	// Sets default values for this character's properties
	ABaseCharacter();

	UPROPERTY(EditAnywhere, Category = Weapon)
	TArray<UActorComponent*> ArmMeshes;

	UPROPERTY(EditDefaultsOnly, Category = Weapon)
	TSubclassOf<class ASword> SwordMesh;

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	void SpawnSword();

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

};
