// Copyright 2026 Prasanna Keerthivasan. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Toolkits/AssetEditorToolkit.h"
#include "EditorUndoClient.h"

class UComboGraphAsset;
class SGraphEditor;
class IDetailsView;
class UEdGraphNode;

/**
 * FComboGraphAssetEditor
 *
 * Standalone editor window for UComboGraphAsset.
 * Opened by FComboGraphAssetTypeActions::OpenAssetEditor on double-click.
 *
 * Layout:
 *   +----------------------------------+------------------+
 *   |                                  |                  |
 *   |        Graph Panel (70%)         |  Details (30%)   |
 *   |        SGraphEditor              |  IDetailsView    |
 *   |        node canvas               |  selected node   |
 *   |                                  |  properties      |
 *   +----------------------------------+------------------+
 *
 * Toolbar actions:
 *   - Compile — calls UComboGraphAsset::CompileToDataAsset()
 *
 * Save (Ctrl+S):
 *   - Compiles the graph then saves the package.
 *     A save without a compile would leave OutputAsset stale.
 *
 * Undo/Redo:
 *   - Implements FEditorUndoClient to refresh the graph view after undo/redo.
 */
class COMBOGRAPHEDITOR_API FComboGraphAssetEditor
	: public FAssetEditorToolkit
	, public FEditorUndoClient
{
public:

	FComboGraphAssetEditor();
	virtual ~FComboGraphAssetEditor();

	/**
	 * Initializes and opens the editor window for the given asset.
	 * Called by FComboGraphAssetTypeActions::OpenAssetEditor.
	 *
	 * @param Mode                 Standalone or world-centric.
	 * @param InitToolkitHost      Host for world-centric mode. Null for standalone.
	 * @param Asset                The asset to edit. Must not be null.
	 */
	void InitComboGraphEditor(
		EToolkitMode::Type Mode,
		TSharedPtr<IToolkitHost> InitToolkitHost,
		UComboGraphAsset* Asset
	);

	// -----------------------------------------------------------------------
	// FAssetEditorToolkit interface
	// -----------------------------------------------------------------------

	virtual FName GetToolkitFName() const override;
	virtual FText GetBaseToolkitName() const override;
	virtual FString GetWorldCentricTabPrefix() const override;
	virtual FLinearColor GetWorldCentricTabColorScale() const override;

	virtual void RegisterTabSpawners(const TSharedRef<FTabManager>& InTabManager) override;
	virtual void UnregisterTabSpawners(const TSharedRef<FTabManager>& InTabManager) override;

	/**
	 * Compile then save.
	 * Overrides default save to ensure OutputAsset is always up to date
	 * when the package is written to disk.
	 */
	virtual void SaveAsset_Execute() override;

	// -----------------------------------------------------------------------
	// FEditorUndoClient interface
	// -----------------------------------------------------------------------

	virtual void PostUndo(bool bSuccess) override;
	virtual void PostRedo(bool bSuccess) override;

private:

	// -----------------------------------------------------------------------
	// Tab spawners
	// -----------------------------------------------------------------------

	TSharedRef<SDockTab> SpawnGraphTab(const FSpawnTabArgs& Args);
	TSharedRef<SDockTab> SpawnDetailsTab(const FSpawnTabArgs& Args);

	// -----------------------------------------------------------------------
	// Graph editor widget creation
	// -----------------------------------------------------------------------

	TSharedRef<SGraphEditor> CreateGraphEditorWidget();

	// -----------------------------------------------------------------------
	// Graph editor callbacks
	// -----------------------------------------------------------------------

	/** Updates the Details panel when node selection changes. */
	void OnSelectedNodesChanged(const TSet<UObject*>& SelectedNodes);

	/** Reserved for future use — double-click on a node (e.g. rename). */
	void OnNodeDoubleClicked(UEdGraphNode* Node);

	// -----------------------------------------------------------------------
	// Node deletion
	// -----------------------------------------------------------------------

	void DeleteSelectedNodes();
	bool CanDeleteSelectedNodes() const;

	// -----------------------------------------------------------------------
	// Toolbar
	// -----------------------------------------------------------------------

	void ExtendToolbar();
	void FillToolbar(FToolBarBuilder& ToolBarBuilder);
	void OnCompileClicked();

	// -----------------------------------------------------------------------
	// Command binding
	// -----------------------------------------------------------------------

	void BindGraphCommands();

	// -----------------------------------------------------------------------
	// Data
	// -----------------------------------------------------------------------

	UComboGraphAsset* EditedAsset = nullptr;

	TSharedPtr<SGraphEditor>  GraphEditor;
	TSharedPtr<IDetailsView>  DetailsView;
	TSharedPtr<FUICommandList> GraphEditorCommands;

	// -----------------------------------------------------------------------
	// Tab and app identifiers
	// -----------------------------------------------------------------------

	static const FName AppIdentifier;
	static const FName GraphTabId;
	static const FName DetailsTabId;
};
