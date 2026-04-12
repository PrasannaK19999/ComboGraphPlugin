// Copyright 2026 Prasanna Keerthivasan. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "ComboGraphAsset.generated.h"

class UComboGraphDataAsset;
class UEdGraph_ComboGraph;
class UEdGraphNode_ComboNode;
struct FComboNodeData;
struct FGameplayTag;

/**
 * UComboGraphAsset
 *
 * Editor-only asset that provides visual graph authoring for combo graphs.
 * Lives entirely in the ComboGraphEditor module — never referenced by runtime code.
 *
 * Ownership model:
 *   UComboGraphAsset (this)
 *     |-- EdGraph (UEdGraph_ComboGraph) — visual node layout, editor-only
 *     +-- OutputAsset --> UComboGraphDataAsset (runtime, separate .uasset)
 *
 * On save: DFS-walks the EdGraph from the root node, rebuilds the recursive
 * FComboNodeData tree, and overwrites OutputAsset->RootNode in place.
 * First save auto-creates the runtime asset in the same Content Browser folder.
 *
 * On import: ImportFromDataAsset() walks an existing FComboNodeData tree,
 * creates EdGraphNodes + connections, and applies auto-layout. Lossless for
 * all authored data (montage, section, decay, transitions). Positions are lost.
 */
UCLASS(BlueprintType)
class COMBOGRAPHEDITOR_API UComboGraphAsset : public UObject
{
	GENERATED_BODY()

public:

	UComboGraphAsset();

	// -----------------------------------------------------------------------
	// EdGraph — visual representation, editor-only
	// -----------------------------------------------------------------------

	/** The editor graph containing visual nodes and connections. */
	UPROPERTY()
	TObjectPtr<UEdGraph_ComboGraph> EdGraph;

	// -----------------------------------------------------------------------
	// Runtime output — the asset consumers actually reference
	// -----------------------------------------------------------------------

	/**
	 * The runtime data asset this visual graph writes to on save.
	 * First save auto-creates it in the same Content Browser folder.
	 * Every subsequent save overwrites OutputAsset->RootNode in place.
	 * Consumer's combat component references this — never UComboGraphAsset.
	 */
	UPROPERTY(VisibleAnywhere, Category = "Combo Graph")
	TObjectPtr<UComboGraphDataAsset> OutputAsset;

	// -----------------------------------------------------------------------
	// Core operations
	// -----------------------------------------------------------------------

	/**
	 * Validates then compiles the visual EdGraph into the runtime OutputAsset.
	 * DFS-walks from the root EdGraphNode, builds the recursive FComboNodeData
	 * tree, writes it into OutputAsset->RootNode.
	 *
	 * If OutputAsset is null, auto-creates a new UComboGraphDataAsset
	 * in the same Content Browser folder as this asset and links it.
	 *
	 * @return true if compilation succeeded, false if validation errors exist.
	 */
	bool CompileToDataAsset();

	/**
	 * Imports an existing UComboGraphDataAsset into the visual editor.
	 * Clears the current EdGraph, walks the FComboNodeData tree recursively,
	 * creates UEdGraphNode_ComboNode per node, wires transitions as edges,
	 * applies auto-layout.
	 *
	 * Limitation: node positions are lost — FComboNodeData stores no layout data.
	 * All other data (montage, section, decay, transitions) reconstructs losslessly.
	 *
	 * @param Source The runtime data asset to import from. Must not be null.
	 * @return true if import succeeded.
	 */
	bool ImportFromDataAsset(const UComboGraphDataAsset* Source);

	// -----------------------------------------------------------------------
	// Validation
	// -----------------------------------------------------------------------

	/**
	 * Validates the EdGraph for correctness before compilation.
	 *
	 * Checks:
	 *   - Exactly one root node exists
	 *   - No cycles (DFS O(V+E))
	 *   - No duplicate tags on the same node
	 *   - No untagged connected output pins
	 *   - All non-root nodes have a montage assigned
	 *
	 * Reports errors via FMessageLog("ComboGraphCompiler") with
	 * clickable links that focus the offending node.
	 *
	 * @return true if the graph is valid, false if any errors exist.
	 */
	bool ValidateGraph() const;

	// -----------------------------------------------------------------------
	// Helpers
	// -----------------------------------------------------------------------

	/** Returns the root EdGraphNode, or nullptr if none exists. */
	UEdGraphNode_ComboNode* GetRootEdNode() const;

	// -----------------------------------------------------------------------
	// UObject interface
	// -----------------------------------------------------------------------

	virtual void PostInitProperties() override;
	virtual void PostEditUndo() override;

private:

	/**
	 * Recursively walks an EdGraphNode and its output connections,
	 * building the FComboNodeData tree.
	 *
	 * @param EdNode       The current editor graph node to compile.
	 * @param OutNodeData  The FComboNodeData struct to populate.
	 * @param VisitedNodes Set of already-visited node GUIDs for cycle detection.
	 * @return true if the subtree compiled without errors.
	 */
	bool CompileNodeRecursive(
		const UEdGraphNode_ComboNode* EdNode,
		FComboNodeData& OutNodeData,
		TSet<FGuid>& VisitedNodes
	) const;

	/**
	 * Recursively walks an FComboNodeData tree, creating EdGraphNodes
	 * and wiring connections for the import pipeline.
	 *
	 * @param NodeData       The source runtime node data.
	 * @param ParentEdNode   The parent EdGraphNode (null for root).
	 * @param ParentPinTag   The GameplayTag on the parent's output pin leading here.
	 * @param Depth          Tree depth — used for auto-layout X positioning.
	 * @param SiblingIndex   Index among siblings — used for auto-layout Y positioning.
	 */
	void ImportNodeRecursive(
		const FComboNodeData& NodeData,
		UEdGraphNode_ComboNode* ParentEdNode,
		const FGameplayTag& ParentPinTag,
		int32 Depth,
		int32 SiblingIndex
	);

	/**
	 * Creates or finds the output UComboGraphDataAsset in the same
	 * Content Browser directory as this asset. Called on first compile
	 * when OutputAsset is null. Names it "<ThisAssetName>_Data".
	 *
	 * @return The created or found data asset, or nullptr on failure.
	 */
	UComboGraphDataAsset* CreateOrFindOutputAsset();
};
