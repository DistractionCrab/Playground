// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Delegates/Delegate.h"
#include "EntityStatistics.generated.h"

DECLARE_DYNAMIC_DELEGATE_TwoParams(FStatisticsListenerDiff, int, Base, int, Diff);

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class UEntityStatistics : public UActorComponent
{
	GENERATED_BODY()

private:
	TArray<FStatisticsListenerDiff> HealthDelegates;

	
public:	
	// Sets default values for this component's properties
	UEntityStatistics();

protected:
	// -- Below are the statistics managed by this class. Each will have a listener for changes.
	// Health value
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "NPC Statistics")
	int Health;

	// Called when the game starts
	virtual void BeginPlay() override;

	UFUNCTION(BlueprintCallable, Category = "NPC Statistics")
	virtual void ApplyDamage(int Dmg);

public:	
	// Called every frame
	virtual void TickComponent(
		float DeltaTime, 
		ELevelTick TickType, 
		FActorComponentTickFunction* ThisTickFunction) 
		override;

	UFUNCTION(BlueprintCallable, Category = "NPC Statistics")
	virtual void HealthListen(const FStatisticsListenerDiff& Del);
};
