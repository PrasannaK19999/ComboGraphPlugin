// Copyright 2026 Prasanna Keerthivasan. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "GameplayTagContainer.h"
#include "ComboGraphContract.generated.h"

class UComboNode;

UINTERFACE()
class UComboGraphContract : public UInterface
{
	GENERATED_BODY()
};


class COMBOGRAPH_API IComboGraphContract
{
	GENERATED_BODY()

public:

	virtual UComboNode* GetRootNode() const = 0;

	virtual void NotifyTransition(const FGameplayTag ComboTag) = 0;

	virtual void NotifyComboEnd() = 0;

	virtual void NotifyComboInterruption() = 0;

	virtual void NotifyNodeActivated(UComboNode* Node) = 0;
};
