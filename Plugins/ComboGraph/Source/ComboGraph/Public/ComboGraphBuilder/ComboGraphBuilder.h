// Copyright 2026 Prasanna Keerthivasan. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"

class UComboGraph;
class UComboNode;
class UComboGraphDataAsset;
class UDataTable;
struct FComboNodeData;


class COMBOGRAPH_API FComboGraphBuilder
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
