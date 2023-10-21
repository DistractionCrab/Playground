// Fill out your copyright notice in the Description page of Project Settings.


#include "EventCharMovementComponent.h"

void UEventCharMovementComponent::OnMovementModeChanged(EMovementMode Previous, uint8 PreviousCustom) {
	Super::OnMovementModeChanged(Previous, PreviousCustom);
	this->MovementModeChangedEvent.Broadcast(this, Previous, PreviousCustom);
}