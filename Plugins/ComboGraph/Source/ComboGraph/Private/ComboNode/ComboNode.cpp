// Copyright 2026 Prasanna Keerthivasan. All Rights Reserved.

#include "ComboNode/ComboNode.h"
#include "Interfaces/ComboGraphContract.h"
#include "ComboPath/ComboPath.h"
#include "ComboGraphModule.h"

#include "Components/SkeletalMeshComponent.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "TimerManager.h"
#include "Engine/World.h"

IComboGraphContract* UComboNode::GetGraphContract() const
{
	return Cast<IComboGraphContract>(GetOuter());
}

void UComboNode::StopDecayTimer()
{
	if (const USkeletalMeshComponent* Mesh = CachedMesh.Get())
	{
		if (UWorld* World = Mesh->GetWorld())
		{
			World->GetTimerManager().ClearTimer(DecayTimerHandle);
		}
	}
}

void UComboNode::Activate(USkeletalMeshComponent* InMesh)
{
	if (!InMesh)
	{
		UE_LOG(LogComboGraph, Warning, TEXT("Node [%s] Activate() called with null Mesh"), *GetName());
		return;
	}

	// Double-activation guard — reject if a transition is already in flight
	IComboGraphContract* Graph = GetGraphContract();
	if (Graph && Graph->IsTransitioning())
	{
		UE_LOG(LogComboGraph, Warning, TEXT("Node [%s] Activate() rejected — graph is already transitioning"), *GetName());
		return;
	}

	// Clear the transitioning flag — this node has taken over, transition is complete
	if (Graph)
	{
		Graph->SetTransitioning(false);
	}

	UAnimInstance* AnimInstance = InMesh->GetAnimInstance();
	if (!AnimInstance)
	{
		UE_LOG(LogComboGraph, Error, TEXT("Node [%s] Mesh has no AnimInstance — cannot play montage"), *GetName());
		return;
	}

	if (!ActionMontage)
	{
		UE_LOG(LogComboGraph, Error, TEXT("Node [%s] has no ActionMontage assigned — cannot activate"), *GetName());
		return;
	}

	// Step 1: Reset transient state so no stale data from a previous activation leaks through
	NextComboTag = FGameplayTag();
	bMontageCompleted = false;
	StopDecayTimer();

	// Step 2: Cache the mesh — minimum dependency for montage playback and world/timer access
	CachedMesh = InMesh;

	// Step 3: Play the montage
	const float MontageLength = AnimInstance->Montage_Play(ActionMontage);
	if (MontageLength <= 0.f)
	{
		UE_LOG(LogComboGraph, Warning, TEXT("Node [%s] Montage_Play returned 0 — montage may be invalid"), *GetName());
		return;
	}

	if (MontageSection != NAME_None)
	{
		AnimInstance->Montage_JumpToSection(MontageSection, ActionMontage);
	}

	// Bind to the global OnMontageEnded multicast delegate
	AnimInstance->OnMontageEnded.AddDynamic(this, &UComboNode::OnMontageCompleted);

	if (Graph)
	{
		Graph->NotifyNodeActivated(this);
	}

	UE_LOG(LogComboGraph, Verbose, TEXT("Node [%s] activated"), *GetName());
}

void UComboNode::ReceiveInput(FGameplayTag Tag)
{
	if (NextComboTag.IsValid())
	{
		return;
	}

	NextComboTag = Tag;
	UE_LOG(LogComboGraph, Verbose, TEXT("Node [%s] buffered input [%s]"), *GetName(), *Tag.ToString());

	// If the montage already finished, we're in the decay window — go immediately
	if (bMontageCompleted)
	{
		StopDecayTimer();
		OnExitState();
	}
}

void UComboNode::OnMontageCompleted(UAnimMontage* Montage, bool bInterrupted)
{
	// OnMontageEnded fires for all montages — ignore anything that isn't ours
	if (Montage != ActionMontage)
	{
		return;
	}

	if (bInterrupted)
	{
		// External cancellation (dodge, block, stagger, death).
		// Clean up but do NOT call OnExitState — the interrupting system is in control.
		UE_LOG(LogComboGraph, Verbose, TEXT("Node [%s] interrupted externally"), *GetName());
		ResetActivationState();
		return;
	}

	bMontageCompleted = true;

	if (NextComboTag.IsValid())
	{
		// Input was buffered during montage — skip decay entirely, transition now
		UE_LOG(LogComboGraph, Verbose, TEXT("Node [%s] montage completed, decay=skipped"), *GetName());
		OnExitState();
	}
	else
	{
		// No input yet — start the decay window. Node keeps listening for input.
		UE_LOG(LogComboGraph, Verbose, TEXT("Node [%s] montage completed, decay=started"), *GetName());

		if (const USkeletalMeshComponent* Mesh = CachedMesh.Get())
		{
			if (UWorld* World = Mesh->GetWorld())
			{
				World->GetTimerManager().SetTimer(
					DecayTimerHandle,
					this,
					&UComboNode::OnDecayExpired,
					DecayTime,
					false // fires once — not a repeating timer
				);
			}
		}
	}
}

void UComboNode::OnDecayExpired()
{
	UE_LOG(LogComboGraph, Verbose, TEXT("Node [%s] decay expired"), *GetName());

	// Frame-perfect edge case: input might have arrived between timer firing and this callback.
	// If so, OnExitState will find NextComboTag valid and honor it — input wins on ties.
	OnExitState();
}

void UComboNode::OnExitState()
{
	IComboGraphContract* Graph = GetGraphContract();
	if (!Graph)
	{
		// Safety: graph was destroyed while node was active. Become inert. No crash.
		UE_LOG(LogComboGraph, Error, TEXT("Node [%s] GetGraphContract() returned null — graph may have been destroyed"), *GetName());
		return;
	}

	// Cache mesh before ResetActivationState — Outcomes 2 and 3 need it for root activation.
	// ResetActivationState intentionally preserves CachedMesh for this reason.
	USkeletalMeshComponent* Mesh = CachedMesh.Get();

	if (NextComboTag.IsValid())
	{
		UComboPath* Path = TransitionMap.FindRef(NextComboTag);

		if (Path && Path->TargetNode)
		{
			// OUTCOME 1: Valid transition — combo continues
			UE_LOG(LogComboGraph, Verbose, TEXT("Transition [%s] -> [%s]"), *GetName(), *Path->TargetNode->GetName());

			// Must capture tag before ResetActivationState clears NextComboTag
			const FGameplayTag TransitionTag = NextComboTag;
			ResetActivationState();
			Graph->SetTransitioning(true);
			Graph->NotifyTransition(TransitionTag);
			Path->TargetNode->Activate(Mesh);
		}
		else
		{
			// OUTCOME 2: Tag valid but no matching path — dead end
			// Input is DISCARDED. Does NOT carry forward to root. Player must input fresh.
			UE_LOG(LogComboGraph, Verbose, TEXT("Dead end at [%s], tag [%s] has no path"), *GetName(), *NextComboTag.ToString());
			ResetActivationState();
			Graph->NotifyComboEnd();

			if (UComboNode* RootNode = Graph->GetRootNode())
			{
				Graph->SetTransitioning(true);
				RootNode->Activate(Mesh);
			}
		}
	}
	else
	{
		// OUTCOME 3: No input received — decay expired, reset to root
		UE_LOG(LogComboGraph, Verbose, TEXT("No input at [%s], resetting to root"), *GetName());
		ResetActivationState();
		Graph->NotifyComboEnd();

		if (UComboNode* RootNode = Graph->GetRootNode())
		{
			Graph->SetTransitioning(true);
			RootNode->Activate(Mesh);
		}
	}
}

void UComboNode::ResetActivationState()
{
	NextComboTag = FGameplayTag();
	StopDecayTimer();
	bMontageCompleted = false;

	if (const USkeletalMeshComponent* Mesh = CachedMesh.Get())
	{
		if (UAnimInstance* AnimInstance = Mesh->GetAnimInstance())
		{
			AnimInstance->OnMontageEnded.RemoveDynamic(this, &UComboNode::OnMontageCompleted);
		}
	}

	IComboGraphContract* Graph = GetGraphContract();
	if (Graph)
	{
		Graph->NotifyNodeActivated(nullptr);
	}
}
