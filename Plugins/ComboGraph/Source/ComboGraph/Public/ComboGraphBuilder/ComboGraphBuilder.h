#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"

class UComboGraph;
class UComboNode;
class UComboGraphDataAsset;
struct FComboNodeData;


class FComboGraphBuilder
{
public:

	static UComboGraph* BuildFromDataAsset(UObject* Outer, const UComboGraphDataAsset* Asset);
	static TMap<FGameplayTag, UComboGraph*> BuildFromDataTable(UObject* Outer, const UDataTable* DataTable);

private:

	/**
	 * Recursively creates UComboNode from FComboNodeData and wires transitions.
	 * Each call creates one node and recurses into its Transitions map.
	 */
	static UComboNode* BuildNodeRecursive(UComboGraph* Graph, const FComboNodeData& NodeData, TArray<UComboNode*>& OutAllNodes);
};
