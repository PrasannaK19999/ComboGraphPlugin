// Copyright 2026 Prasanna Keerthivasan. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AssetTypeActions_Base.h"

/**
 * FComboGraphAssetTypeActions
 *
 * Registers UComboGraphAsset with the editor's asset system. Controls:
 *   - Display name and color swatch in the Content Browser
 *   - Which "New Asset" submenu category it appears under
 *   - What happens on double-click (opens FComboGraphAssetEditor)
 *   - Right-click asset actions: Compile, Import from DataAsset
 */
class COMBOGRAPHEDITOR_API FComboGraphAssetTypeActions : public FAssetTypeActions_Base
{
public:

	// -----------------------------------------------------------------------
	// FAssetTypeActions_Base interface
	// -----------------------------------------------------------------------

	virtual FText GetName() const override;
	virtual FColor GetTypeColor() const override;
	virtual UClass* GetSupportedClass() const override;
	virtual uint32 GetCategories() override;

	/**
	 * Opens the visual combo graph editor for the selected asset(s).
	 * Spawns FComboGraphAssetEditor as a standalone editor tab.
	 * If multiple assets are selected, opens one editor per asset.
	 */
	virtual void OpenAssetEditor(
		const TArray<UObject*>& InObjects,
		TSharedPtr<IToolkitHost> EditWithinLevelEditor = TSharedPtr<IToolkitHost>()
	) override;

	/** Enables right-click actions on UComboGraphAsset entries in Content Browser. */
	virtual bool HasActions(const TArray<UObject*>& InObjects) const override { return true; }

	/**
	 * Populates the right-click context menu for UComboGraphAsset in Content Browser.
	 * Actions:
	 *   - Compile   — runs UComboGraphAsset::CompileToDataAsset()
	 *   - Import    — opens a picker to import from an existing UComboGraphDataAsset
	 */
	virtual void GetActions(
		const TArray<UObject*>& InObjects,
		FToolMenuSection& Section
	) override;

private:

	/** Compiles all selected combo graph assets. */
	void ExecuteCompile(TArray<TWeakObjectPtr<class UComboGraphAsset>> Assets);

	/** Opens an asset picker for importing from a UComboGraphDataAsset. */
	void ExecuteImport(TWeakObjectPtr<class UComboGraphAsset> Asset);
};
