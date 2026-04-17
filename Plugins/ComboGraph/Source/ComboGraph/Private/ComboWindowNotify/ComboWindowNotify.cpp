// Copyright 2026 Prasanna Keerthivasan. All Rights Reserved.

#include "ComboWindowNotify/ComboWindowNotify.h"
#include "Interfaces/ComboWindowListener.h"

#include "Components/ActorComponent.h"
#include "GameFramework/Actor.h"

void UComboWindowNotifyState::NotifyBegin(
	USkeletalMeshComponent* MeshComp,
	UAnimSequenceBase* Animation,
	float TotalDuration,
	const FAnimNotifyEventReference& EventReference)
{
	BroadcastToListener(MeshComp, true);
}

void UComboWindowNotifyState::NotifyEnd(
	USkeletalMeshComponent* MeshComp,
	UAnimSequenceBase* Animation,
	const FAnimNotifyEventReference& EventReference)
{
	BroadcastToListener(MeshComp, false);
}

void UComboWindowNotifyState::BroadcastToListener(USkeletalMeshComponent* MeshComp, bool bOpen)
{
	if (!MeshComp)
	{
		return;
	}

	AActor* Owner = MeshComp->GetOwner();
	if (!Owner)
	{
		return;
	}

	for (UActorComponent* Component : Owner->GetComponents())
	{
		if (Component && Component->Implements<UComboWindowListener>())
		{
			if (bOpen)
			{
				IComboWindowListener::Execute_OnComboWindowOpened(Component);
			}
			else
			{
				IComboWindowListener::Execute_OnComboWindowClosed(Component);
			}
			return;
		}
	}
}
