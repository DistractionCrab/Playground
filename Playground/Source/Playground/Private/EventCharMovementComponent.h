// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Delegates/Delegate.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "EventCharMovementComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(
	FMovementModeChanged, 
	UEventCharMovementComponent*, Component,
	EMovementMode, Previous,
	uint8, PreviousCustom);

/**
 * 
 */
UCLASS()
class UEventCharMovementComponent : public UCharacterMovementComponent
{
	GENERATED_BODY()

protected:	
	virtual void OnMovementModeChanged(EMovementMode Previous, uint8 PreviousCustom) override;
	virtual void OnMovementUpdated(float DeltaSeconds, const FVector& OldLocation, const FVector& OldVelocity) override;

public:

	UPROPERTY(BlueprintReadWrite, BlueprintAssignable, Category="Movement")
	FMovementModeChanged MovementModeChangedEvent;
};
