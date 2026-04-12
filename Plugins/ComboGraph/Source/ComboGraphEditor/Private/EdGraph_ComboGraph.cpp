// Copyright 2026 Prasanna Keerthivasan. All Rights Reserved.

#include "EdGraph_ComboGraph.h"
#include "EdGraphNode_ComboNode.h"
#include "ComboGraphAsset.h"

UEdGraphNode_ComboNode* UEdGraph_ComboGraph::GetRootNode() const
{
	for (UEdGraphNode* Node : Nodes)
	{
		if (UEdGraphNode_ComboNode* ComboNode = Cast<UEdGraphNode_ComboNode>(Node))
		{
			if (ComboNode->IsRootNode())
			{
				return ComboNode;
			}
		}
	}

	return nullptr;
}

TArray<UEdGraphNode_ComboNode*> UEdGraph_ComboGraph::GetAllComboNodes() const
{
	TArray<UEdGraphNode_ComboNode*> Result;
	Result.Reserve(Nodes.Num());

	for (UEdGraphNode* Node : Nodes)
	{
		if (UEdGraphNode_ComboNode* ComboNode = Cast<UEdGraphNode_ComboNode>(Node))
		{
			Result.Add(ComboNode);
		}
	}

	return Result;
}

UComboGraphAsset* UEdGraph_ComboGraph::GetOwnerAsset() const
{
	return Cast<UComboGraphAsset>(GetOuter());
}

void UEdGraph_ComboGraph::NotifyGraphChanged()
{
	Super::NotifyGraphChanged();

	// Mark the owning asset dirty — user will be prompted to save
	if (UComboGraphAsset* Asset = GetOwnerAsset())
	{
		Asset->MarkPackageDirty();
	}
}

void UEdGraph_ComboGraph::NotifyGraphChanged(const FEdGraphEditAction& Action)
{
	Super::NotifyGraphChanged(Action);

	if (UComboGraphAsset* Asset = GetOwnerAsset())
	{
		Asset->MarkPackageDirty();
	}
}
