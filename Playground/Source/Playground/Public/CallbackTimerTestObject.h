// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "CallbackTimerTestObject.generated.h"

DECLARE_DELEGATE(FSimpleTestCallbackDelegate);

USTRUCT()
struct FTestingStructure {
	GENERATED_BODY();

public:
	UPROPERTY(VisibleAnywhere, Category="Testing")
	int TestValue;
};

/**
 * 
 */
UCLASS(Blueprintable)
class PLAYGROUND_API UCallbackTimerTestObject : public UObject
{
	GENERATED_BODY()
	

public:

	UPROPERTY(EditAnywhere, Category="Testing")
	TArray<FName> Values;

	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Testing")
	void BPCallbackEvent();
	virtual void BPCallbackEvent_Implementation() {}

	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Testing")
	void BPCallbackSwitchEvent(FName EName);
	virtual void BPCallbackSwitchEvent_Implementation(FName EName) {}

	virtual void PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent) override;
};
