// Copyright 2026 Prasanna Keerthivasan. All Rights Reserved.

#include "ComboWindowNotify/ComboWindowNotify.h"
#include "Interfaces/ComboWindowListener.h"

#include "Components/ActorComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/Actor.h"

void UComboWindowNotifyState::NotifyBegin(
	USkeletalMeshComponent* MeshComp,
	UAnimSequenceBase* Animation,
	float TotalDuration,
	const FAnimNotifyEventReference& EventReference)
{
	if (!MeshComp) return;
	AActor* Owner = MeshComp->GetOwner();
	if (!Owner) return;

	for (UActorComponent* Component : Owner->GetComponents())
	{
		if (Component && Component->Implements<UComboWindowListener>())
		{
			IComboWindowListener::Execute_OnComboWindowOpened(Component);
			return;
		}
	}
}

void UComboWindowNotifyState::NotifyEnd(
	USkeletalMeshComponent* MeshComp,
	UAnimSequenceBase* Animation,
	const FAnimNotifyEventReference& EventReference)
{
	if (!MeshComp) return;
	AActor* Owner = MeshComp->GetOwner();
	if (!Owner) return;

	for (UActorComponent* Component : Owner->GetComponents())
	{
		if (Component && Component->Implements<UComboWindowListener>())
		{
			// Pass Animation so the receiver can verify this End belongs to the correct montage
			IComboWindowListener::Execute_OnComboWindowClosed(Component, Animation);
			return;
		}
	}
}
