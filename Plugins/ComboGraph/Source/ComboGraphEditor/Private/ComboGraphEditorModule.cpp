// Copyright 2026 Prasanna Keerthivasan. All Rights Reserved.

#include "ComboGraphEditorModule.h"
#include "ComboGraphAssetTypeActions.h"
#include "AssetToolsModule.h"
#include "IAssetTools.h"

#define LOCTEXT_NAMESPACE "FComboGraphEditorModule"

void FComboGraphEditorModule::StartupModule()
{
	IAssetTools& AssetTools = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools").Get();

	ComboGraphAssetTypeActions = MakeShareable(new FComboGraphAssetTypeActions());
	AssetTools.RegisterAssetTypeActions(ComboGraphAssetTypeActions.ToSharedRef());
}

void FComboGraphEditorModule::ShutdownModule()
{
	// Unregister asset type actions if registered
	if (ComboGraphAssetTypeActions.IsValid())
	{
		if (FModuleManager::Get().IsModuleLoaded("AssetTools"))
		{
			IAssetTools& AssetTools = FModuleManager::GetModuleChecked<FAssetToolsModule>("AssetTools").Get();
			AssetTools.UnregisterAssetTypeActions(ComboGraphAssetTypeActions.ToSharedRef());
		}

		ComboGraphAssetTypeActions.Reset();
	}
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FComboGraphEditorModule, ComboGraphEditor)
