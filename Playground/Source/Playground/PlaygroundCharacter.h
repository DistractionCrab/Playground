// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "InputActionValue.h"
#include "PerspectiveManager.h"
#include "Delegates/Delegate.h"
#include "PlaygroundCharacter.generated.h"

UENUM(BlueprintType)
enum class EPlaygroundCharacterState : uint8 {
	IDLE       UMETA(DisplayName = "Idle"),
	WALKING        UMETA(DisplayName = "Walking"),
	RUNNING        UMETA(DisplayName = "Running"),
};

DECLARE_DYNAMIC_DELEGATE_TwoParams(FStateChangeListener, EPlaygroundCharacterState, From, EPlaygroundCharacterState, To);

UCLASS(config=Game)
class APlaygroundCharacter : public ACharacter
{
	GENERATED_BODY()

protected:
	/* Character stats that determine what can and cannot be done. */
	static class State {
	public:
		virtual EPlaygroundCharacterState GetState() { return EPlaygroundCharacterState::IDLE; }
		virtual void Enter(APlaygroundCharacter* mc) {}
		virtual void Exit(APlaygroundCharacter* mc) {}
		virtual State* Step(APlaygroundCharacter* mc, float delta);
		virtual State* AttemptMove(APlaygroundCharacter* mc);
		virtual State* StopMove(APlaygroundCharacter* mc);
		virtual State* RunUpdate(APlaygroundCharacter* mc);
		virtual FVector2D ModifyMovement(FVector2D fv) { return FVector2D::ZeroVector; }
		virtual bool CanControl() { return true; }
		virtual bool CanWalk() { return true; }
	} IDLE;

	/* Walking state which will control the player walking and will. */
	static class WalkingState : public State {
	public:
		virtual EPlaygroundCharacterState GetState() override { return EPlaygroundCharacterState::WALKING; }
		virtual State* StopMove(APlaygroundCharacter* mc) override;
		virtual FVector2D ModifyMovement(FVector2D fv) override { return fv; }
		virtual State* RunUpdate(APlaygroundCharacter* mc) override;
	} WALKING;

	static class RunningState : public WalkingState {
	public:
		virtual EPlaygroundCharacterState GetState() override { return EPlaygroundCharacterState::RUNNING; }
		virtual void Enter(APlaygroundCharacter* mc) override;
	} RUNNING;


private:
	/** Camera boom positioning the camera behind the character */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	class UPerspectiveManager* PerspectiveManager;

	/** Camera boom positioning the camera behind the character */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	class USpringArmComponent* CameraBoom;

	/** Follow camera */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	class UCameraComponent* FollowCamera;
	
	/** MappingContext */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	class UInputMappingContext* DefaultMappingContext;

	/** Jump Input Action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	class UInputAction* JumpAction;

	/** Move Input Action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	class UInputAction* MoveAction;

	/** Move Input Action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	class UInputAction* RunAction;

	/** Look Input Action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	class UInputAction* LookAction;

	bool RunPressed = false;
	State* CurrentState;
	TArray<FStateChangeListener> Listeners;

public:
	APlaygroundCharacter();
	

protected:

	/** Called for run input */
	void RunInput(const FInputActionValue& Value);

	/** Called for run input */
	void StopRunInput(const FInputActionValue& Value);

	/** Called for movement input */
	void Move(const FInputActionValue& Value);

	/** Called for movement input */
	void StopMove(const FInputActionValue& Value);

	/** Called for looking input */
	void Look(const FInputActionValue& Value);

	/** Called when transitioning states. Handles all listener callbacks  **/
	void UpdateState(State* From, State* To);

protected:
	// APawn interface
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;
	
	// To add mapping context
	virtual void BeginPlay();

public:
	/** Returns CameraBoom subobject **/
	FORCEINLINE class USpringArmComponent* GetCameraBoom() const { return CameraBoom; }
	/** Returns FollowCamera subobject **/
	FORCEINLINE class UCameraComponent* GetFollowCamera() const { return FollowCamera; }

	
};

