// Copyright 2026 Prasanna Keerthivasan. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "EdGraph/EdGraphSchema.h"
#include "EdGraphSchema_ComboGraph.generated.h"

class UEdGraphNode_ComboNode;

/**
 * FComboGraphSchemaAction_AddNode
 *
 * Right-click context menu action that creates a new UEdGraphNode_ComboNode
 * at the cursor position. Registered in GetGraphContextActions().
 */
USTRUCT()
struct COMBOGRAPHEDITOR_API FComboGraphSchemaAction_AddNode : public FEdGraphSchemaAction
{
	GENERATED_BODY()

	FComboGraphSchemaAction_AddNode() {}

	FComboGraphSchemaAction_AddNode(
		FText InNodeCategory,
		FText InMenuDesc,
		FText InToolTip,
		int32 InGrouping
	)
		: FEdGraphSchemaAction(MoveTemp(InNodeCategory), MoveTemp(InMenuDesc), MoveTemp(InToolTip), InGrouping)
	{}

	// FEdGraphSchemaAction interface
	virtual UEdGraphNode* PerformAction(UEdGraph* ParentGraph, UEdGraphPin* FromPin, const FVector2D Location, bool bSelectNewNode = true) override;
};

/**
 * UEdGraphSchema_ComboGraph
 *
 * Defines the rules for the visual combo graph editor:
 *   - What nodes can be created (right-click context menu)
 *   - What connections are valid between pins
 *   - Auto-spawned root node on new graph creation
 *   - Pin colors
 *
 * Connection rules (enforced in CanCreateConnection):
 *   1. Both pins must be category "ComboTransition"
 *   2. Must be opposite directions — Output -> Input only
 *   3. Cannot connect a node to itself
 *   4. Input pin accepts exactly one connection (one parent per node)
 *   5. Output pin accepts exactly one connection (one target per transition tag)
 *   6. Cycle detection is deferred to compile-time validation (UComboGraphAsset::ValidateGraph)
 *      rather than enforced here — O(V+E) DFS per connection attempt is too expensive for UX
 */
UCLASS()
class COMBOGRAPHEDITOR_API UEdGraphSchema_ComboGraph : public UEdGraphSchema
{
	GENERATED_BODY()

public:

	// -----------------------------------------------------------------------
	// UEdGraphSchema interface
	// -----------------------------------------------------------------------

	/**
	 * Populates the right-click context menu on empty graph space.
	 * Adds: "Add Combo Node"
	 */
	virtual void GetGraphContextActions(FGraphContextMenuBuilder& ContextMenuBuilder) const override;

	/**
	 * Populates the right-click context menu on a node or pin.
	 * Adds: "Add Transition Pin" (on output-pin area), "Remove Pin" (on specific output pin)
	 */
	virtual void GetContextMenuActions(UToolMenu* Menu, UGraphNodeContextMenuContext* Context) const override;

	/**
	 * Validates whether two pins can be connected.
	 * Returns CONNECT_RESPONSE_MAKE on success or CONNECT_RESPONSE_DISALLOW with a reason.
	 * See connection rules in class comment above.
	 */
	virtual const FPinConnectionResponse CanCreateConnection(const UEdGraphPin* A, const UEdGraphPin* B) const override;

	/**
	 * Attempts to create a connection between two pins.
	 * Breaks existing connections on the input pin before connecting (one-parent rule).
	 * Breaks existing connection on the output pin before connecting (one-target rule).
	 */
	virtual bool TryCreateConnection(UEdGraphPin* A, UEdGraphPin* B) const override;

	/**
	 * Called when a new graph is created.
	 * Auto-spawns the root node at (0, 0) and marks it as bIsRootNode.
	 * The graph is invalid without a root — this guarantees one always exists.
	 */
	virtual void CreateDefaultNodesForGraph(UEdGraph& Graph) const override;

	/** Returns the display color for a given pin type. ComboTransition pins are white. */
	virtual FLinearColor GetPinTypeColor(const FEdGraphPinType& PinType) const override;

	virtual void BreakNodeLinks(UEdGraphNode& TargetNode) const override;
	virtual void BreakPinLinks(UEdGraphPin& TargetPin, bool bSendsNodeNotification) const override;
	virtual void BreakSinglePinLink(UEdGraphPin* SourcePin, UEdGraphPin* TargetPin) const override;

	// -----------------------------------------------------------------------
	// Helpers
	// -----------------------------------------------------------------------

	/**
	 * Creates a new UEdGraphNode_ComboNode at the given graph position.
	 * Handles Modify(), node registration, AllocateDefaultPins(), and graph notification.
	 *
	 * @param Graph        The graph to add the node to.
	 * @param Position     Screen-space position for the new node.
	 * @param bSelectNewNode  Whether to select the node after creation.
	 * @return The created node, or nullptr on failure.
	 */
	static UEdGraphNode_ComboNode* CreateComboNode(
		UEdGraph* Graph,
		const FVector2D& Position,
		bool bSelectNewNode = true
	);
};
