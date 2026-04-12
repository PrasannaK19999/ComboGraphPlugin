// Copyright 2026 Prasanna Keerthivasan. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Factories/Factory.h"
#include "ComboGraphFactory.generated.h"

/**
 * UComboGraphFactory
 *
 * Registers UComboGraphAsset as a creatable asset type in the Content Browser.
 * Enables: right-click Content Browser -> Miscellaneous -> Combo Graph Asset.
 *
 * Opens the visual editor immediately after creation (bEditAfterNew = true).
 */
UCLASS()
class COMBOGRAPHEDITOR_API UComboGraphFactory : public UFactory
{
	GENERATED_BODY()

public:

	UComboGraphFactory();

	// -----------------------------------------------------------------------
	// UFactory interface
	// -----------------------------------------------------------------------

	/**
	 * Creates a new UComboGraphAsset instance.
	 * PostInitProperties on the asset handles EdGraph initialization
	 * and root node creation — nothing extra needed here.
	 */
	virtual UObject* FactoryCreateNew(
		UClass* InClass,
		UObject* InParent,
		FName InName,
		EObjectFlags Flags,
		UObject* Context,
		FFeedbackContext* Warn
	) override;

	virtual bool ShouldShowInNewMenu() const override { return true; }
	virtual FText GetDisplayName() const override;
	virtual FString GetDefaultNewAssetName() const override;
};
