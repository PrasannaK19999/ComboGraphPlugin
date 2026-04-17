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

void UComboNode::StartDecayTimer()
{
	if (const USkeletalMeshComponent* Mesh = CachedMesh.Get())
	{
		if (UWorld* World = Mesh->GetWorld())
		{
			World->GetTimerManager().SetTimer(
				DecayTimerHandle,
				this,
				&UComboNode::OnDecayExpired,
				DecayTime,
				false
			);
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

	IComboGraphContract* Graph = GetGraphContract();
	if (Graph && Graph->IsTransitioning())
	{
		UE_LOG(LogComboGraph, Warning, TEXT("Node [%s] Activate() rejected — graph is already transitioning"), *GetName());
		return;
	}

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

	// Reset all transient state — new activation starts clean
	NextComboTag       = FGameplayTag();
	bMontageCompleted  = false;
	bComboWindowOpen   = false;
	bExitStateInFlight = false;
	StopDecayTimer();

	CachedMesh = InMesh;

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

	// Act immediately if the window is already open (notify fired or montage done)
	if (bComboWindowOpen || bMontageCompleted)
	{
		StopDecayTimer();
		OnExitState();
	}
}

void UComboNode::OpenComboWindow()
{
	// Ignore if already open — notify should only fire once per activation
	if (bComboWindowOpen)
	{
		return;
	}

	bComboWindowOpen = true;

	UE_LOG(LogComboGraph, Verbose, TEXT("Node [%s] combo window opened via notify"), *GetName());

	if (NextComboTag.IsValid())
	{
		// Input was already buffered before the window opened — transition now
		StopDecayTimer();
		OnExitState();
	}
	else
	{
		// No input yet — start decay window. Montage keeps playing in the background.
		StartDecayTimer();
	}
}

void UComboNode::CloseComboWindow()
{
	if (!bComboWindowOpen)
	{
		return;
	}

	UE_LOG(LogComboGraph, Verbose, TEXT("Node [%s] combo window closed via notify end"), *GetName());

	// Window closed by the notify state end — if no input arrived, stop decay and end combo
	if (!NextComboTag.IsValid())
	{
		StopDecayTimer();
		bComboWindowOpen = false;
		OnExitState();
	}
	// If input arrived during the window, OnExitState was already called — do nothing
}

void UComboNode::OnMontageCompleted(UAnimMontage* Montage, bool bInterrupted)
{
	if (Montage != ActionMontage)
	{
		return;
	}

	if (bInterrupted)
	{
		// Montage was stopped externally (e.g. next Montage_Play during a rapid transition).
		// Only clean up LOCAL state — do NOT notify the graph. By the time this fires
		// (next anim tick), the graph has already moved on to a new active node.
		// Calling NotifyNodeActivated(nullptr) here would wipe out the live active node.
		UE_LOG(LogComboGraph, Verbose, TEXT("Node [%s] montage interrupted — local cleanup only"), *GetName());
		NextComboTag       = FGameplayTag();
		bMontageCompleted  = false;
		bComboWindowOpen   = false;
		bExitStateInFlight = false;
		StopDecayTimer();
		if (const USkeletalMeshComponent* Mesh = CachedMesh.Get())
		{
			if (UAnimInstance* AnimInstance = Mesh->GetAnimInstance())
			{
				AnimInstance->OnMontageEnded.RemoveDynamic(this, &UComboNode::OnMontageCompleted);
			}
		}
		return;
	}

	bMontageCompleted = true;

	if (bComboWindowOpen)
	{
		// Notify already opened the window — decay timer is running.
		// If input arrived, OnExitState was already called from ReceiveInput or OpenComboWindow.
		// If decay is still running, let it expire naturally. Nothing to do here.
		UE_LOG(LogComboGraph, Verbose, TEXT("Node [%s] montage completed, window already open — decay continues"), *GetName());
		return;
	}

	// Fallback: no notify was placed — open the window now (original behavior)
	if (NextComboTag.IsValid())
	{
		UE_LOG(LogComboGraph, Verbose, TEXT("Node [%s] montage completed, decay=skipped (input buffered)"), *GetName());
		OnExitState();
	}
	else
	{
		UE_LOG(LogComboGraph, Verbose, TEXT("Node [%s] montage completed, decay=started (no notify fallback)"), *GetName());
		StartDecayTimer();
	}
}

void UComboNode::OnDecayExpired()
{
	UE_LOG(LogComboGraph, Verbose, TEXT("Node [%s] decay expired"), *GetName());
	OnExitState();
}

void UComboNode::OnExitState()
{
	// Race condition guard — notify + montage-end can both lead here on the same frame
	if (bExitStateInFlight)
	{
		return;
	}
	bExitStateInFlight = true;

	IComboGraphContract* Graph = GetGraphContract();
	if (!Graph)
	{
		UE_LOG(LogComboGraph, Error, TEXT("Node [%s] GetGraphContract() returned null — graph may have been destroyed"), *GetName());
		return;
	}

	USkeletalMeshComponent* Mesh = CachedMesh.Get();

	if (NextComboTag.IsValid())
	{
		UComboPath* Path = TransitionMap.FindRef(NextComboTag);

		if (Path && Path->TargetNode)
		{
			// OUTCOME 1: Valid transition — combo continues
			UE_LOG(LogComboGraph, Verbose, TEXT("Transition [%s] -> [%s]"), *GetName(), *Path->TargetNode->GetName());

			const FGameplayTag TransitionTag = NextComboTag;
			ResetActivationState();
			Graph->NotifyTransition(TransitionTag);
			Path->TargetNode->Activate(Mesh);
		}
		else
		{
			// OUTCOME 2: Dead end — no matching path
			UE_LOG(LogComboGraph, Verbose, TEXT("Dead end at [%s], tag [%s] has no path"), *GetName(), *NextComboTag.ToString());
			ResetActivationState();
			Graph->NotifyComboEnd();
		}
	}
	else
	{
		// OUTCOME 3: No input — decay expired
		UE_LOG(LogComboGraph, Verbose, TEXT("No input at [%s], resetting to root"), *GetName());
		ResetActivationState();
		Graph->NotifyComboEnd();
	}
}

void UComboNode::ResetActivationState()
{
	NextComboTag       = FGameplayTag();
	bMontageCompleted  = false;
	bComboWindowOpen   = false;
	bExitStateInFlight = false;
	StopDecayTimer();

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
