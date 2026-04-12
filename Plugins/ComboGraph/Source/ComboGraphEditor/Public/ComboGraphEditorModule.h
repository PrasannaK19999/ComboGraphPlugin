// Copyright 2026 Prasanna Keerthivasan. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleInterface.h"
#include "Modules/ModuleManager.h"

class IAssetTypeActions;

/**
 * ComboGraphEditor module — Pro-only editor extension.
 *
 * Responsibilities:
 *   - Registers UComboGraphAsset as a custom asset type in the Content Browser
 *   - Registers the visual graph editor window for UComboGraphAsset
 *   - Unregisters all of the above on shutdown
 *
 * This module has zero visibility to the runtime module.
 * The runtime module (ComboGraph) compiles identically whether this module
 * is present or not.
 */
class FComboGraphEditorModule : public IModuleInterface
{
public:

	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

private:

	TSharedPtr<IAssetTypeActions> ComboGraphAssetTypeActions;
};
