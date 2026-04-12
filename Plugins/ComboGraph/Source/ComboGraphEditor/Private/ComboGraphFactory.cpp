// Copyright 2026 Prasanna Keerthivasan. All Rights Reserved.

#include "ComboGraphFactory.h"
#include "ComboGraphAsset.h"

#define LOCTEXT_NAMESPACE "ComboGraphFactory"

UComboGraphFactory::UComboGraphFactory()
{
	SupportedClass  = UComboGraphAsset::StaticClass();
	bCreateNew      = true;   // "New Asset" path — no file import needed
	bEditAfterNew   = true;   // Open visual editor immediately after creation
	bEditorImport   = false;
}

UObject* UComboGraphFactory::FactoryCreateNew(
	UClass* InClass,
	UObject* InParent,
	FName InName,
	EObjectFlags Flags,
	UObject* Context,
	FFeedbackContext* Warn)
{
	// NewObject triggers PostInitProperties on UComboGraphAsset,
	// which creates the EdGraph and spawns the root node automatically
	return NewObject<UComboGraphAsset>(InParent, InClass, InName, Flags);
}

FText UComboGraphFactory::GetDisplayName() const
{
	return LOCTEXT("DisplayName", "Combo Graph Asset");
}

FString UComboGraphFactory::GetDefaultNewAssetName() const
{
	return FString(TEXT("NewComboGraph"));
}

#undef LOCTEXT_NAMESPACE
