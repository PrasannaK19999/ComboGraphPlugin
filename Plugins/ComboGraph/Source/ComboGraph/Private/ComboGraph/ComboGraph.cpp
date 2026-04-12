// Copyright 2026 Prasanna Keerthivasan. All Rights Reserved.

#include "ComboGraph/ComboGraph.h"
#include "ComboNode/ComboNode.h"
#include "ComboGraphModule.h"

UComboNode* UComboGraph::GetRootNode() const
{
	return RootNode;
}

void UComboGraph::NotifyTransition(const FGameplayTag ComboTag)
{
	CurrentDepth++;
	ComboHistory.Add(ComboTag);

	if (CurrentDepth == 1)
	{
		// First transition — combo just started
		OnComboBegin.Broadcast();
	}
	else
	{
		// Subsequent transition — combo continues
		OnComboOnGoing.Broadcast();
	}

	UE_LOG(LogComboGraph, Log, TEXT("Graph [%s] transition [%s] depth=%d"),
		*GetName(), *ComboTag.ToString(), CurrentDepth);
}

void UComboGraph::NotifyComboEnd()
{
	OnComboEnd.Broadcast();

	UE_LOG(LogComboGraph, Log, TEXT("Graph [%s] combo ended at depth %d"),
		*GetName(), CurrentDepth);

	// Reset AFTER logging so the log captures the depth at which the combo ended
	CurrentDepth = 0;
	ComboHistory.Empty();
}

void UComboGraph::NotifyNodeActivated(UComboNode* Node)
{
	ActiveNode = Node;
	UE_LOG(LogComboGraph, Verbose, TEXT("Graph [%s] active node set to [%s]"),
		*GetName(), Node ? *Node->GetName() : TEXT("null"));
}

void UComboGraph::NotifyInterruption()
{
	// v1: all interruptions are hard reset. No EComboInterruptPolicy enum — single behavior.
	UE_LOG(LogComboGraph, Log, TEXT("Graph [%s] interrupted, hard reset"), *GetName());
	NotifyComboEnd();
}

void UComboGraph::InitializeFromNodes(TArray<UComboNode*> Nodes, UComboNode* Root)
{
	AllNodes.Reset();
	AllNodes.Reserve(Nodes.Num());

	for (UComboNode* Node : Nodes)
	{
		if (Node)
		{
			AllNodes.Add(Node);
		}
		else
		{
			UE_LOG(LogComboGraph, Warning, TEXT("Graph [%s] InitializeFromNodes received null node, skipped"),
				*GetName());
		}
	}

	RootNode = Root;

	if (!RootNode)
	{
		UE_LOG(LogComboGraph, Error, TEXT("Graph [%s] initialized with null RootNode"), *GetName());
	}
	else
	{
		UE_LOG(LogComboGraph, Log, TEXT("Graph [%s] initialized with %d nodes, root=[%s]"),
			*GetName(), AllNodes.Num(), *RootNode->GetName());
	}
}

void UComboGraph::RouteInput(FGameplayTag Tag)
{
	if (!ActiveNode.IsValid())
	{
		UE_LOG(LogComboGraph, Warning, TEXT("Graph [%s] RouteInput [%s] — no active node"),
			*GetName(), *Tag.ToString());
		return;
	}

	ActiveNode->ReceiveInput(Tag);
}
