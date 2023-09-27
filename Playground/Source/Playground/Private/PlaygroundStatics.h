// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "PlaygroundStatics.generated.h"

UENUM(BlueprintType)
enum class ECastAnimationID : uint8 {
	FIREBALL           UMETA(DisplayName = "Fireball"),    
};


/**
	Structure used for spell data. This defines how spells are cast, their spellbook display, and other properties
**/
USTRUCT(BlueprintType)
struct FSpellData : public FTableRowBase
{
	GENERATED_USTRUCT_BODY()

public:   

	/** Display name of the spell in the spellbook. **/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = LevelUp)
	FString SpellDisplayName = "UNNAMED_SPELL";

	/** Time it takes to cast the spell **/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = LevelUp)
	float CastTime = 1.0f;

	/** ID of the cast animation to use. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = LevelUp)
	ECastAnimationID AnimID = ECastAnimationID::FIREBALL;

	/** Icon to use for Spellbook display */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = LevelUp)
	TSoftObjectPtr<UTexture> SpellBookIcon;

	/** Icon to use for Spellbook display */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = LevelUp)
	TSubclassOf<AActor> SpellClass;
};

/**
 * 
 */
UCLASS()
class UPlaygroundStatics : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
};
