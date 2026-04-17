// Copyright 2026 Prasanna Keerthivasan. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "ComboWindowListener.generated.h"

UINTERFACE(MinimalAPI, Blueprintable)
class UComboWindowListener : public UInterface
{
	GENERATED_BODY()
};

/**
 * Implement on any ActorComponent to receive combo window notifications
 * fired by UComboWindowNotifyState placed on a montage.
 */
class COMBOGRAPH_API IComboWindowListener
{
	GENERATED_BODY()

public:

	/** Called when the combo window notify state begins — input window opens. */
	UFUNCTION(BlueprintNativeEvent, Category = "Combo")
	void OnComboWindowOpened();

	/** Called when the combo window notify state ends — input window closes. */
	UFUNCTION(BlueprintNativeEvent, Category = "Combo")
	void OnComboWindowClosed();
};
