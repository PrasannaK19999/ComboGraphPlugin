#include "ComboGraphBuilder/ComboGraphBuilder.h"
#include "ComboGraphDataAsset/ComboGraphDataAsset.h"
#include "ComboGraph/ComboGraph.h"
#include "ComboNode/ComboNode.h"
#include "ComboPath/ComboPath.h"
#include "ComboGraphModule.h"
#include "StructUtils/InstancedStruct.h"

UComboGraph* FComboGraphBuilder::BuildFromDataAsset(UObject* Outer, const UComboGraphDataAsset* Asset)
{
	if (!Outer)
	{
		UE_LOG(LogComboGraph, Error, TEXT("Builder: null Outer"));
		return nullptr;
	}

	if (!Asset)
	{
		UE_LOG(LogComboGraph, Error, TEXT("Builder: null DataAsset"));
		return nullptr;
	}

	UComboGraph* Graph = NewObject<UComboGraph>(Outer);

	TArray<UComboNode*> AllNodes;

	// Walk the nested tree starting from the root — returns the root runtime node
	UComboNode* RootNode = BuildNodeRecursive(Graph, Asset->RootNode, AllNodes);

	if (!RootNode)
	{
		UE_LOG(LogComboGraph, Error, TEXT("Builder: failed to create root node from asset [%s]"),
			*Asset->GetName());
		return nullptr;
	}

	Graph->InitializeFromNodes(AllNodes, RootNode);

	UE_LOG(LogComboGraph, Log, TEXT("Builder: graph [%s] from asset [%s] — %d nodes, root=[%s]"),
		*Graph->GetName(), *Asset->GetName(), AllNodes.Num(), *RootNode->GetName());

	return Graph;
}

TMap<FGameplayTag, UComboGraph*> FComboGraphBuilder::BuildFromDataTable(UObject* Outer, const UDataTable* DataTable)
{
	TMap<FGameplayTag, UComboGraph*> Result;

	if (!Outer)
	{
		UE_LOG(LogComboGraph, Error, TEXT("BuildFromDataTable: null Outer"));
		return Result;
	}

	if (!DataTable)
	{
		UE_LOG(LogComboGraph, Error, TEXT("BuildFromDataTable: null DataTable"));
		return Result;
	}

	if (DataTable->GetRowStruct() != FComboGraphTableRow::StaticStruct())
	{
		UE_LOG(LogComboGraph, Error, TEXT("BuildFromDataTable: DataTable [%s] does not use FComboGraphTableRow"),
			*DataTable->GetName());
		return Result;
	}

	const TMap<FName, uint8*>& RowMap = DataTable->GetRowMap();

	for (const auto& Pair : RowMap)
	{
		const FComboGraphTableRow* Row = reinterpret_cast<const FComboGraphTableRow*>(Pair.Value);

		if (!Row)
		{
			UE_LOG(LogComboGraph, Warning, TEXT("BuildFromDataTable: null row at [%s], skipped"),
				*Pair.Key.ToString());
			continue;
		}

		if (!Row->InputTag.IsValid())
		{
			UE_LOG(LogComboGraph, Warning, TEXT("BuildFromDataTable: row [%s] has invalid InputTag, skipped"),
				*Pair.Key.ToString());
			continue;
		}

		if (!Row->ComboChain)
		{
			UE_LOG(LogComboGraph, Warning, TEXT("BuildFromDataTable: row [%s] has null ComboChain, skipped"),
				*Pair.Key.ToString());
			continue;
		}

		UComboGraph* Graph = BuildFromDataAsset(Outer, Row->ComboChain);

		if (Graph)
		{
			Result.Add(Row->InputTag, Graph);
			UE_LOG(LogComboGraph, Log, TEXT("BuildFromDataTable: [%s] -> graph [%s]"),
				*Row->InputTag.ToString(), *Graph->GetName());
		}
	}

	UE_LOG(LogComboGraph, Log, TEXT("BuildFromDataTable: built %d graphs from DataTable [%s]"),
		Result.Num(), *DataTable->GetName());

	return Result;
}

UComboNode* FComboGraphBuilder::BuildNodeRecursive(UComboGraph* Graph, const FComboNodeData& NodeData, TArray<UComboNode*>& OutAllNodes)
{
	UComboNode* Node = NewObject<UComboNode>(Graph, UComboNode::StaticClass(), NodeData.DisplayName);

	Node->ActionMontage = NodeData.Montage;
	Node->MontageSection = NodeData.MontageSection;
	Node->DecayTime = NodeData.DecayTime;

	OutAllNodes.Add(Node);

	UE_LOG(LogComboGraph, Verbose, TEXT("Builder: created node [%s]"), *NodeData.DisplayName.ToString());

	for (const auto& Transition : NodeData.Transitions)
	{
		const FGameplayTag& Tag = Transition.Key;
		const FInstancedStruct& ChildStruct = Transition.Value;

		// Extract the nested FComboNodeData from the FInstancedStruct
		const FComboNodeData* ChildData = ChildStruct.GetPtr<FComboNodeData>();

		if (!ChildData)
		{
			UE_LOG(LogComboGraph, Warning, TEXT("Builder: node [%s] transition [%s] has invalid struct, skipped"),
				*NodeData.DisplayName.ToString(), *Tag.ToString());
			continue;
		}

		// Recurse — create the child node and its entire subtree
		UComboNode* ChildNode = BuildNodeRecursive(Graph, *ChildData, OutAllNodes);

		if (!ChildNode)
		{
			UE_LOG(LogComboGraph, Warning, TEXT("Builder: node [%s] transition [%s] child creation failed, skipped"),
				*NodeData.DisplayName.ToString(), *Tag.ToString());
			continue;
		}

		// Wire the connection — graph owns the path
		UComboPath* Path = NewObject<UComboPath>(Graph);
		Path->TargetNode = ChildNode;
		Path->TransitionName = ChildData->DisplayName;

		Node->TransitionMap.Add(Tag, Path);

		UE_LOG(LogComboGraph, Verbose, TEXT("Builder: [%s] —(%s)-> [%s]"),
			*NodeData.DisplayName.ToString(), *Tag.ToString(), *ChildData->DisplayName.ToString());
	}

	return Node;
}
