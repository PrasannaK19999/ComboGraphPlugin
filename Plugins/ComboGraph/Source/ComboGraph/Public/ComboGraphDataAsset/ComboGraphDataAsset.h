// Copyright 2026 Prasanna Keerthivasan. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Engine/DataTable.h"
#include "GameplayTagContainer.h"
#include "StructUtils/InstancedStruct.h"
#include "ComboGraphDataAsset.generated.h"

class UAnimMontage;

USTRUCT(BlueprintType)
struct COMBOGRAPH_API FComboNodeData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combo")
	FName DisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combo")
	TObjectPtr<UAnimMontage> Montage = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combo")
	FName MontageSection = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combo")
	float DecayTime = 0.5f;

	/**
	 * Exits from this node. Key = GameplayTag input, Value = FInstancedStruct
	 * holding the next FComboNodeData. Self-nesting builds the combo tree.
	 * Empty map = leaf node (combo ends here).
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combo", meta = (BaseStruct = "/Script/ComboGraph.ComboNodeData"))
	TMap<FGameplayTag, FInstancedStruct> Transitions;
};

UCLASS()
class COMBOGRAPH_API UComboGraphDataAsset : public UDataAsset
{
	GENERATED_BODY()

public:

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combo")
	FComboNodeData RootNode;
};

USTRUCT(BlueprintType)
struct COMBOGRAPH_API FComboGraphTableRow : public FTableRowBase
{
	GENERATED_BODY()

	/** The input tag that starts this combo chain (e.g., Input.Light). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combo")
	FGameplayTag InputTag;

	/** Reference to the DataAsset holding the full combo chain. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combo")
	TObjectPtr<UComboGraphDataAsset> ComboChain = nullptr;
};
