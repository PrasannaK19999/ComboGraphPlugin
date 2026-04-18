// Copyright 2026 Prasanna Keerthivasan. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotifyState.h"
#include "ComboWindowNotify.generated.h"

/**
 * UComboWindowNotifyState
 *
 * Place this notify state on a montage to define the exact frames where combo
 * input is accepted. Drag the bar to set the window duration directly on the timeline.
 *
 * NotifyBegin → opens the input window (decay timer does NOT run during this)
 * NotifyEnd   → closes the input window
 *
 * If no notify state is placed, the system falls back to DecayTime on UComboNode:
 * the window opens after OnMontageEnded fires and stays open for DecayTime seconds.
 */
UCLASS(meta = (DisplayName = "Combo Window"))
class COMBOGRAPH_API UComboWindowNotifyState : public UAnimNotifyState
{
	GENERATED_BODY()

public:

	virtual void NotifyBegin(
		USkeletalMeshComponent* MeshComp,
		UAnimSequenceBase* Animation,
		float TotalDuration,
		const FAnimNotifyEventReference& EventReference
	) override;

	virtual void NotifyEnd(
		USkeletalMeshComponent* MeshComp,
		UAnimSequenceBase* Animation,
		const FAnimNotifyEventReference& EventReference
	) override;

	virtual FString GetNotifyName_Implementation() const override
	{
		return TEXT("Combo Window");
	}

};
