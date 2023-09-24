// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "InputActionValue.h"
#include "PerspectiveManager.h"
#include "Delegates/Delegate.h"
#include "PlaygroundCharacter.generated.h"

#define DEFAULT_WALK_SPEED 200.0f
#define DEFAULT_RUN_SPEED 500.0f

UENUM(BlueprintType)
enum class EPlaygroundCharacterState : uint8 {
	IDLE           UMETA(DisplayName = "Idle"),
	WALKING        UMETA(DisplayName = "Walking"),
	RUNNING        UMETA(DisplayName = "Running"),
	AIRBORNE       UMETA(DisplayName = "Airborne"),
	SPELLCAST       UMETA(DisplayName = "SpellCasting"),
};

DECLARE_DYNAMIC_DELEGATE_TwoParams(FStateChangeListener, EPlaygroundCharacterState, From, EPlaygroundCharacterState, To);

class PlaygroundCharacterStateMachine {
public:
	bool RunPressed = false;
	float WalkingSpeed = DEFAULT_WALK_SPEED;
	float RunningSpeed = DEFAULT_RUN_SPEED;
	FVector2D InputAxis;
	TArray<FStateChangeListener> Listeners;

	class PlaygroundCharacterState {
	public:
		PlaygroundCharacterStateMachine* Owner;
	public:
		virtual EPlaygroundCharacterState GetState() { return EPlaygroundCharacterState::IDLE; }
		virtual PlaygroundCharacterState* Enter(APlaygroundCharacter* mc) { return this; }
		virtual void Exit(APlaygroundCharacter* mc) {}
		virtual PlaygroundCharacterState* Step(APlaygroundCharacter* mc, float DeltaTime) { return this; }
		virtual PlaygroundCharacterState* AttemptMove(APlaygroundCharacter* mc) { return this; }
		virtual PlaygroundCharacterState* StopMove(APlaygroundCharacter* mc) { return this; }
		virtual PlaygroundCharacterState* RunUpdate(APlaygroundCharacter* mc) { return this; }
		virtual PlaygroundCharacterState* AttemptJump(APlaygroundCharacter* mc) { return this; }
	};

	class Idle: public PlaygroundCharacterState {
		virtual PlaygroundCharacterState* Step(APlaygroundCharacter* mc, float DeltaTime) override;
		virtual PlaygroundCharacterState* AttemptMove(APlaygroundCharacter* mc) override;
		virtual PlaygroundCharacterState* StopMove(APlaygroundCharacter* mc) override;
		virtual PlaygroundCharacterState* AttemptJump(APlaygroundCharacter* mc) override;
	} IDLE;

	class Walking: public PlaygroundCharacterState {
	public:
		virtual PlaygroundCharacterState* Enter(APlaygroundCharacter* mc) override;
		virtual PlaygroundCharacterState* AttemptMove(APlaygroundCharacter* mc) override;
		virtual PlaygroundCharacterState* StopMove(APlaygroundCharacter* mc) override;
		virtual PlaygroundCharacterState* RunUpdate(APlaygroundCharacter* mc) override;
		virtual PlaygroundCharacterState* AttemptJump(APlaygroundCharacter* mc) override;
		virtual EPlaygroundCharacterState GetState() override { return EPlaygroundCharacterState::WALKING; }

		virtual void ApplyMovement(APlaygroundCharacter* mc, FVector2D Input);
	} WALKING;

	class Running: public Walking {
	public:
		virtual PlaygroundCharacterState* Enter(APlaygroundCharacter* mc) override;
		virtual PlaygroundCharacterState* RunUpdate(APlaygroundCharacter* mc) override;
		virtual EPlaygroundCharacterState GetState() override { return EPlaygroundCharacterState::RUNNING; }
	} RUNNING;

	class Airborne : public PlaygroundCharacterState {
		virtual PlaygroundCharacterState* Step(APlaygroundCharacter* mc, float DeltaTime) override;
		virtual PlaygroundCharacterState* Enter(APlaygroundCharacter* mc) override;
		virtual EPlaygroundCharacterState GetState() override { return EPlaygroundCharacterState::AIRBORNE; }
	} AIRBORNE;

	class Casting : public PlaygroundCharacterState {

	} CASTING;

	PlaygroundCharacterState* CurrentState;

public:
	PlaygroundCharacterStateMachine() {
		this->CurrentState = &IDLE;
		this->IDLE.Owner = this;
		this->WALKING.Owner = this;
		this->RUNNING.Owner = this;
		this->AIRBORNE.Owner = this;
	}

	void BeginPlay() {
		for (const FStateChangeListener& a : this->Listeners) {
			a.ExecuteIfBound(IDLE.GetState(), IDLE.GetState());
		}
	}
	void Step(APlaygroundCharacter* mc, float DeltaTime) { this->UpdateState(this->CurrentState->Step(mc, DeltaTime), mc); }
	void AttemptMove(APlaygroundCharacter* mc) { this->UpdateState(this->CurrentState->AttemptMove(mc), mc); }
	void StopMove(APlaygroundCharacter* mc) { this->UpdateState(this->CurrentState->StopMove(mc), mc); }
	void RunUpdate(APlaygroundCharacter* mc) { this->UpdateState(this->CurrentState->RunUpdate(mc), mc); }
	void AttemptJump(APlaygroundCharacter* mc) { this->UpdateState(this->CurrentState->AttemptJump(mc), mc); }

private:
	void UpdateState(PlaygroundCharacterState* To, APlaygroundCharacter* mc) {
		PlaygroundCharacterState* From = this->CurrentState;
		if (From != To) {
			From->Exit(mc);
			auto Check = To->Enter(mc);

			if (Check == To) {
				for (const FStateChangeListener& a : this->Listeners) {
					a.ExecuteIfBound(From->GetState(), To->GetState());
				}
				this->CurrentState = To;
			}
			else {
				this->UpdateState(Check, mc);
			}
			
		}
	}
};



UCLASS(config=Game)
class APlaygroundCharacter : public ACharacter
{
	GENERATED_BODY()

	/* Primary State MAchine for dealing with character state.*/
	PlaygroundCharacterStateMachine Machine;

private:
	/** The scaling factor for how much faster running is than walking. **/
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Character Physics",
		meta = (AllowPrivateAccess = "true"))
	float RunningScale = 1.0;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Character Physics",
		meta = (AllowPrivateAccess = "true"))
	float WalkingSpeed = DEFAULT_WALK_SPEED;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Character Physics",
		meta = (AllowPrivateAccess = "true"))
	float RunningSpeed = DEFAULT_RUN_SPEED;

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

	/** Spell Cast Input Action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	class UInputAction* SpellCastAction;

public:
	APlaygroundCharacter();
	

protected:

	void PostEditChangeProperty(struct FPropertyChangedEvent& e) override;

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

	/** Called for looking input */
	virtual void JumpInput(const FInputActionValue& Value);

	/** Called for looking input */
	virtual void SpellCastInput(const FInputActionValue& Value);

protected:
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// APawn interface
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;
	
	// To add mapping context
	virtual void BeginPlay();

public:
	/** Returns CameraBoom subobject **/
	FORCEINLINE class USpringArmComponent* GetCameraBoom() const { return CameraBoom; }
	/** Returns FollowCamera subobject **/
	FORCEINLINE class UCameraComponent* GetFollowCamera() const { return FollowCamera; }

	UFUNCTION(BlueprintCallable, Category = "PlaygroundCharacter")
	virtual void StateListen(const FStateChangeListener& Del) {
		this->Machine.Listeners.Add(Del);
	}

	FORCEINLINE PlaygroundCharacterStateMachine* GetMachine() { return &this->Machine; }

	FORCEINLINE UPerspectiveManager* GetPerspective() { return this->PerspectiveManager; }
};

