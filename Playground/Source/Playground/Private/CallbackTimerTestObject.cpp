// Fill out your copyright notice in the Description page of Project Settings.

#include "Kismet2/BlueprintEditorUtils.h"

#include "CallbackTimerTestObject.h"

void UCallbackTimerTestObject::PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent) {
	Super::PostEditChangeProperty(PropertyChangedEvent);


	if (GEngine)
		GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Yellow, TEXT("Changed a property!"));
	UE_LOG(LogTemp, Warning, TEXT("Changed a property?"));

	if (UBlueprint* BlueprintAsset = UBlueprint::GetBlueprintFromClass(this->GetClass()))
	{
		if (GEngine)
			GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Yellow, TEXT("Found a blueprint asset!"));

		UE_LOG(LogTemp, Warning, TEXT("Found a blueprint asset?"));

		FName PropFName = FName("MyProperty");
		FEdGraphPinType PinType;
		PinType.ContainerType = EPinContainerType::None;
		//PinType.PinCategory = UEdGraphSchema_K2::PC_Real;
		//PinType.PinSubCategory = UEdGraphSchema_K2::PC_Float;
		PinType.PinCategory = UEdGraphSchema_K2::PC_Struct;
		PinType.PinSubCategoryObject = FTestingStructure::StaticStruct();

		FBlueprintEditorUtils::AddMemberVariable(BlueprintAsset, PropFName, PinType);
		const int32 VarIndex =
			FBlueprintEditorUtils::FindNewVariableIndex(BlueprintAsset, PropFName);

		if (VarIndex != INDEX_NONE)
		{
			//BlueprintAsset->NewVariables[VarIndex].PropertyFlags &= ~CPF_DisableEditOnInstance;
			BlueprintAsset->NewVariables[VarIndex].PropertyFlags |= CPF_EditConst;
			BlueprintAsset->NewVariables[VarIndex].PropertyFlags &= ~CPF_Edit;
			BlueprintAsset->NewVariables[VarIndex].PropertyFlags |= CPF_BlueprintReadOnly;

			BlueprintAsset->NewVariables[VarIndex].Category = FText::FromString("Static|Events");
			
		}
		FBlueprintEditorUtils::SetVariableSaveGameFlag(BlueprintAsset, PropFName, false);

		UE_LOG(LogTemp, Warning, TEXT("Added property??"));
	}
}