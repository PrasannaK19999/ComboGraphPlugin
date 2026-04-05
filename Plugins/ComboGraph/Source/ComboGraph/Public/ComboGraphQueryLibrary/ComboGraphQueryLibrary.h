#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "GameplayTagContainer.h"
#include "ComboGraphQueryLibrary.generated.h"

class UComboGraph;

UCLASS()
class COMBOGRAPH_API UComboGraphQueryLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Combo")
	static int32 GetCurrentComboDepth(const UComboGraph* Graph);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Combo")
	static TArray<FGameplayTag> GetComboHistory(const UComboGraph* Graph);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Combo")
	static FGameplayTag GetLastComboTag(const UComboGraph* Graph);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Combo")
	static bool IsComboActive(const UComboGraph* Graph);
};
