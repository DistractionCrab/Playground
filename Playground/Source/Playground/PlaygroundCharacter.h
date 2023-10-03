// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "InputActionValue.h"
#include "PerspectiveManager.h"
#include "PlaygroundStatics.h"
#include "Delegates/Delegate.h"
#include "PlaygroundCharacter.generated.h"

const float DEFAULT_WALK_SPEED = 200.0f;
const float DEFAULT_CAST_WALK_SPEED = 50.0f;
const float DEFAULT_RUN_SPEED = 500.0f;
const float DEFAULT_CAST_TIME = 2.0f;

/**
 * Enum in charge of communicating the exact state the character is in.
 */
UENUM(BlueprintType)
enum class EPlaygroundCharacterState : uint8 {
	IDLE           UMETA(DisplayName = "Idle"),
	WALKING        UMETA(DisplayName = "Walking"),
	RUNNING        UMETA(DisplayName = "Running"),
	AIRBORNE       UMETA(DisplayName = "Airborne"),
	SPELLCAST       UMETA(DisplayName = "SpellCasting"),
};

/**
 * Enum defining what actions exist. These enumerate all possible actions regardless of state,
 * and only states themselves can define what action can take place in each state. 
 */
UENUM(BlueprintType)
enum class EPlaygroundCharacterActions : uint8 {
	MOVE               UMETA(DisplayName = "Moving"),
	LANTERNHOLD        UMETA(DisplayName = "Walking"),
	SHIELDBLOCK        UMETA(DisplayName = "Shield Block"),
	ATTACK             UMETA(DisplayName = "Attack"),
	LOOKAT             UMETA(DisplayName = "Look At"),
	NOSPELL            UMETA(DisplayName = "No Spell"), // Used when attempting to cast a spell, but cannot.
};

DECLARE_DYNAMIC_DELEGATE_TwoParams(FStateChangeListener, EPlaygroundCharacterState, From, EPlaygroundCharacterState, To);
DECLARE_DYNAMIC_DELEGATE_TwoParams(FActionChangeListener, EPlaygroundCharacterActions, Actions, bool, AddedQ);


class PlaygroundCharacterStateMachine {
public:
	bool RunPressed = false;
	float WalkingSpeed = DEFAULT_WALK_SPEED;
	float RunningSpeed = DEFAULT_RUN_SPEED;
	float CastWalkSpeed = DEFAULT_CAST_WALK_SPEED;
	float CastTime = DEFAULT_CAST_TIME;
	FVector2D InputAxis;
	FVector2D LookAxis;
	TSet<EPlaygroundCharacterActions> CurrentActions;
	TArray<FStateChangeListener> Listeners;
	TArray<FActionChangeListener> ActionListeners;

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
		virtual PlaygroundCharacterState* AttemptCast(APlaygroundCharacter* mc) { return this; }
		virtual PlaygroundCharacterState* FinishCast(APlaygroundCharacter* mc) { return this; }
		virtual PlaygroundCharacterState* AttemptLook(APlaygroundCharacter* mc);
	};

	class Idle: public PlaygroundCharacterState {
		virtual PlaygroundCharacterState* Step(APlaygroundCharacter* mc, float DeltaTime) override;
		virtual PlaygroundCharacterState* AttemptMove(APlaygroundCharacter* mc) override;
		virtual PlaygroundCharacterState* AttemptJump(APlaygroundCharacter* mc) override;
		virtual PlaygroundCharacterState* AttemptCast(APlaygroundCharacter* mc) override { return &this->Owner->CASTING; }
	} IDLE;

	class Walking: public PlaygroundCharacterState {
	public:
		virtual PlaygroundCharacterState* Enter(APlaygroundCharacter* mc) override;
		virtual PlaygroundCharacterState* AttemptMove(APlaygroundCharacter* mc) override;
		virtual PlaygroundCharacterState* StopMove(APlaygroundCharacter* mc) override;
		virtual PlaygroundCharacterState* RunUpdate(APlaygroundCharacter* mc) override;
		virtual PlaygroundCharacterState* AttemptJump(APlaygroundCharacter* mc) override;
		virtual PlaygroundCharacterState* AttemptCast(APlaygroundCharacter* mc) override { return &this->Owner->CASTING; }
		virtual EPlaygroundCharacterState GetState() override { return EPlaygroundCharacterState::WALKING; }
		

		virtual void ApplyMovement(APlaygroundCharacter* mc, FVector2D Input);
		virtual float GetWalkingSpeed() { return this->Owner->WalkingSpeed; }
	} WALKING;

	class Running: public Walking {
	public:
		virtual PlaygroundCharacterState* Enter(APlaygroundCharacter* mc) override;
		virtual PlaygroundCharacterState* RunUpdate(APlaygroundCharacter* mc) override;
		virtual EPlaygroundCharacterState GetState() override { return EPlaygroundCharacterState::RUNNING; }
		virtual float GetWalkingSpeed() override { return this->Owner->RunningSpeed; }
	} RUNNING;

	class Airborne : public PlaygroundCharacterState {
		virtual PlaygroundCharacterState* Step(APlaygroundCharacter* mc, float DeltaTime) override;
		virtual PlaygroundCharacterState* Enter(APlaygroundCharacter* mc) override;
		virtual EPlaygroundCharacterState GetState() override { return EPlaygroundCharacterState::AIRBORNE; }
	} AIRBORNE;

	class Casting : public Walking {
	public:
		virtual EPlaygroundCharacterState GetState() override { return EPlaygroundCharacterState::SPELLCAST; }

		virtual PlaygroundCharacterState* AttemptMove(APlaygroundCharacter* mc) override;
		virtual PlaygroundCharacterState* StopMove(APlaygroundCharacter* mc) override;
		virtual PlaygroundCharacterState* FinishCast(APlaygroundCharacter* mc) override { return &this->Owner->IDLE; }		
		virtual PlaygroundCharacterState* RunUpdate(APlaygroundCharacter* mc) override { return this; }
		virtual PlaygroundCharacterState* AttemptJump(APlaygroundCharacter* mc) override { return this; }
		virtual PlaygroundCharacterState* Enter(APlaygroundCharacter* mc) override;
		virtual void Exit(APlaygroundCharacter* mc) override;

		virtual float GetWalkingSpeed() override { return this->Owner->CastWalkSpeed; }
	} CASTING;

	PlaygroundCharacterState* CurrentState;

private:
	struct Transition {
	private:
		int ID = 0;

	public:
		int EnterTransition() {
			this->ID += 1;
			return ID;
		}

		bool Valid(int OID) {
			return OID == this->ID;
		}
	} TRANSITION;

public:
	PlaygroundCharacterStateMachine() {
		this->CurrentState = &IDLE;
		this->IDLE.Owner = this;
		this->WALKING.Owner = this;
		this->RUNNING.Owner = this;
		this->AIRBORNE.Owner = this;
		this->CASTING.Owner = this;
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
	void AttemptCast(APlaygroundCharacter* mc) { this->UpdateState(this->CurrentState->AttemptCast(mc), mc); }
	void FinishCast(APlaygroundCharacter* mc) { this->UpdateState(this->CurrentState->FinishCast(mc), mc); }
	void AttemptLook(APlaygroundCharacter* mc) { this->UpdateState(this->CurrentState->AttemptLook(mc), mc); }

	void ForceState(EPlaygroundCharacterState e, APlaygroundCharacter* mc) {
		switch (e) {
			case EPlaygroundCharacterState::IDLE: this->UpdateState(&this->IDLE, mc); break;
			case EPlaygroundCharacterState::WALKING: this->UpdateState(&this->WALKING, mc); break;
			case EPlaygroundCharacterState::RUNNING: this->UpdateState(&this->RUNNING, mc); break;
			case EPlaygroundCharacterState::AIRBORNE: this->UpdateState(&this->AIRBORNE, mc); break;
			case EPlaygroundCharacterState::SPELLCAST: this->UpdateState(&this->CASTING, mc); break;
		}
	}

private:
	void UpdateState(PlaygroundCharacterState* To, APlaygroundCharacter* mc);
	void AddAction(EPlaygroundCharacterActions Action, APlaygroundCharacter* mc);
	void RemoveAction(EPlaygroundCharacterActions Action, APlaygroundCharacter* mc);
	void ClearActions(APlaygroundCharacter* mc);
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

	UFUNCTION(BlueprintCallable, Category = "PlaygroundCharacter")
	virtual void ActionListen(const FActionChangeListener& Del) {
		this->Machine.ActionListeners.Add(Del);
	}

	UFUNCTION(BlueprintCallable, Category = "PlaygroundCharacter")
	virtual void FinishCast();

	UFUNCTION(BlueprintCallable, Category = "PlaygroundCharacter")
	void ForceState(EPlaygroundCharacterState e) { this->Machine.ForceState(e, this); }

	UFUNCTION(BlueprintNativeEvent, Category = "PlaygroundCharacter")
	void StartCast();
	virtual void StartCast_Implementation();

	UFUNCTION(BlueprintPure, Category = "PlaygroundCharacter")
	float GetCastTime() { return this->DefineCastTime(); }

	UFUNCTION(BlueprintNativeEvent, Category = "PlaygroundCharacter")
	float DefineCastTime();
	virtual float DefineCastTime_Implementation();


	FORCEINLINE PlaygroundCharacterStateMachine* GetMachine() { return &this->Machine; }
	FORCEINLINE UPerspectiveManager* GetPerspective() { return this->PerspectiveManager; }
};

