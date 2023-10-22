// Fill out your copyright notice in the Description page of Project Settings.


#include "EventCharMovementComponent.h"

void UEventCharMovementComponent::OnMovementModeChanged(EMovementMode Previous, uint8 PreviousCustom) {
	Super::OnMovementModeChanged(Previous, PreviousCustom);
	this->MovementModeChangedEvent.Broadcast(this, Previous, PreviousCustom);
}

void UEventCharMovementComponent::OnMovementUpdated(float DeltaSeconds, const FVector& OldLocation, const FVector& OldVelocity) {
	if (GEngine)
		GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Yellow, 
			FString::Printf(TEXT("Z-velocity: %f"), OldVelocity.Z));
	Super::OnMovementUpdated(DeltaSeconds, OldLocation, OldVelocity);
}