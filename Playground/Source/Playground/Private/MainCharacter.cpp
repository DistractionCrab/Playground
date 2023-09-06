// Fill out your copyright notice in the Description page of Project Settings.


#include "MainCharacter.h"

// Singleton object declarations for linkage.
AMainCharacter::MCState AMainCharacter::IDLE;
AMainCharacter::WalkingState AMainCharacter::WALKING;

// Sets default values
AMainCharacter::AMainCharacter()
{
 	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;	
	this->state = &IDLE;
}

// Called when the game starts or when spawned
void AMainCharacter::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void AMainCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	EMainCharacterState from = this->state->GetState();

	this->state = this->state->Step(this, DeltaTime);

	for (FMCStateChangeListener l : this->listeners) {
		l.ExecuteIfBound(from, this->state->GetState());
	}

}

// Called to bind functionality to input
void AMainCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

}


EMainCharacterState AMainCharacter::CurrentState() {
	return this->state->GetState();
}

void AMainCharacter::StateListen(const FMCStateChangeListener& Del) {
	this->listeners.Add(Del);
}

void AMainCharacter::InputAxis(float x, float y) {
	this->AxisVec.X = x;
	this->AxisVec.Y = y;
}

void AMainCharacter::InputAxisX(float x) {
	this->AxisVec.X = x;
}

void AMainCharacter::InputAxisY(float y) {
	this->AxisVec.Y = y;
}

void AMainCharacter::InputRun(bool run) {
	this->isRunning = run;
}

void AMainCharacter::ViewAxis(float x, float y, float z) {

}

AMainCharacter::MCState* AMainCharacter::WalkingState::Step(AMainCharacter* mc, float delta) {
	// If the input is negligible, then return to the Idle state.
	if (FVector::DistSquared(mc->AxisVec, FVector::ZeroVector) <= 0.0000001) {
		return &AMainCharacter::IDLE;
		//return this;
	}
	// Otherwise we need to update the velocity of the
	else {
		this->HandleVelocityUpdate(mc, delta);
		return this;
	}
}

void AMainCharacter::WalkingState::HandleAngularUpdate(AMainCharacter* mc, float delta) {
	// If the forward vector isn't being managed by something else, then update it ourselves.
	if (!mc->ViewOverride) {
		
		FVector rotation = mc->ViewVec.GetSafeNormal(0.00001) * mc->AngularVelocity;
		FRotator rot = FRotator(rotation.Y, rotation.X, rotation.Z);

		mc->Forward = rot.RotateVector(mc->Forward);
	}

	// Once the Forward has been updated, we need to interpolate our actors rotation to match the forward.
	//
	// In the case that there is no ViewOverride, we simply just snap the main character to the change in angle, 
	// as the character cannot out spin the angular velocity. This situation is for when the character's rotation is locked with the
	// camera. Generally an older style, and doesn't play too well.
	//
	// In the second case, the character's perspective is changing, but the character's physical rotation isn't locked with it.
	// In this case we interpolate from the character's rotation to the Forward (i.e. perspective) direction. This case is generally for
	// Situations in which the camera is free to move, but the character is not locked with the camera, so when moving the character is
	// reoriented to match the camera (perspective).
	if (!mc->ViewOverride) {
		mc->SetActorRotation(mc->Forward.Rotation());
	}
	else {
		FRotator r1 = mc->GetActorRotation();
		FRotator r2 = mc->Forward.Rotation();
		double angle = FMath::Acos(FVector::DotProduct(r1.RotateVector(FVector::ForwardVector), r2.RotateVector(FVector::ForwardVector)));

		if (FMath::Abs(angle) <= 0.00001) {
			mc->SetActorRotation(mc->Forward.Rotation());
		}
		else {
			
			double deltaAngle = delta * mc->AngularVelocity;
			double alpha = deltaAngle / angle;


			mc->SetActorRotation(FMath::Lerp(
				r1, 
				r2, 
				FMath::Clamp(deltaAngle/FMath::Abs(angle), 0.0, 1.0)));
		}
		
	}
	
}

void AMainCharacter::WalkingState::HandleVelocityUpdate(AMainCharacter* mc, float delta) {
	FRotator rotation = mc->Forward.Rotation();
	FVector velocity = rotation.RotateVector((mc->AxisVec.GetSafeNormal(0.000001) * mc->WalkVelocity));
	mc->AddMovementInput(velocity * delta);	
}

AMainCharacter::MCState* AMainCharacter::MCState::Step(AMainCharacter* mc, float delta) {
	if (mc->AxisVec.SquaredLength() > 0.00001) {
		return &AMainCharacter::WALKING;
		//return this;
	}
	else {
		if (mc->GetVelocity().SquaredLength() > 0.01) {
			mc->PostNetReceiveVelocity(mc->GetVelocity() * 0.1);
		}
		else {
			mc->PostNetReceiveVelocity(FVector::ZeroVector);
		}
		return this;
	}
	
}