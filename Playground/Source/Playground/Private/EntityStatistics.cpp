// Fill out your copyright notice in the Description page of Project Settings.


#include "EntityStatistics.h"

// Sets default values for this component's properties
UEntityStatistics::UEntityStatistics()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = false;
}


// Called when the game starts
void UEntityStatistics::BeginPlay()
{
	Super::BeginPlay();
	
}


// Called every frame
void UEntityStatistics::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// ...
}


void UEntityStatistics::ApplyDamage(int Dmg) {

}

void UEntityStatistics::HealthListen(const FStatisticsListenerDiff& Del) {
	this->HealthDelegates.Add(Del);
}