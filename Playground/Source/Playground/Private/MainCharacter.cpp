// Fill out your copyright notice in the Description page of Project Settings.


#include "MainCharacter.h"


// Sets default values
AMainCharacter::AMainCharacter()
{
 	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
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


	//for (FMCStateChangeListener l : this->listeners) {
	//	l.ExecuteIfBound(from, this->state->GetState());
	//}

}

// Called to bind functionality to input
void AMainCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

}


//void AMainCharacter::StateListen(const FMCStateChangeListener& Del) {
//	this->listeners.Add(Del);
//}

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
