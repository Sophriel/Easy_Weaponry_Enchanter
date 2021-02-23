// Fill out your copyright notice in the Description page of Project Settings.


#include "Sword.h"
#include "Engine.h"

ASword::ASword()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	SwordMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("SwordMesh"));
	RootComponent = SwordMesh;

	//  런타임에서
	//static USkeletalMesh* mesh = Cast<USkeletalMesh>(StaticLoadObject(UObject::StaticClass(), NULL, TEXT("SkeletalMesh'/Game/InfinityBladeWeapons/Weapons/Blade/Swords/Blade_SwordA/SK_Blade_SwordA.SK_Blade_SwordA'")));
	//SwordMesh->SetSkeletalMesh(mesh);
	
	//  생성단에서
	static ConstructorHelpers::FObjectFinder<USkeletalMesh> SK_Blade_SwordA(TEXT("SkeletalMesh'/Game/InfinityBladeWeapons/Weapons/Blade/Swords/Blade_SwordA/SK_Blade_SwordA.SK_Blade_SwordA'"));
	SwordMesh->SetSkeletalMesh(SK_Blade_SwordA.Object);
}

// Called when the game starts or when spawned
void ASword::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void ASword::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

