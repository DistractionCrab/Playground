// Copyright Epic Games, Inc. All Rights Reserved.

#include "PlaygroundCharacter.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/InputComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Controller.h"
#include "GameFramework/SpringArmComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"


// Singleton object declarations for linkage.
APlaygroundCharacter::State APlaygroundCharacter::IDLE;
APlaygroundCharacter::WalkingState APlaygroundCharacter::WALKING;
APlaygroundCharacter::RunningState APlaygroundCharacter::RUNNING;
APlaygroundCharacter::AirborneState APlaygroundCharacter::AIRBORNE;


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
	GetCharacterMovement()->AirControl = 0.01f;
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

	this->CurrentState = &IDLE;
	//Add Input Mapping Context
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
	if (UCharacterMovementComponent* PlayerController = Cast<UCharacterMovementComponent>(this->GetMovementComponent())) {
		if (PlayerController->IsFalling()) {
			this->UpdateState(&AIRBORNE);
		}
		else {
			this->UpdateState(this->CurrentState->Step(this, DeltaTime));
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

	}

}

APlaygroundCharacter::State* APlaygroundCharacter::State::RunUpdate(APlaygroundCharacter* mc) {
	return this;
}

APlaygroundCharacter::State* APlaygroundCharacter::WalkingState::RunUpdate(APlaygroundCharacter* mc) {
	if (mc->RunPressed) {
		return &RUNNING;
	}
	else {
		return this;
	}
	
}

void APlaygroundCharacter::JumpInput(const FInputActionValue& Value) {
	ACharacter::Jump();

	if (this->CurrentState->CanControl()) {
		this->UpdateState(&AIRBORNE);
	}
}

void APlaygroundCharacter::StopRunInput(const FInputActionValue& Value) {
	this->RunPressed = false;
	State* To = this->CurrentState->RunUpdate(this);
	this->UpdateState(To);
}

void APlaygroundCharacter::RunInput(const FInputActionValue& Value) {
	this->RunPressed = true;
	State* To = this->CurrentState->RunUpdate(this);
	this->UpdateState(To);
}

void APlaygroundCharacter::StopMove(const FInputActionValue& Value) {
	this->UpdateState(this->CurrentState->StopMove(this));
}

void APlaygroundCharacter::Move(const FInputActionValue& Value)
{
	
	
	// input is a Vector2D
	FVector2D MovementVector = this->CurrentState->ModifyMovement(this, Value.Get<FVector2D>());	

	bool c1 = this->CurrentState->CanControl();
	bool c2 = this->CurrentState->CanWalk();
	if (c1 && c2 && Controller != nullptr)
	{
		State* To = this->CurrentState->AttemptMove(this);
		this->UpdateState(To);


		const FRotator Rotation = this->PerspectiveManager->IsBound() ?
			this->PerspectiveManager->GetPerspective()
			: this->Controller->GetControlRotation();
		const FRotator YawRotation(0, Rotation.Yaw, 0);

		// get forward vector
		const FVector ForwardDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
	
		// get right vector 
		const FVector RightDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

		// add movement 
		AddMovementInput(ForwardDirection, MovementVector.Y);
		AddMovementInput(RightDirection, MovementVector.X);
	}
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


void APlaygroundCharacter::UpdateState(State* To) {
	State* From = this->CurrentState;
	if (From != To) {		
		this->CurrentState = To;
		From->Exit(this);
		To->Enter(this);

		for (const FStateChangeListener& a : this->Listeners) {
			a.ExecuteIfBound(From->GetState(), To->GetState());
		}
	}
}

APlaygroundCharacter::State* APlaygroundCharacter::State::Step(APlaygroundCharacter* mc, float delta) {
	return this;
}

APlaygroundCharacter::State* APlaygroundCharacter::State::AttemptMove(APlaygroundCharacter* mc) {
	if (mc->RunPressed) {
		return &APlaygroundCharacter::RUNNING;
	}
	else {
		return &APlaygroundCharacter::WALKING;
	}
	
}

APlaygroundCharacter::State* APlaygroundCharacter::State::StopMove(APlaygroundCharacter* mc) {
	return this;
}

APlaygroundCharacter::State* APlaygroundCharacter::WalkingState::StopMove(APlaygroundCharacter* mc) {
	return &APlaygroundCharacter::IDLE;
}



FVector2D APlaygroundCharacter::State::ModifyMovement(APlaygroundCharacter* mc, FVector2D fv) { 
	return FVector2D::ZeroVector; 
}

FVector2D APlaygroundCharacter::WalkingState::ModifyMovement(APlaygroundCharacter* mc, FVector2D fv) { 
	return fv/mc->RunningScale; 
}

FVector2D APlaygroundCharacter::RunningState::ModifyMovement(APlaygroundCharacter* mc, FVector2D fv) { 
	return fv; 
}

void  APlaygroundCharacter::AirborneState::Enter(APlaygroundCharacter* mc) {
	
}

APlaygroundCharacter::State* APlaygroundCharacter::State::AttemptJump(APlaygroundCharacter* mc) {
	return &APlaygroundCharacter::AIRBORNE;
}

APlaygroundCharacter::State* APlaygroundCharacter::AirborneState::Step(APlaygroundCharacter* mc, float delta) {
	if (UCharacterMovementComponent* PlayerController = Cast<UCharacterMovementComponent>(mc->GetMovementComponent())) {
		if (PlayerController->IsFalling()) {
			return this;
		}
		else {
			return &IDLE;
		}
	}
	else {
		return this;
	}	
}
