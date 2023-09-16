// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "MainCharacter.generated.h"




UCLASS()
class PLAYGROUND_API AMainCharacter : public ACharacter
{
	GENERATED_BODY()


private:
	/* Vector which represents directional inputs. Used to compute movement. */
	FVector AxisVec = FVector::ZeroVector;
	/* Vector representing rotations around an axis. Used to rotate character on camera changes. */
	FVector ViewVec = FVector::ZeroVector;
	/* Whether or not running should happen. This will generally be disregarded in non-walking/running states. */
	bool isRunning = false;

protected:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Main Character/Physics")
	float WalkVelocity = 500.0f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Main Character/Physics")
	float RunVelocity = 1000.0f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Main Character/Physics")
	float AngularVelocity = 5.0f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Main Character/Physics")
	FVector Forward = FVector::ForwardVector;
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:
	/* Whether or not view rotations and Forward will be overriden*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Main Character/Physics")
	bool ViewOverride = false;

public:
	// Sets default values for this character's properties
	AMainCharacter();

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;


	//UFUNCTION(BlueprintCallable, Category = "Main Character")
	//virtual void StateListen(const FMCStateChangeListener& Del);

	//UFUNCTION(BlueprintPure, Category = "Main Character")
	//virtual EMainCharacterState CurrentState();

	UFUNCTION(BlueprintCallable, Category = "Main Character")
	virtual void InputAxis(float x, float y);

	UFUNCTION(BlueprintCallable, Category = "Main Character")
	virtual void InputAxisX(float x);

	UFUNCTION(BlueprintCallable, Category = "Main Character")
	virtual void InputAxisY(float y);

	UFUNCTION(BlueprintCallable, Category = "Main Character")
	virtual void InputRun(bool run);

	UFUNCTION(BlueprintCallable, Category = "Main Character")
	virtual void ViewAxis(float x, float y, float z);
};


