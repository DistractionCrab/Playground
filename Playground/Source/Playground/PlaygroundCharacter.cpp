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

void Machine::UpdateState(PlaygroundCharacterState* To, APlaygroundCharacter* mc) {
	int TID = this->TRANSITION.EnterTransition();
	PlaygroundCharacterState* From = this->CurrentState;
	if (From != To) {
		From->Exit(mc);
		auto Check = To->Enter(mc);

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
			this->UpdateState(Check, mc);
		}
		
	}
}


//////////////////////////////////////////////////////////////////////////
// APlaygroundCharacter

APlaygroundCharacter::APlaygroundCharacter()
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
	this->Machine.Step(this, DeltaTime);
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
	}

}

void APlaygroundCharacter::JumpInput(const FInputActionValue& Value) {
	this->Machine.AttemptJump(this);
}

void APlaygroundCharacter::StopRunInput(const FInputActionValue& Value) {
	this->Machine.RunPressed = false;
	this->Machine.RunUpdate(this);
}

void APlaygroundCharacter::RunInput(const FInputActionValue& Value) {
	this->Machine.RunPressed = true;
	this->Machine.RunUpdate(this);
}

void APlaygroundCharacter::StopMove(const FInputActionValue& Value) {
	this->Machine.StopMove(this);
	//this->UpdateState(this->CurrentState->StopMove(this));
}

void APlaygroundCharacter::SpellCastInput(const FInputActionValue& Value)
{
	this->StartCast();
}
void APlaygroundCharacter::Move(const FInputActionValue& Value)	{	
	this->Machine.InputAxis = Value.Get<FVector2D>();
	this->Machine.AttemptMove(this);
}

void APlaygroundCharacter::Look(const FInputActionValue& Value)
{
	// input is a Vector2D
	FVector2D LookAxisVector = Value.Get<FVector2D>();

	if (Controller != nullptr && !this->PerspectiveManager->IsBound())
	{
		// add yaw and pitch input to controller
		AddControllerYawInput(LookAxisVector.X);
		AddControllerPitchInput(LookAxisVector.Y);
	}
}

void APlaygroundCharacter::FinishCast() {
	this->Machine.FinishCast(this); 
}

void APlaygroundCharacter::StartCast_Implementation() {
	this->Machine.CastTime = this->GetCastTime();
	this->Machine.AttemptCast(this);
}

float APlaygroundCharacter::DefineCastTime_Implementation() {
	return DEFAULT_CAST_TIME;
}

// -------------------------- State Machine Idle State Implementation --------------------------

State* Machine::Idle::Step(APlaygroundCharacter* mc, float DeltaTime) {
	if (mc->GetCharacterMovement()->IsFalling()) {
		return &this->Owner->AIRBORNE;
	}
	else {
		return this;
	}
}

State* Machine::Idle::AttemptMove(APlaygroundCharacter* mc) {
	if (this->Owner->RunPressed) {
		return &this->Owner->RUNNING;
	}
	else {
		return &this->Owner->WALKING;
	}
	
}

State* Machine::Idle::AttemptJump(APlaygroundCharacter* mc){
	return &this->Owner->AIRBORNE;
}

// -------------------------- State Machine Walking State Implementation --------------------------

State* Machine::Walking::AttemptMove(APlaygroundCharacter* mc) {
	this->ApplyMovement(mc, this->Owner->InputAxis);
	return this;
}
State* Machine::Walking::StopMove(APlaygroundCharacter* mc) {
	return &this->Owner->IDLE;
}
State* Machine::Walking::RunUpdate(APlaygroundCharacter* mc) {
	if (this->Owner->RunPressed) {
		return &this->Owner->RUNNING;
	}
	else {
		return this;
	}
}
State* Machine::Walking::AttemptJump(APlaygroundCharacter* mc){
	return &this->Owner->AIRBORNE;
}

State* Machine::Walking::Enter(APlaygroundCharacter* mc) {
	if (this->Owner->RunPressed) {
		return &this->Owner->RUNNING;
	}
	else {
		mc->GetCharacterMovement()->MaxWalkSpeed = this->GetWalkingSpeed();
		this->ApplyMovement(mc, this->Owner->InputAxis);
		return this;
	}	
}

void Machine::Walking::ApplyMovement(APlaygroundCharacter* mc, FVector2D Input) {
	FRotator Rotation;

	auto Perspective = mc->GetPerspective();
	if (Perspective->IsBound()) {
		Rotation = Perspective->GetPerspective();
	} else {
		Rotation = mc->Controller->GetControlRotation();
	}
	const FRotator YawRotation(0, Rotation.Yaw, 0);

	// get forward vector
	const FVector ForwardDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);

	// get right vector 
	const FVector RightDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

	// add movement 
	mc->AddMovementInput(ForwardDirection, Input.Y);
	mc->AddMovementInput(RightDirection, Input.X);
}

// -------------------------- State Machine Running State Implementation --------------------------

State* Machine::Running::Enter(APlaygroundCharacter* mc) {
	if (this->Owner->RunPressed) {
		mc->GetCharacterMovement()->MaxWalkSpeed = this->GetWalkingSpeed();
		this->ApplyMovement(mc, this->Owner->InputAxis);
		return this;		
	}
	else {
		return &this->Owner->WALKING;		
	}
}


State* Machine::Running::RunUpdate(APlaygroundCharacter* mc) {
	if (this->Owner->RunPressed) {
		return this;
	}
	else {
		return &this->Owner->WALKING;
	}	
}

// -------------------------- State Machine Airborne State Implementation --------------------------

State* Machine::Airborne::Step(APlaygroundCharacter* mc, float DeltaTime) {
	if (mc->GetCharacterMovement()->IsFalling()) {
		return this;
	}
	else {
		return &this->Owner->IDLE;
	}
}
State* Machine::Airborne::Enter(APlaygroundCharacter* mc) {
	if (!mc->GetCharacterMovement()->IsFalling()) {
		mc->Jump();
	}
	return this;
}

// --------------------------- State Machine Casting State Implementation ------------------------
State* Machine::Casting::Enter(APlaygroundCharacter* mc) {
	mc->GetCharacterMovement()->MaxWalkSpeed = this->GetWalkingSpeed();
	return this;
}