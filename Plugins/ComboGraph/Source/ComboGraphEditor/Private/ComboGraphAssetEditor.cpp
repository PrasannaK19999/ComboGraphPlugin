// Copyright 2026 Prasanna Keerthivasan. All Rights Reserved.

#include "ComboGraphAssetEditor.h"
#include "ComboGraphAsset.h"
#include "EdGraph_ComboGraph.h"
#include "EdGraphNode_ComboNode.h"

#include "GraphEditor.h"
#include "GraphEditorActions.h"
#include "EdGraph/EdGraphSchema.h"
#include "Framework/Commands/GenericCommands.h"
#include "Framework/Commands/UICommandList.h"
#include "Framework/MultiBox/MultiBoxBuilder.h"
#include "IDetailsView.h"
#include "PropertyEditorModule.h"
#include "Modules/ModuleManager.h"
#include "Widgets/Docking/SDockTab.h"
#include "ScopedTransaction.h"
#include "ToolMenus.h"
#include "Dialogs/Dialogs.h"

#define LOCTEXT_NAMESPACE "ComboGraphAssetEditor"

// -----------------------------------------------------------------------
// Tab and app identifiers
// -----------------------------------------------------------------------

const FName FComboGraphAssetEditor::AppIdentifier(TEXT("ComboGraphAssetEditorApp"));
const FName FComboGraphAssetEditor::GraphTabId(TEXT("ComboGraphEditor_GraphTab"));
const FName FComboGraphAssetEditor::DetailsTabId(TEXT("ComboGraphEditor_DetailsTab"));

// -----------------------------------------------------------------------
// Constructor / Destructor
// -----------------------------------------------------------------------

FComboGraphAssetEditor::FComboGraphAssetEditor()
{
}

FComboGraphAssetEditor::~FComboGraphAssetEditor()
{
	GEditor->UnregisterForUndo(this);
}

// -----------------------------------------------------------------------
// Init
// -----------------------------------------------------------------------

void FComboGraphAssetEditor::InitComboGraphEditor(
	EToolkitMode::Type Mode,
	TSharedPtr<IToolkitHost> InitToolkitHost,
	UComboGraphAsset* Asset)
{
	check(Asset);
	EditedAsset = Asset;

	// Register for undo/redo notifications
	GEditor->RegisterForUndo(this);

	// Create the graph editor commands and bind them
	GraphEditorCommands = MakeShareable(new FUICommandList());
	BindGraphCommands();

	// Create the Details panel
	FPropertyEditorModule& PropertyEditorModule =
		FModuleManager::LoadModuleChecked<FPropertyEditorModule>("PropertyEditor");

	FDetailsViewArgs DetailsViewArgs;
	DetailsViewArgs.bHideSelectionTip  = true;
	DetailsViewArgs.bAllowSearch       = false;
	DetailsViewArgs.NameAreaSettings   = FDetailsViewArgs::HideNameArea;
	DetailsViewArgs.bShowPropertyMatrixButton = false;

	DetailsView = PropertyEditorModule.CreateDetailView(DetailsViewArgs);
	DetailsView->SetObject(EditedAsset);

	// Editor layout — graph left (70%), details right (30%)
	const TSharedRef<FTabManager::FLayout> Layout =
		FTabManager::NewLayout("ComboGraphAssetEditor_Layout_v1")
		->AddArea
		(
			FTabManager::NewPrimaryArea()
			->SetOrientation(Orient_Horizontal)
			->Split
			(
				FTabManager::NewStack()
				->SetSizeCoefficient(0.7f)
				->SetHideTabWell(true)
				->AddTab(GraphTabId, ETabState::OpenedTab)
			)
			->Split
			(
				FTabManager::NewStack()
				->SetSizeCoefficient(0.3f)
				->SetHideTabWell(true)
				->AddTab(DetailsTabId, ETabState::OpenedTab)
			)
		);

	// Initialize the toolkit — this creates the window and tab manager
	InitAssetEditor(
		Mode,
		InitToolkitHost,
		AppIdentifier,
		Layout,
		true,   // bCreateDefaultStandaloneMenu
		true,   // bCreateDefaultToolbar
		Asset
	);

	// Extend toolbar with the Compile button
	ExtendToolbar();

	RegenerateMenusAndToolbars();
}

// -----------------------------------------------------------------------
// FAssetEditorToolkit interface
// -----------------------------------------------------------------------

FName FComboGraphAssetEditor::GetToolkitFName() const
{
	return FName(TEXT("ComboGraphAssetEditor"));
}

FText FComboGraphAssetEditor::GetBaseToolkitName() const
{
	return LOCTEXT("AppLabel", "Combo Graph Editor");
}

FString FComboGraphAssetEditor::GetWorldCentricTabPrefix() const
{
	return LOCTEXT("WorldCentricTabPrefix", "ComboGraph ").ToString();
}

FLinearColor FComboGraphAssetEditor::GetWorldCentricTabColorScale() const
{
	// Matches the asset type color — amber
	return FLinearColor(0.86f, 0.55f, 0.08f);
}

void FComboGraphAssetEditor::RegisterTabSpawners(const TSharedRef<FTabManager>& InTabManager)
{
	WorkspaceMenuCategory = InTabManager->AddLocalWorkspaceMenuCategory(
		LOCTEXT("WorkspaceMenu", "Combo Graph Editor")
	);

	FAssetEditorToolkit::RegisterTabSpawners(InTabManager);

	InTabManager->RegisterTabSpawner(GraphTabId, FOnSpawnTab::CreateSP(this, &FComboGraphAssetEditor::SpawnGraphTab))
		.SetDisplayName(LOCTEXT("GraphTabLabel", "Graph"))
		.SetGroup(WorkspaceMenuCategory.ToSharedRef());

	InTabManager->RegisterTabSpawner(DetailsTabId, FOnSpawnTab::CreateSP(this, &FComboGraphAssetEditor::SpawnDetailsTab))
		.SetDisplayName(LOCTEXT("DetailsTabLabel", "Details"))
		.SetGroup(WorkspaceMenuCategory.ToSharedRef());
}

void FComboGraphAssetEditor::UnregisterTabSpawners(const TSharedRef<FTabManager>& InTabManager)
{
	FAssetEditorToolkit::UnregisterTabSpawners(InTabManager);

	InTabManager->UnregisterTabSpawner(GraphTabId);
	InTabManager->UnregisterTabSpawner(DetailsTabId);
}

void FComboGraphAssetEditor::SaveAsset_Execute()
{
	// Always compile before saving — OutputAsset must be up to date when written to disk
	if (EditedAsset)
	{
		EditedAsset->CompileToDataAsset();
	}

	FAssetEditorToolkit::SaveAsset_Execute();
}

// -----------------------------------------------------------------------
// FEditorUndoClient interface
// -----------------------------------------------------------------------

void FComboGraphAssetEditor::PostUndo(bool bSuccess)
{
	if (bSuccess && GraphEditor.IsValid())
	{
		GraphEditor->ClearSelectionSet();
		GraphEditor->NotifyGraphChanged();
	}
}

void FComboGraphAssetEditor::PostRedo(bool bSuccess)
{
	PostUndo(bSuccess);
}

// -----------------------------------------------------------------------
// Tab spawners
// -----------------------------------------------------------------------

TSharedRef<SDockTab> FComboGraphAssetEditor::SpawnGraphTab(const FSpawnTabArgs& Args)
{
	return SNew(SDockTab)
		.Label(LOCTEXT("GraphTabLabel", "Graph"))
		[
			CreateGraphEditorWidget()
		];
}

TSharedRef<SDockTab> FComboGraphAssetEditor::SpawnDetailsTab(const FSpawnTabArgs& Args)
{
	return SNew(SDockTab)
		.Label(LOCTEXT("DetailsTabLabel", "Details"))
		[
			DetailsView.ToSharedRef()
		];
}

// -----------------------------------------------------------------------
// Graph editor widget
// -----------------------------------------------------------------------

TSharedRef<SGraphEditor> FComboGraphAssetEditor::CreateGraphEditorWidget()
{
	check(EditedAsset && EditedAsset->EdGraph);

	FGraphAppearanceInfo AppearanceInfo;
	AppearanceInfo.CornerText = LOCTEXT("CornerText", "COMBO GRAPH");

	SGraphEditor::FGraphEditorEvents GraphEvents;
	GraphEvents.OnSelectionChanged = SGraphEditor::FOnSelectionChanged::CreateSP(
		this, &FComboGraphAssetEditor::OnSelectedNodesChanged
	);
	GraphEvents.OnNodeDoubleClicked = FSingleNodeEvent::CreateSP(
		this, &FComboGraphAssetEditor::OnNodeDoubleClicked
	);

	GraphEditor = SNew(SGraphEditor)
		.AdditionalCommands(GraphEditorCommands)
		.Appearance(AppearanceInfo)
		.GraphToEdit(EditedAsset->EdGraph)
		.GraphEvents(GraphEvents);

	return GraphEditor.ToSharedRef();
}

// -----------------------------------------------------------------------
// Graph editor callbacks
// -----------------------------------------------------------------------

void FComboGraphAssetEditor::OnSelectedNodesChanged(const TSet<UObject*>& SelectedNodes)
{
	if (!DetailsView.IsValid())
	{
		return;
	}

	TArray<UObject*> Selection;

	if (SelectedNodes.Num() > 0)
	{
		for (UObject* Node : SelectedNodes)
		{
			Selection.Add(Node);
		}
	}
	else
	{
		// Nothing selected — show the asset itself in the Details panel
		Selection.Add(EditedAsset);
	}

	DetailsView->SetObjects(Selection);
}

void FComboGraphAssetEditor::OnNodeDoubleClicked(UEdGraphNode* Node)
{
	// Reserved — could open a rename dialog or montage picker in a future version
}

// -----------------------------------------------------------------------
// Node deletion
// -----------------------------------------------------------------------

void FComboGraphAssetEditor::DeleteSelectedNodes()
{
	if (!GraphEditor.IsValid())
	{
		return;
	}

	const TSet<UObject*> SelectedNodes = GraphEditor->GetSelectedNodes();

	// Count connections that will be broken across all deletable selected nodes
	int32 ConnectionCount = 0;
	for (UObject* NodeObj : SelectedNodes)
	{
		const UEdGraphNode* Node = Cast<UEdGraphNode>(NodeObj);
		if (Node && Node->CanUserDeleteNode())
		{
			for (const UEdGraphPin* Pin : Node->Pins)
			{
				if (Pin)
				{
					ConnectionCount += Pin->LinkedTo.Num();
				}
			}
		}
	}

	// Warn before breaking connections — suppressable so repeat users can opt out
	if (ConnectionCount > 0)
	{
		FSuppressableWarningDialog::FSetupInfo Info(
			FText::Format(
				LOCTEXT("DeleteNodeWarning",
					"Deleting the selected node(s) will break {0} connection(s).\n\nProceed?"),
				FText::AsNumber(ConnectionCount)
			),
			LOCTEXT("DeleteNodeWarningTitle", "Delete Nodes"),
			TEXT("ComboGraph_DeleteNodeWarning")
		);
		Info.ConfirmText = LOCTEXT("DeleteConfirm", "Delete");
		Info.CancelText  = LOCTEXT("DeleteCancel",  "Cancel");

		FSuppressableWarningDialog DeleteWarning(Info);
		if (DeleteWarning.ShowModal() == FSuppressableWarningDialog::Cancel)
		{
			return;
		}
	}

	const FScopedTransaction Transaction(LOCTEXT("DeleteSelectedNodes", "Delete Selected Nodes"));
	EditedAsset->EdGraph->Modify();

	GraphEditor->ClearSelectionSet();

	for (UObject* NodeObj : SelectedNodes)
	{
		UEdGraphNode* Node = Cast<UEdGraphNode>(NodeObj);
		if (Node && Node->CanUserDeleteNode())
		{
			Node->Modify();
			Node->DestroyNode();
		}
	}
}

bool FComboGraphAssetEditor::CanDeleteSelectedNodes() const
{
	if (!GraphEditor.IsValid())
	{
		return false;
	}

	for (UObject* NodeObj : GraphEditor->GetSelectedNodes())
	{
		const UEdGraphNode* Node = Cast<UEdGraphNode>(NodeObj);
		if (Node && Node->CanUserDeleteNode())
		{
			return true;
		}
	}

	return false;
}

// -----------------------------------------------------------------------
// Toolbar
// -----------------------------------------------------------------------

void FComboGraphAssetEditor::ExtendToolbar()
{
	TSharedPtr<FExtender> ToolbarExtender = MakeShareable(new FExtender());

	ToolbarExtender->AddToolBarExtension(
		"Asset",
		EExtensionHook::After,
		GraphEditorCommands,
		FToolBarExtensionDelegate::CreateSP(this, &FComboGraphAssetEditor::FillToolbar)
	);

	AddToolbarExtender(ToolbarExtender);
}

void FComboGraphAssetEditor::FillToolbar(FToolBarBuilder& ToolBarBuilder)
{
	ToolBarBuilder.BeginSection(TEXT("ComboGraph"));
	{
		ToolBarBuilder.AddToolBarButton(
			FUIAction(FExecuteAction::CreateSP(this, &FComboGraphAssetEditor::OnCompileClicked)),
			NAME_None,
			LOCTEXT("CompileButton", "Compile"),
			LOCTEXT("CompileButtonTooltip", "Validates and compiles the graph into the linked UComboGraphDataAsset."),
			FSlateIcon(FAppStyle::GetAppStyleSetName(), "LevelEditor.Recompile")
		);
	}
	ToolBarBuilder.EndSection();
}

void FComboGraphAssetEditor::OnCompileClicked()
{
	if (EditedAsset)
	{
		EditedAsset->CompileToDataAsset();
	}
}

// -----------------------------------------------------------------------
// Command binding
// -----------------------------------------------------------------------

void FComboGraphAssetEditor::BindGraphCommands()
{
	GraphEditorCommands->MapAction(
		FGenericCommands::Get().Delete,
		FExecuteAction::CreateSP(this, &FComboGraphAssetEditor::DeleteSelectedNodes),
		FCanExecuteAction::CreateSP(this, &FComboGraphAssetEditor::CanDeleteSelectedNodes)
	);

	GraphEditorCommands->MapAction(
		FGenericCommands::Get().SelectAll,
		FExecuteAction::CreateLambda([this]()
		{
			if (GraphEditor.IsValid())
			{
				GraphEditor->SelectAllNodes();
			}
		})
	);
}

#undef LOCTEXT_NAMESPACE
