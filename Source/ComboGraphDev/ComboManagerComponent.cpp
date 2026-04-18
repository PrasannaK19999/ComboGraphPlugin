// Copyright 2026 Prasanna Keerthivasan. All Rights Reserved.

#include "ComboManagerComponent.h"

#include "ComboGraph/ComboGraph.h"
#include "ComboNode/ComboNode.h"
#include "ComboGraphBuilder/ComboGraphBuilder.h"

#include "Animation/AnimSequenceBase.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/DataTable.h"
#include "GameFramework/Actor.h"

UComboManagerComponent::UComboManagerComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UComboManagerComponent::BeginPlay()
{
	Super::BeginPlay();

	// Cache the first skeletal mesh found on the owning actor
	if (AActor* Owner = GetOwner())
	{
		CachedMesh = Owner->FindComponentByClass<USkeletalMeshComponent>();
	}

	RebuildGraphs();
}

// -----------------------------------------------------------------------
// Runtime API
// -----------------------------------------------------------------------

void UComboManagerComponent::OnComboInput(FGameplayTag InputTag)
{
	if (!CachedMesh.IsValid())
	{
		return;
	}

	// Active combo in progress — route tag as continuation input
	if (ActiveGraph != nullptr)
	{
		ActiveGraph->RouteInput(InputTag);
		return;
	}

	// No active combo — look up the chain for this starting input
	TObjectPtr<UComboGraph>* Found = ComboGraphMap.Find(InputTag);
	if (!Found || !(*Found))
	{
		return;
	}

	UComboGraph* Graph = Found->Get();
	UComboNode* Root = Graph->GetRootNode();
	if (!Root)
	{
		return;
	}

	ActiveGraph = Graph;

	// Listen for combo end so we can clean up and allow a fresh start
	Graph->OnComboEnd.RemoveDynamic(this, &UComboManagerComponent::OnActiveComboEnded);
	Graph->OnComboEnd.AddDynamic(this, &UComboManagerComponent::OnActiveComboEnded);

	Root->Activate(CachedMesh.Get());
}

void UComboManagerComponent::OnComboWindowOpened_Implementation()
{
	if (ActiveGraph != nullptr)
	{
		ActiveGraph->NotifyComboWindow();
	}
}

void UComboManagerComponent::OnComboWindowClosed_Implementation(UAnimSequenceBase* Animation)
{
	if (ActiveGraph != nullptr)
	{
		ActiveGraph->NotifyComboWindowClosed(Animation);
	}
}

void UComboManagerComponent::SetWeaponDataTable(UDataTable* NewTable)
{
	// Interrupt any running combo cleanly before swapping
	if (ActiveGraph != nullptr)
	{
		ActiveGraph->NotifyInterruption();
	}

	ComboDataTable = NewTable;
	RebuildGraphs();
}

// -----------------------------------------------------------------------
// Private
// -----------------------------------------------------------------------

void UComboManagerComponent::RebuildGraphs()
{
	// Unbind from old graphs
	for (auto& Pair : ComboGraphMap)
	{
		if (UComboGraph* Graph = Pair.Value.Get())
		{
			Graph->OnComboEnd.RemoveDynamic(this, &UComboManagerComponent::OnActiveComboEnded);
		}
	}

	ComboGraphMap.Empty();
	ActiveGraph = nullptr;

	if (!ComboDataTable)
	{
		return;
	}

	TMap<FGameplayTag, UComboGraph*> Built =
		FComboGraphBuilder::BuildFromDataTable(GetOwner(), ComboDataTable);

	for (auto& Pair : Built)
	{
		ComboGraphMap.Add(Pair.Key, Pair.Value);
	}
}

void UComboManagerComponent::OnActiveComboEnded()
{
	if (UComboGraph* Graph = ActiveGraph)
	{
		// Outcomes 2/3 in OnExitState leave bIsTransitioning=true to block auto-restart.
		// Clear it here so the player can start a fresh combo on next input.
		Graph->SetTransitioning(false);
		Graph->OnComboEnd.RemoveDynamic(this, &UComboManagerComponent::OnActiveComboEnded);
	}

	ActiveGraph = nullptr;
}
