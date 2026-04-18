// Copyright 2026 Prasanna Keerthivasan. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "ComboWindowListener.generated.h"

class UAnimSequenceBase;

UINTERFACE(MinimalAPI, Blueprintable)
class UComboWindowListener : public UInterface
{
	GENERATED_BODY()
};

class COMBOGRAPH_API IComboWindowListener
{
	GENERATED_BODY()

public:

	UFUNCTION(BlueprintNativeEvent, Category = "Combo")
	void OnComboWindowOpened();

	// Animation identifies which montage fired the End event — used to discard stale closes
	UFUNCTION(BlueprintNativeEvent, Category = "Combo")
	void OnComboWindowClosed(UAnimSequenceBase* Animation);
};
