#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "ComboPath.generated.h"

class UComboNode;

UCLASS(BlueprintType)
class COMBOGRAPH_API UComboPath : public UObject
{
	GENERATED_BODY()

public:

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combo")
	TObjectPtr<UComboNode> TargetNode;

	/** Optional human-readable label for editor/debug display. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combo")
	FName TransitionName;
};
