// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "EditableArchetypeObject.generated.h"

/**
 * 
 */
UCLASS(Blueprintable, EditInlineNew, CollapseCategories)
class UEditableArchetypeObject : public UObject
{
	GENERATED_BODY()
	

protected:
	UPROPERTY(EditDefaultsOnly, Category = "Testing", meta=(AllowPrivateAccess=true))
	bool bValue;
	
};
