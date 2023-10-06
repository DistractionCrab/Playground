// Copyright Epic Games, Inc. All Rights Reserved.

#include "PlaygroundCharacter.h"
#include "Engine/LocalPlayer.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/InputComponent.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Controller.h"
#include "GameFramework/SpringArmComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"

typedef PlaygroundCharacterStateMachine Machine;
typedef PlaygroundCharacterStateMachine::PlaygroundCharacterState State;

// -------------------------------- State Machine ------------------------

void Machine::UpdateState(PlaygroundCharacterState* To) {
	int TID = this->TRANSITION.EnterTransition();
	PlaygroundCharacterState* From = this->CurrentState;
	if (From != To) {
		From->Exit();
		auto Check = To->Enter();

		if (Check == To) {
			this->CurrentState = To;
			for (const FStateChangeListener& a : this->Listeners) {
				a.ExecuteIfBound(From->GetState(), To->GetState());
				if (!this->TRANSITION.Valid(TID)) {
					break;
				}
			}				
		}
		else {
			this->UpdateState(Check);
		}
		
	}
}

State* State::AttemptLook() {
	if (this->GetActor()->Controller != nullptr && !GetActor()->GetPerspective()->IsBound())
	{
		// add yaw and pitch input to controller
		this->GetActor()->AddControllerYawInput(this->Owner->LookAxis.X);
		this->GetActor()->AddControllerPitchInput(this->Owner->LookAxis.Y);
	}

	return this;
}

void Machine::AddAction(EPlaygroundCharacterActions Action) {
	if (!this->CurrentActions.Contains(Action)) {
		this->CurrentActions.Add(Action);
		for (const FActionChangeListener& a : this->ActionListeners) {
			a.ExecuteIfBound(Action, true);
		}
	}
	
}

void Machine::RemoveAction(EPlaygroundCharacterActions Action) {
	if (this->CurrentActions.Contains(Action)) {
		this->CurrentActions.Remove(Action);
		for (const FActionChangeListener& a : this->ActionListeners) {
			a.ExecuteIfBound(Action, false);
		}
	}
}

void Machine::ClearActions() {
	for (const EPlaygroundCharacterActions& a: this->CurrentActions) {
		this->RemoveAction(a);
	}
}

void Machine::DeflectionEvent(bool AgainstPlayer) {
	this->CurrentState->DeflectionEvent(AgainstPlayer);
}


//////////////////////////////////////////////////////////////////////////
// APlaygroundCharacter

APlaygroundCharacter::APlaygroundCharacter() : Machine(this)
{
	PrimaryActorTick.bCanEverTick = true;
	// Set size for collision capsule
	GetCapsuleComponent()->InitCapsuleSize(42.f, 96.0f);
		
	// Don't rotate when the controller rotates. Let that just affect the camera.
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;
	

	// Configure character movement
	GetCharacterMovement()->bOrientRotationToMovement = true; // Character moves in the direction of input...	
	GetCharacterMovement()->RotationRate = FRotator(0.0f, 500.0f, 0.0f); // ...at this rotation rate

	// Note: For faster iteration times these variables, and many more, can be tweaked in the Character Blueprint
	// instead of recompiling to adjust them
	GetCharacterMovement()->JumpZVelocity = 700.f;
	GetCharacterMovement()->AirControl = 0.2f;
	GetCharacterMovement()->MaxWalkSpeed = 500.f;
	GetCharacterMovement()->MinAnalogWalkSpeed = 20.f;
	GetCharacterMovement()->BrakingDecelerationWalking = 2000.f;


	// Create a camera boom (pulls in towards the player if there is a collision)
	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->TargetArmLength = 400.0f; // The camera follows at this distance behind the character	
	CameraBoom->bUsePawnControlRotation = true; // Rotate the arm based on the controller

	// Create a follow camera
	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName); // Attach the camera to the end of the boom and let the boom adjust to match the controller orientation
	FollowCamera->bUsePawnControlRotation = false; // Camera does not rotate relative to arm

	// Note: The skeletal mesh and anim blueprint references on the Mesh component (inherited from Character) 
	// are set in the derived blueprint asset named ThirdPersonCharacter (to avoid direct content references in C++)
	this->PerspectiveManager = CreateDefaultSubobject<UPerspectiveManager>(TEXT("Custom Perspective"));
}

void APlaygroundCharacter::BeginPlay()
{
	// Call the base class  
	Super::BeginPlay();

	if (APlayerController* PlayerController = Cast<APlayerController>(Controller))
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PlayerController->GetLocalPlayer()))
		{
			Subsystem->AddMappingContext(DefaultMappingContext, 0);
		}
	}
}

// Called every frame
void APlaygroundCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	this->Machine.Step(DeltaTime);
}


void APlaygroundCharacter::PostEditChangeProperty(struct FPropertyChangedEvent& e) {
	Super::PostEditChangeProperty(e);

	if (e.Property != nullptr) {
		if (e.Property->GetFName() == GET_MEMBER_NAME_CHECKED(APlaygroundCharacter, WalkingSpeed)) {
			this->Machine.WalkingSpeed = this->WalkingSpeed;			
		}
		else if (e.Property->GetFName() == GET_MEMBER_NAME_CHECKED(APlaygroundCharacter, RunningSpeed)) {
			this->Machine.RunningSpeed = this->RunningSpeed;
		}
	}
}

//////////////////////////////////////////////////////////////////////////
// Input

void APlaygroundCharacter::SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent)
{
	// Set up action bindings
	if (UEnhancedInputComponent* EnhancedInputComponent = CastChecked<UEnhancedInputComponent>(PlayerInputComponent)) {
		
		//Jumping
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Started, this, &APlaygroundCharacter::JumpInput);
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Completed, this, &ACharacter::StopJumping);

		//Moving
		EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &APlaygroundCharacter::Move);
		EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Completed, this, &APlaygroundCharacter::StopMove);
		EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Canceled, this, &APlaygroundCharacter::StopMove);

		// Running Init
		EnhancedInputComponent->BindAction(RunAction, ETriggerEvent::Started, this, &APlaygroundCharacter::RunInput);
		EnhancedInputComponent->BindAction(RunAction, ETriggerEvent::Completed, this, &APlaygroundCharacter::StopRunInput);
		EnhancedInputComponent->BindAction(RunAction, ETriggerEvent::Canceled, this, &APlaygroundCharacter::StopRunInput);

		//Looking
		EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Triggered, this, &APlaygroundCharacter::Look);

		// Spell Casting
		EnhancedInputComponent->BindAction(SpellCastAction, ETriggerEvent::Started, this, &APlaygroundCharacter::SpellCastInput);

		// Attacking
		EnhancedInputComponent->BindAction(AttackAction, ETriggerEvent::Started, this, &APlaygroundCharacter::AttackInput);
		EnhancedInputComponent->BindAction(GuardAction, ETriggerEvent::Started, this, &APlaygroundCharacter::GuardInput);
		EnhancedInputComponent->BindAction(GuardAction, ETriggerEvent::Completed, this, &APlaygroundCharacter::GuardRelease);
		EnhancedInputComponent->BindAction(GuardAction, ETriggerEvent::Canceled, this, &APlaygroundCharacter::GuardRelease);
	}

}

void APlaygroundCharacter::JumpInput(const FInputActionValue& Value) {
	this->Machine.AttemptJump();
}

void APlaygroundCharacter::StopRunInput(const FInputActionValue& Value) {
	this->Machine.RunPressed = false;
	this->Machine.RunUpdate();
}

void APlaygroundCharacter::RunInput(const FInputActionValue& Value) {
	this->Machine.RunPressed = true;
	this->Machine.RunUpdate();
}

void APlaygroundCharacter::StopMove(const FInputActionValue& Value) {
	this->Machine.StopMove();
	//this->UpdateState(this->CurrentState->StopMove(this));
}

void APlaygroundCharacter::SpellCastInput(const FInputActionValue& Value)
{
	this->StartCast();
}

void APlaygroundCharacter::AttackInput(const FInputActionValue& Value)
{
	this->StartAttack();
}

void APlaygroundCharacter::GuardInput(const FInputActionValue& Value)
{
	this->Machine.GuardPressed = true;
	this->StartGuard();
}

void APlaygroundCharacter::GuardRelease(const FInputActionValue& Value) {
	this->Machine.GuardPressed = false;
	this->FinishGuard();
}

void APlaygroundCharacter::Move(const FInputActionValue& Value)	{	
	this->Machine.InputAxis = Value.Get<FVector2D>();
	this->Machine.AttemptMove();
}

void APlaygroundCharacter::Look(const FInputActionValue& Value)
{
	// input is a Vector2D
	this->Machine.LookAxis = Value.Get<FVector2D>();
	this->Machine.AttemptLook();
	
}

void APlaygroundCharacter::FinishCast() {
	this->Machine.FinishCast(); 
}

void APlaygroundCharacter::StartCast_Implementation() {
	this->Machine.CastTime = this->GetCastTime();
	this->Machine.AttemptCast();
}

float APlaygroundCharacter::DefineCastTime_Implementation() {
	return DEFAULT_CAST_TIME;
}


void APlaygroundCharacter::StartAttack_Implementation() {
	this->Machine.AttemptAttack();
}

void APlaygroundCharacter::FinishAttack() {
	this->Machine.FinishAttack();
}

void APlaygroundCharacter::StartGuard_Implementation() {
	this->Machine.AttemptGuard();
}

void APlaygroundCharacter::FinishGuard_Implementation() {
	this->Machine.FinishGuard();
}

void APlaygroundCharacter::StartDeflection_Implementation(bool AgainstPlayer) {
	
    this->Machine.DeflectionEvent(AgainstPlayer);
}

// -------------------------- State Machine Idle State Implementation --------------------------

State* Machine::Idle::Step(float DeltaTime) {
	if (GetActor()->GetCharacterMovement()->IsFalling()) {
		return &this->Owner->AIRBORNE;
	}
	else {
		return this;
	}
}

State* Machine::Idle::AttemptMove() {
	if (this->Owner->RunPressed) {
		return &this->Owner->RUNNING;
	}
	else {
		return &this->Owner->WALKING;
	}
	
}

State* Machine::Idle::AttemptJump(){
	return &this->Owner->AIRBORNE;
}

State* Machine::Idle::AttemptAttack() {
	this->Owner->ClearActions();
	this->Owner->AddAction(EPlaygroundCharacterActions::ATTACK);
	return &this->Owner->ATTACKING;
}

State* Machine::Idle::AttemptGuard() {

	this->Owner->ClearActions();
	this->Owner->AddAction(EPlaygroundCharacterActions::SHIELDBLOCK);
	return &this->Owner->ATTACKING;
}

// -------------------------- State Machine Walking State Implementation --------------------------

State* Machine::Walking::AttemptMove() {
	this->ApplyMovement();
	return this;
}
State* Machine::Walking::StopMove() {
	return &this->Owner->IDLE;
}
State* Machine::Walking::RunUpdate() {
	if (this->Owner->RunPressed) {
		return &this->Owner->RUNNING;
	}
	else {
		return this;
	}
}

State* Machine::Walking::AttemptJump(){
	return &this->Owner->AIRBORNE;
}

State* Machine::Walking::AttemptAttack() {
	this->Owner->AddAction(EPlaygroundCharacterActions::ATTACK);
	return &this->Owner->ATTACKING;
}

State* Machine::Walking::AttemptGuard() {
	this->Owner->AddAction(EPlaygroundCharacterActions::SHIELDBLOCK);
	return &this->Owner->ATTACKING;
}

State* Machine::Walking::Enter() {
	if (this->Owner->RunPressed) {
		return &this->Owner->RUNNING;
	}
	else {
		GetActor()->GetCharacterMovement()->MaxWalkSpeed = this->GetWalkingSpeed();
		this->ApplyMovement();
		return this;
	}	
}

void Machine::Walking::ApplyMovement() {
	FRotator Rotation;

	auto Perspective = GetActor()->GetPerspective();
	if (Perspective->IsBound()) {
		Rotation = Perspective->GetPerspective();
	} else {
		Rotation = GetActor()->Controller->GetControlRotation();
	}
	const FRotator YawRotation(0, Rotation.Yaw, 0);

	// get forward vector
	const FVector ForwardDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);

	// get right vector 
	const FVector RightDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

	// add movement 
	GetActor()->AddMovementInput(ForwardDirection, this->Owner->InputAxis.Y);
	GetActor()->AddMovementInput(RightDirection, this->Owner->InputAxis.X);
}

// -------------------------- State Machine Running State Implementation --------------------------

State* Machine::Running::Enter() {
	if (this->Owner->RunPressed) {
		GetActor()->GetCharacterMovement()->MaxWalkSpeed = this->GetWalkingSpeed();
		this->ApplyMovement();
		return this;		
	}
	else {
		return &this->Owner->WALKING;		
	}
}


State* Machine::Running::RunUpdate() {
	if (this->Owner->RunPressed) {
		return this;
	}
	else {
		return &this->Owner->WALKING;
	}	
}

// -------------------------- State Machine Airborne State Implementation --------------------------

State* Machine::Airborne::Step(float DeltaTime) {
	if (GetActor()->GetCharacterMovement()->IsFalling()) {
		return this;
	}
	else {
		return &this->Owner->IDLE;
	}
}
State* Machine::Airborne::Enter() {
	if (!GetActor()->GetCharacterMovement()->IsFalling()) {
		GetActor()->Jump();
	}
	return this;
}

// --------------------------- State Machine Casting State Implementation ------------------------
State* Machine::Casting::Enter() {

	GetActor()->GetCharacterMovement()->MaxWalkSpeed = this->GetWalkingSpeed();
	return this;
}

State* Machine::Casting::AttemptMove() {
	this->Owner->AddAction(EPlaygroundCharacterActions::MOVE);
	this->ApplyMovement();
	return this;
}

State* Machine::Casting::StopMove() {
	this->Owner->RemoveAction(EPlaygroundCharacterActions::MOVE);
	return this;
}

void Machine::Casting::Exit() {
	this->Owner->ClearActions();
}

// ------------------------- State Machine Attacking State Implementation ---------------------

State* Machine::Attacking::AttemptAttack() {
	if (this->ChainCondition()) {
		this->Owner->RemoveAction(EPlaygroundCharacterActions::DEFLECTING);
		this->Owner->RemoveAction(EPlaygroundCharacterActions::SHIELDBLOCK);
		this->Owner->AddAction(EPlaygroundCharacterActions::ATTACK);
		this->CanChain = false;
	}
	return this;
}

State* Machine::Attacking::FinishAttack() {
	this->Owner->ClearActions();
	if (this->Owner->GuardPressed) {
		this->Owner->AddAction(EPlaygroundCharacterActions::SHIELDBLOCK);
		return this;
	} else {
		return &this->Owner->IDLE;	
	}	
}

State* Machine::Attacking::AttemptGuard() {
	if (this->ChainCondition()) {
		this->Owner->RemoveAction(EPlaygroundCharacterActions::DEFLECTING);
		this->Owner->RemoveAction(EPlaygroundCharacterActions::ATTACK);
		this->Owner->AddAction(EPlaygroundCharacterActions::SHIELDBLOCK);
		this->CanChain = false;
		return this;
	}
	return this;
}

State* Machine::Attacking::FinishGuard() {
	if (this->Owner->CurrentActions.Contains(EPlaygroundCharacterActions::SHIELDBLOCK)) {
		this->Owner->ClearActions();
		return &this->Owner->IDLE;
	} else {
		return this;
	}	
}


State* Machine::Attacking::AttemptMove() {
	if (this->Owner->CurrentActions.Contains(EPlaygroundCharacterActions::SHIELDBLOCK)) {
		this->Owner->AddAction(EPlaygroundCharacterActions::MOVE);
		this->ApplyMovement();
	} else if (this->Owner->CurrentActions.IsEmpty()) {
		return &this->Owner->WALKING;
	}
	
	return this;
}

State* Machine::Attacking::StopMove() {
	this->Owner->RemoveAction(EPlaygroundCharacterActions::MOVE);
	return this;
}


bool Machine::Attacking::ChainCondition() {
	if (this->Owner->CurrentActions.Contains(EPlaygroundCharacterActions::SHIELDBLOCK)) {
		return true;
	} else {
		return this->CanChain;
	}
}

State* Machine::Attacking::DeflectionEvent(bool AgainstPlayer) {
	if(GEngine)
		GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Yellow, TEXT("Deflection Setup!!"));
	this->Owner->RemoveAction(EPlaygroundCharacterActions::SHIELDBLOCK);
	this->Owner->RemoveAction(EPlaygroundCharacterActions::ATTACK);
	this->Owner->RemoveAction(EPlaygroundCharacterActions::MOVE);
	this->Owner->AddAction(EPlaygroundCharacterActions::DEFLECTING);
	return this;
}

State* Machine::Attacking::RunUpdate() {
	return this;
}