// Copyright 2026 Prasanna Keerthivasan. All Rights Reserved.

#include "ComboGraphAssetTypeActions.h"
#include "ComboGraphAsset.h"
#include "ComboGraphAssetEditor.h"
#include "ComboGraphDataAsset/ComboGraphDataAsset.h"

#include "AssetRegistry/AssetRegistryModule.h"
#include "AssetToolsModule.h"
#include "ContentBrowserModule.h"
#include "IContentBrowserSingleton.h"
#include "ScopedTransaction.h"
#include "ToolMenuSection.h"
#include "Toolkits/IToolkitHost.h"
#include "Toolkits/ToolkitManager.h"

#define LOCTEXT_NAMESPACE "ComboGraphAssetTypeActions"

// -----------------------------------------------------------------------
// FAssetTypeActions_Base interface
// -----------------------------------------------------------------------

FText FComboGraphAssetTypeActions::GetName() const
{
	return LOCTEXT("AssetName", "Combo Graph Asset");
}

FColor FComboGraphAssetTypeActions::GetTypeColor() const
{
	// Deep amber — matches the root node title color in the visual editor
	return FColor(220, 140, 20);
}

UClass* FComboGraphAssetTypeActions::GetSupportedClass() const
{
	return UComboGraphAsset::StaticClass();
}

uint32 FComboGraphAssetTypeActions::GetCategories()
{
	return EAssetTypeCategories::Misc;
}

void FComboGraphAssetTypeActions::OpenAssetEditor(
	const TArray<UObject*>& InObjects,
	TSharedPtr<IToolkitHost> EditWithinLevelEditor)
{
	const EToolkitMode::Type Mode = EditWithinLevelEditor.IsValid()
		? EToolkitMode::WorldCentric
		: EToolkitMode::Standalone;

	for (UObject* Obj : InObjects)
	{
		UComboGraphAsset* Asset = Cast<UComboGraphAsset>(Obj);
		if (!Asset)
		{
			continue;
		}

		// Reuse an existing editor if one is already open for this asset
		TSharedPtr<IToolkit> ExistingEditor = FToolkitManager::Get().FindEditorForAsset(Asset);
		if (ExistingEditor.IsValid())
		{
			ExistingEditor->BringToolkitToFront();
			continue;
		}

		// Spawn a new editor instance
		TSharedRef<FComboGraphAssetEditor> NewEditor = MakeShareable(new FComboGraphAssetEditor());
		NewEditor->InitComboGraphEditor(Mode, EditWithinLevelEditor, Asset);
	}
}

// -----------------------------------------------------------------------
// Right-click actions
// -----------------------------------------------------------------------

void FComboGraphAssetTypeActions::GetActions(
	const TArray<UObject*>& InObjects,
	FToolMenuSection& Section)
{
	TArray<TWeakObjectPtr<UComboGraphAsset>> Assets;
	for (UObject* Obj : InObjects)
	{
		if (UComboGraphAsset* Asset = Cast<UComboGraphAsset>(Obj))
		{
			Assets.Add(Asset);
		}
	}

	if (Assets.IsEmpty())
	{
		return;
	}

	Section.AddMenuEntry(
		"ComboGraph_Compile",
		LOCTEXT("Compile", "Compile"),
		LOCTEXT("CompileTooltip", "Compiles the visual graph into the linked UComboGraphDataAsset."),
		FSlateIcon(),
		FUIAction(FExecuteAction::CreateSP(this, &FComboGraphAssetTypeActions::ExecuteCompile, Assets))
	);

	// Import is only offered for single-asset selection — multi-import would be ambiguous
	if (Assets.Num() == 1)
	{
		Section.AddMenuEntry(
			"ComboGraph_Import",
			LOCTEXT("Import", "Import from DataAsset"),
			LOCTEXT("ImportTooltip", "Reconstructs the visual graph from an existing UComboGraphDataAsset. Node positions will be reset to auto-layout."),
			FSlateIcon(),
			FUIAction(FExecuteAction::CreateSP(this, &FComboGraphAssetTypeActions::ExecuteImport, Assets[0]))
		);
	}
}

// -----------------------------------------------------------------------
// Private action implementations
// -----------------------------------------------------------------------

void FComboGraphAssetTypeActions::ExecuteCompile(TArray<TWeakObjectPtr<UComboGraphAsset>> Assets)
{
	for (const TWeakObjectPtr<UComboGraphAsset>& WeakAsset : Assets)
	{
		UComboGraphAsset* Asset = WeakAsset.Get();
		if (!Asset)
		{
			continue;
		}

		Asset->CompileToDataAsset();
	}
}

void FComboGraphAssetTypeActions::ExecuteImport(TWeakObjectPtr<UComboGraphAsset> WeakAsset)
{
	UComboGraphAsset* Asset = WeakAsset.Get();
	if (!Asset)
	{
		return;
	}

	// Open an asset picker scoped to UComboGraphDataAsset only
	FOpenAssetDialogConfig PickerConfig;
	PickerConfig.DialogTitleOverride  = LOCTEXT("ImportPickerTitle", "Select a Combo Graph Data Asset to import from");
	PickerConfig.bAllowMultipleSelection = false;
	PickerConfig.AssetClassNames.Add(UComboGraphDataAsset::StaticClass()->GetClassPathName());

	FContentBrowserModule& ContentBrowserModule =
		FModuleManager::LoadModuleChecked<FContentBrowserModule>("ContentBrowser");

	ContentBrowserModule.Get().CreateOpenAssetDialog(
		PickerConfig,
		FOnAssetsChosenForOpen::CreateLambda([WeakAsset](const TArray<FAssetData>& Chosen)
		{
			UComboGraphAsset* InnerAsset = WeakAsset.Get();
			if (!InnerAsset || Chosen.IsEmpty())
			{
				return;
			}

			UComboGraphDataAsset* Source = Cast<UComboGraphDataAsset>(Chosen[0].GetAsset());
			if (Source)
			{
				const FScopedTransaction Transaction(LOCTEXT("ImportTransaction", "Import Combo Graph from DataAsset"));
				InnerAsset->Modify();
				InnerAsset->ImportFromDataAsset(Source);
			}
		}),
		FOnAssetDialogCancelled::CreateLambda([]() {})
	);
}

#undef LOCTEXT_NAMESPACE
