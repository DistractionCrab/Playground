// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "InputActionValue.h"
#include "Components/PerspectiveManager.h"
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
	SPELLCAST      UMETA(DisplayName = "SpellCasting"),
	ATTACKING      UMETA(DisplayName = "Attacking"),
	DEFLECTING     UMETA(DisplayName = "Deflected"),
	GUARDING       UMETA(DisplayName = "Guarding"),
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
	DEFLECTING         UMETA(DisplayName = "Deflecting"),
	NOSPELL            UMETA(DisplayName = "No Spell"), // Used when attempting to cast a spell, but cannot.
};

DECLARE_DYNAMIC_DELEGATE_TwoParams(FStateChangeListener, EPlaygroundCharacterState, From, EPlaygroundCharacterState, To);
DECLARE_DYNAMIC_DELEGATE_TwoParams(FActionChangeListener, EPlaygroundCharacterActions, Actions, bool, AddedQ);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FAttackEventListener);


class PlaygroundCharacterStateMachine {
private:
	APlaygroundCharacter* Actor;

public:
	bool RunPressed = false;
	bool GuardPressed = false;
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
		// Reference to owner state machine.
		PlaygroundCharacterStateMachine* Owner;

		virtual EPlaygroundCharacterState GetState() { return EPlaygroundCharacterState::IDLE; }
		virtual PlaygroundCharacterState* Enter() { return this; }
		virtual void Exit() {}
		virtual PlaygroundCharacterState* Step(float DeltaTime) { return this; }

		// General Motion
		virtual PlaygroundCharacterState* AttemptMove() { return this; }
		virtual PlaygroundCharacterState* StopMove() { return this; }
		virtual PlaygroundCharacterState* RunUpdate() { return this; }
		virtual PlaygroundCharacterState* AttemptJump() { return this; }
		virtual PlaygroundCharacterState* AttemptLook();
		// Spell Casting
		virtual PlaygroundCharacterState* AttemptCast() { return this; }
		virtual PlaygroundCharacterState* FinishCast() { return this; }		
		// Attacking
		virtual PlaygroundCharacterState* AttemptAttack() { return this; }
		virtual PlaygroundCharacterState* FinishAttack() { return this; }
		// Deflection
		virtual PlaygroundCharacterState* AttemptGuard() { return this; }
		virtual PlaygroundCharacterState* FinishGuard() { return this; }
		virtual void AttackRecovery() {}
		virtual PlaygroundCharacterState* DeflectionEvent(bool AgainstPlayer) { return this; }

		FORCEINLINE APlaygroundCharacter* GetActor() { return this->Owner->Actor; }
		FORCEINLINE PlaygroundCharacterStateMachine* GetOwner() { return this->Owner; }
	};

	

	class Idle: public PlaygroundCharacterState {
		virtual PlaygroundCharacterState* Step(float DeltaTime) override;
		virtual PlaygroundCharacterState* AttemptMove() override;
		virtual PlaygroundCharacterState* AttemptJump() override;
		virtual PlaygroundCharacterState* AttemptCast() override { return &this->Owner->CASTING; }
		virtual PlaygroundCharacterState* AttemptAttack() override;
		virtual PlaygroundCharacterState* AttemptGuard() override;
	} IDLE;

	class Walking: public PlaygroundCharacterState {
	public:
		virtual PlaygroundCharacterState* Enter() override;
		virtual PlaygroundCharacterState* AttemptMove() override;
		virtual PlaygroundCharacterState* StopMove() override;
		virtual PlaygroundCharacterState* RunUpdate() override;
		virtual PlaygroundCharacterState* AttemptJump() override;
		virtual PlaygroundCharacterState* AttemptCast() override { return &this->Owner->CASTING; }
		virtual PlaygroundCharacterState* AttemptAttack() override;
		virtual PlaygroundCharacterState* AttemptGuard() override;


		virtual EPlaygroundCharacterState GetState() override { return EPlaygroundCharacterState::WALKING; }
		

		virtual void ApplyMovement();
		virtual float GetWalkingSpeed() { return this->Owner->WalkingSpeed; }
	} WALKING;

	class Running: public Walking {
	public:
		virtual PlaygroundCharacterState* Enter() override;
		virtual PlaygroundCharacterState* RunUpdate() override;
		virtual EPlaygroundCharacterState GetState() override { return EPlaygroundCharacterState::RUNNING; }
		virtual float GetWalkingSpeed() override { return this->Owner->RunningSpeed; }
	} RUNNING;

	class Airborne : public PlaygroundCharacterState {
		virtual PlaygroundCharacterState* Step(float DeltaTime) override;
		virtual PlaygroundCharacterState* Enter() override;
		virtual EPlaygroundCharacterState GetState() override { return EPlaygroundCharacterState::AIRBORNE; }
	} AIRBORNE;

	class Casting : public Walking {
	public:
		virtual EPlaygroundCharacterState GetState() override { return EPlaygroundCharacterState::SPELLCAST; }

		virtual PlaygroundCharacterState* AttemptMove() override;
		virtual PlaygroundCharacterState* StopMove() override;
		virtual PlaygroundCharacterState* FinishCast() override { return &this->Owner->IDLE; }		
		virtual PlaygroundCharacterState* RunUpdate() override { return this; }
		virtual PlaygroundCharacterState* AttemptJump() override { return this; }
		virtual PlaygroundCharacterState* Enter() override;
		virtual void Exit() override;

		virtual float GetWalkingSpeed() override { return this->Owner->CastWalkSpeed; }
	} CASTING;

	/* ------------------------------------ Combat Oriented States -------------------------------------- */

	class Attacking : public Walking {
		bool CanChain = false;
		virtual EPlaygroundCharacterState GetState() override { return EPlaygroundCharacterState::ATTACKING; }
		virtual PlaygroundCharacterState* Enter() override;
		virtual PlaygroundCharacterState* RunUpdate() override;
		virtual PlaygroundCharacterState* AttemptMove() override;
		virtual PlaygroundCharacterState* StopMove() override;
		virtual PlaygroundCharacterState* AttemptAttack() override;
		virtual PlaygroundCharacterState* FinishAttack() override;
		// Deflection
		virtual PlaygroundCharacterState* AttemptGuard() override;
		virtual PlaygroundCharacterState* FinishGuard() override;
		virtual void AttackRecovery() override;
		virtual PlaygroundCharacterState* DeflectionEvent(bool AgainstPlayer) override;
		virtual float GetWalkingSpeed() override;

		virtual bool ChainCondition();
	} ATTACKING;


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
	PlaygroundCharacterStateMachine(APlaygroundCharacter* Parent): Actor(Parent) {
		this->CurrentState = &IDLE;
		this->IDLE.Owner = this;
		this->WALKING.Owner = this;
		this->RUNNING.Owner = this;
		this->AIRBORNE.Owner = this;
		this->CASTING.Owner = this;
		this->ATTACKING.Owner = this;
	}

	PlaygroundCharacterStateMachine() {
		this->CurrentState = &IDLE;
		this->IDLE.Owner = this;
		this->WALKING.Owner = this;
		this->RUNNING.Owner = this;
		this->AIRBORNE.Owner = this;
		this->CASTING.Owner = this;
		this->ATTACKING.Owner = this;
	}

	void BeginPlay() {
		for (const FStateChangeListener& a : this->Listeners) {
			a.ExecuteIfBound(IDLE.GetState(), IDLE.GetState());
		}
	}

	void Step(float DeltaTime) { this->UpdateState(this->CurrentState->Step(DeltaTime)); }
	void AttemptMove() { this->UpdateState(this->CurrentState->AttemptMove()); }
	void StopMove() { this->UpdateState(this->CurrentState->StopMove()); }
	void RunUpdate() { this->UpdateState(this->CurrentState->RunUpdate()); }
	void AttemptJump() { this->UpdateState(this->CurrentState->AttemptJump()); }
	void AttemptCast() { this->UpdateState(this->CurrentState->AttemptCast()); }
	void FinishCast() { this->UpdateState(this->CurrentState->FinishCast()); }
	void AttemptAttack() { this->UpdateState(this->CurrentState->AttemptAttack()); }
	void FinishAttack() { this->UpdateState(this->CurrentState->FinishAttack()); }
	void AttemptGuard() { this->UpdateState(this->CurrentState->AttemptGuard()); }
	void FinishGuard() { this->UpdateState(this->CurrentState->FinishGuard()); }
	void AttemptLook() { this->UpdateState(this->CurrentState->AttemptLook()); }
	void DeflectionEvent(bool AgainstPlayer);
	void AttackRecovery() { this->CurrentState->AttackRecovery(); }
	void ConsumeAttack() { this->RemoveAction(EPlaygroundCharacterActions::ATTACK); }

	void ForceState(EPlaygroundCharacterState e) {
		switch (e) {
			case EPlaygroundCharacterState::IDLE: this->UpdateState(&this->IDLE); break;
			case EPlaygroundCharacterState::WALKING: this->UpdateState(&this->WALKING); break;
			case EPlaygroundCharacterState::RUNNING: this->UpdateState(&this->RUNNING); break;
			case EPlaygroundCharacterState::AIRBORNE: this->UpdateState(&this->AIRBORNE); break;
			case EPlaygroundCharacterState::SPELLCAST: this->UpdateState(&this->CASTING); break;
		}
	}

private:
	void UpdateState(PlaygroundCharacterState* To);
	void AddAction(EPlaygroundCharacterActions Action);
	void RemoveAction(EPlaygroundCharacterActions Action);
	void ClearActions();
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

	UPROPERTY(BlueprintAssignable, Category = "PlaygroundCharacter", meta = (AllowPrivateAccess = "true"))
	FAttackEventListener AttackEvent;

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

	/** Spell Cast Input Action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	class UInputAction* AttackAction;

	/** Spell Cast Input Action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	class UInputAction* GuardAction;

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

	/** Called for spell cast input */
	virtual void SpellCastInput(const FInputActionValue& Value);

	/** Called for attack input */
	virtual void AttackInput(const FInputActionValue& Value);

	/** Called for attack input */
	virtual void GuardInput(const FInputActionValue& Value);
	virtual void GuardRelease(const FInputActionValue& Value);

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
	void ForceState(EPlaygroundCharacterState e) { this->Machine.ForceState(e); }


	// ----------------------Functions for Spell Casting  -----------------------------
	UFUNCTION(BlueprintNativeEvent, Category = "PlaygroundCharacter")
	void StartCast();
	virtual void StartCast_Implementation();
	UFUNCTION(BlueprintCallable, Category = "PlaygroundCharacter")
	virtual void FinishCast();

	UFUNCTION(BlueprintPure, Category = "PlaygroundCharacter")
	float GetCastTime() { return this->DefineCastTime(); }

	UFUNCTION(BlueprintNativeEvent, Category = "PlaygroundCharacter")
	float DefineCastTime();
	virtual float DefineCastTime_Implementation();


	// ----------------------Functions for Attacking  -----------------------------
	UFUNCTION(BlueprintNativeEvent, Category = "PlaygroundCharacter")
	void StartAttack();
	virtual void StartAttack_Implementation();
	UFUNCTION(BlueprintCallable, Category = "PlaygroundCharacter")
	virtual void FinishAttack();

	UFUNCTION(BlueprintNativeEvent, Category = "PlaygroundCharacter")
	void StartGuard();
	virtual void StartGuard_Implementation();
	UFUNCTION(BlueprintNativeEvent, Category = "PlaygroundCharacter")
	void FinishGuard();
	virtual void FinishGuard_Implementation();

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "PlaygroundCharacter")
	void StartDeflection(bool AgainstPlayer);
	virtual void StartDeflection_Implementation(bool AgainstPlayer);
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "PlaygroundCharacter")
	void AttackRecovery();
	virtual void AttackRecovery_Implementation();
	UFUNCTION(BlueprintCallable, Category = "PlaygroundCharacter")
	virtual void ConsumeAttack();

	FORCEINLINE PlaygroundCharacterStateMachine* GetMachine() { return &this->Machine; }
	FORCEINLINE UPerspectiveManager* GetPerspective() { return this->PerspectiveManager; }
};

