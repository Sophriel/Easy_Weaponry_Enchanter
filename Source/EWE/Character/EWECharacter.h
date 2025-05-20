// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Logging/LogMacros.h"
#include "EasyWeaponryEnchanter/Interface/EWECharacterInterface.h"
#include "EWEAttackAnimationInterface.h"
#include "AbilitySystemInterface.h"
#include "EWECharacter.generated.h"

struct FInputActionValue;

DECLARE_LOG_CATEGORY_EXTERN(LogEWECharacter, Log, All);

UENUM()
enum class ECameraType : uint8
{
	Shoulder,
	Quarter
};

UCLASS(config = Game)
class AEWECharacter :
	public ACharacter,
	public IEWECharacterInterface,
	public IEWEAttackAnimationInterface,
	public IAbilitySystemInterface
{
	GENERATED_BODY()

public:
	AEWECharacter();

protected:
	virtual void PossessedBy(AController* NewController) override;
	virtual void BeginPlay() override;

#pragma region Input

protected:
	virtual void NotifyControllerChanged() override;
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	/** MappingContext */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	TObjectPtr<class UInputMappingContext> DefaultMappingContext;

	/** MappingContext */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	TObjectPtr<class UInputMappingContext> QuickSlotMappingContext;

	/** Called for movement input */
	void Move(const FInputActionValue& Value);

	/** Called for looking input */
	void Look(const FInputActionValue& Value);

	/** Jump Input Action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	TObjectPtr<class UInputAction> JumpAction;

	/** Move Input Action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	TObjectPtr<class UInputAction> MoveAction;

	/** Look Input Action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	TObjectPtr<class UInputAction> LookAction;

	/** Change Camera Input Action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	TObjectPtr<class UInputAction> ChangeCameraAction;

	/** Select Item Input Action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	TArray<TObjectPtr<class UInputAction>> SelectItemActions;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	TObjectPtr<class UInputAction> ScrollItemAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	TObjectPtr<class UInputAction> AttackAction;

#pragma endregion

#pragma region Camera

public:
	/** Returns CameraBoom subobject **/
	FORCEINLINE class USpringArmComponent* GetCameraBoom() const { return CameraBoom; }
	/** Returns FollowCamera subobject **/
	FORCEINLINE class UCameraComponent* GetFollowCamera() const { return FollowCamera; }

protected:
	/** Called for Change Camera input */
	void ChangeCamera(const FInputActionValue& Value);
	void SetCharacterControl(ECameraType CameraType);
	void SetCharacterControlData(const class UEWEControlData* ControlData);

	/** Camera boom positioning the camera behind the character */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	TObjectPtr<class USpringArmComponent> CameraBoom;

	/** Follow camera */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	TObjectPtr<class UCameraComponent> FollowCamera;

	ECameraType CurrentCameraType;

	UPROPERTY(EditAnywhere, Category = Camera, Meta = (AllowPrivateAccess = "true"))
	TMap<ECameraType, class UEWEControlData*> CameraControl;

#pragma endregion

#pragma region Weapon

public:
	virtual class UAbilitySystemComponent* GetAbilitySystemComponent() const override;

	void EquipWeapon(class AEWEWeaponBase* Weapon);

protected:
	void SelectItem(const uint8 SlotIndex);
	void ScrollItem(const FInputActionValue& Value);

	UFUNCTION(BlueprintImplementableEvent, Category = Weapon, Meta = (DisplayName = "OnAttack"))
	void K2_Attack();

	void		 Attack(const FInputActionValue& Value);
	virtual void AttackHitCheck() override;
	virtual void AttackReleaseCheck() override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Weapon, meta = (AllowPrivateAccess = "true"))
	TObjectPtr<class UEWEInventoryComponent> InventoryComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Weapon, meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USkeletalMeshComponent> WeaponMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Weapon, meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USkeletalMeshComponent> SubWeaponMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Weapon, meta = (AllowPrivateAccess = "true"))
	TObjectPtr<const class AEWEWeaponBase> CurrentWeapon;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Weapon, meta = (AllowPrivateAccess = "true"))
	TObjectPtr<class UAbilitySystemComponent> WeaponAbilityComponent;

#pragma endregion
};
