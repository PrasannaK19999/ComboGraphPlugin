// Copyright 2026 Prasanna Keerthivasan. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "EdGraph/EdGraph.h"
#include "EdGraph_ComboGraph.generated.h"

class UEdGraphNode_ComboNode;
class UComboGraphAsset;

/**
 * UEdGraph_ComboGraph
 *
 * The graph container for the visual combo editor.
 * Owned by UComboGraphAsset as its EdGraph property.
 *
 * Responsibilities:
 *   - Holds all UEdGraphNode_ComboNode instances
 *   - Provides access to the single root node
 *   - Marks the owning UComboGraphAsset dirty when the graph changes
 *     so the user is prompted to save
 *
 * This class has no traversal or compilation logic — that lives in
 * UComboGraphAsset::CompileToDataAsset(). This class is purely a container.
 */
UCLASS()
class COMBOGRAPHEDITOR_API UEdGraph_ComboGraph : public UEdGraph
{
	GENERATED_BODY()

public:

	/**
	 * Returns the single root node in the graph.
	 * Scans all nodes for bIsRootNode == true.
	 * Returns nullptr if the graph has no root (invalid state).
	 */
	UEdGraphNode_ComboNode* GetRootNode() const;

	/**
	 * Returns all combo nodes in the graph as typed pointers.
	 * All nodes in this graph are UEdGraphNode_ComboNode — no mixed types.
	 */
	TArray<UEdGraphNode_ComboNode*> GetAllComboNodes() const;

	/**
	 * Returns the owning UComboGraphAsset.
	 * The asset is always the Outer of this graph — GetOuter() cast to UComboGraphAsset*.
	 * Returns nullptr if called outside a valid asset context.
	 */
	UComboGraphAsset* GetOwnerAsset() const;

	// -----------------------------------------------------------------------
	// UEdGraph interface
	// -----------------------------------------------------------------------

	/** Marks the owning asset dirty when graph topology changes. */
	virtual void NotifyGraphChanged() override;

	/** Marks the owning asset dirty when graph topology changes (action-specific overload). */
	virtual void NotifyGraphChanged(const FEdGraphEditAction& Action) override;
};
