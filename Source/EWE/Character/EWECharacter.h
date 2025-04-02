// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Logging/LogMacros.h"
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
class AEWECharacter : public ACharacter
{
	GENERATED_BODY()

	/** Camera boom positioning the camera behind the character */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	TObjectPtr<class USpringArmComponent> CameraBoom;

	/** Follow camera */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	TObjectPtr<class UCameraComponent> FollowCamera;

	/** MappingContext */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	TObjectPtr<class UInputMappingContext> DefaultMappingContext;

	/** MappingContext */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	TObjectPtr<class UInputMappingContext> QuickSlotMappingContext;

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

public:
	AEWECharacter();

protected:
	/** Called for movement input */
	void Move(const FInputActionValue& Value);

	/** Called for looking input */
	void Look(const FInputActionValue& Value);

	/** Called for Change Camera input */
	void ChangeCamera(const FInputActionValue& Value);
	void SetCharacterControl(ECameraType CameraType);
	void SetCharacterControlData(const class UEWEControlData* ControlData);

	ECameraType CurrentCameraType;

	UPROPERTY(EditAnywhere, Category = Camera, Meta = (AllowPrivateAccess = "true"))
	TMap<ECameraType, class UEWEControlData*> CameraControl;

protected:
	void SelectItem(const FInputActionValue& Value, const uint8 SlotIndex);
	void ScrollItem(const FInputActionValue& Value);

protected:
	virtual void NotifyControllerChanged() override;

	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

public:
	/** Returns CameraBoom subobject **/
	FORCEINLINE class USpringArmComponent* GetCameraBoom() const { return CameraBoom; }
	/** Returns FollowCamera subobject **/
	FORCEINLINE class UCameraComponent* GetFollowCamera() const { return FollowCamera; }
};
