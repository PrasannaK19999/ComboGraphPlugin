// Copyright 2026 Prasanna Keerthivasan. All Rights Reserved.

#include "ComboGraphAsset.h"
#include "EdGraph_ComboGraph.h"
#include "EdGraphNode_ComboNode.h"
#include "EdGraphSchema_ComboGraph.h"
#include "ComboGraphDataAsset/ComboGraphDataAsset.h"
#include "StructUtils/InstancedStruct.h"

#include "AssetRegistry/AssetRegistryModule.h"
#include "Logging/MessageLog.h"
#include "Misc/PackageName.h"
#include "UObject/Package.h"
#include "UObject/SavePackage.h"

#define LOCTEXT_NAMESPACE "ComboGraphAsset"

// -----------------------------------------------------------------------
// Auto-layout constants
// -----------------------------------------------------------------------

// Horizontal spacing between tree depths (pixels)
static constexpr float NodeLayoutStepX = 300.f;

// Vertical spacing between siblings at the same depth (pixels)
static constexpr float NodeLayoutStepY = 150.f;

// -----------------------------------------------------------------------
// Constructor
// -----------------------------------------------------------------------

UComboGraphAsset::UComboGraphAsset()
{
}

// -----------------------------------------------------------------------
// UObject interface
// -----------------------------------------------------------------------

void UComboGraphAsset::PostInitProperties()
{
	Super::PostInitProperties();

	// Only initialize on actual asset creation — skip CDO and assets being loaded from disk
	if (HasAnyFlags(RF_ClassDefaultObject | RF_NeedLoad))
	{
		return;
	}

	// Create the editor graph
	EdGraph = NewObject<UEdGraph_ComboGraph>(this, NAME_None, RF_Transactional);
	EdGraph->Schema = UEdGraphSchema_ComboGraph::StaticClass();

	// Auto-spawn the root node — graph is invalid without one
	const UEdGraphSchema_ComboGraph* Schema = GetDefault<UEdGraphSchema_ComboGraph>();
	if (Schema)
	{
		Schema->CreateDefaultNodesForGraph(*EdGraph);
	}
}

void UComboGraphAsset::PostEditUndo()
{
	Super::PostEditUndo();

	// Notify the graph that its visual state may have changed after undo
	if (EdGraph)
	{
		EdGraph->NotifyGraphChanged();
	}
}

// -----------------------------------------------------------------------
// Helpers
// -----------------------------------------------------------------------

UEdGraphNode_ComboNode* UComboGraphAsset::GetRootEdNode() const
{
	if (!EdGraph)
	{
		return nullptr;
	}

	return EdGraph->GetRootNode();
}

// -----------------------------------------------------------------------
// Validation
// -----------------------------------------------------------------------

bool UComboGraphAsset::ValidateGraph() const
{
	FMessageLog CompilerLog(TEXT("ComboGraphCompiler"));
	bool bIsValid = true;

	if (!EdGraph)
	{
		CompilerLog.Error(LOCTEXT("NullGraph", "Graph is null. Reopen the asset to reinitialize."));
		return false;
	}

	TArray<UEdGraphNode_ComboNode*> AllNodes = EdGraph->GetAllComboNodes();

	// --- Check: exactly one root node ---
	int32 RootCount = 0;
	for (const UEdGraphNode_ComboNode* Node : AllNodes)
	{
		if (Node->IsRootNode())
		{
			RootCount++;
		}
	}

	if (RootCount == 0)
	{
		CompilerLog.Error(LOCTEXT("NoRoot", "Graph has no root node. Delete and recreate the graph."));
		bIsValid = false;
	}
	else if (RootCount > 1)
	{
		CompilerLog.Error(LOCTEXT("MultipleRoots", "Graph has more than one root node. Only one root is allowed."));
		bIsValid = false;
	}

	// --- Check: no duplicate tags on same node, no untagged connected pins, montages assigned ---
	for (const UEdGraphNode_ComboNode* Node : AllNodes)
	{
		if (!Node)
		{
			continue;
		}

		// Non-root nodes must have a montage
		if (!Node->IsRootNode() && !Node->GetMontage())
		{
			CompilerLog.Warning(
				FText::Format(
					LOCTEXT("NoMontage", "Node '{0}' has no montage assigned."),
					FText::FromName(Node->GetComboNodeName())
				)
			);
			// Warning only — does not block compilation
		}

		TArray<UEdGraphPin*> OutputPins = Node->GetOutputPins();
		TSet<FGameplayTag> SeenTags;

		for (const UEdGraphPin* Pin : OutputPins)
		{
			if (!Pin)
			{
				continue;
			}

			const bool bIsConnected = Pin->LinkedTo.Num() > 0;
			const FGameplayTag PinTag = Node->GetPinGameplayTag(Pin);

			// Untagged pin that has a connection — will produce an invalid TMap key at runtime
			if (bIsConnected && !PinTag.IsValid())
			{
				CompilerLog.Error(
					FText::Format(
						LOCTEXT("UntaggedPin", "Node '{0}' has a connected output pin with no GameplayTag assigned."),
						FText::FromName(Node->GetComboNodeName())
					)
				);
				bIsValid = false;
			}

			// Duplicate tag on same node — would overwrite a TMap entry at runtime
			if (PinTag.IsValid())
			{
				if (SeenTags.Contains(PinTag))
				{
					CompilerLog.Error(
						FText::Format(
							LOCTEXT("DuplicateTag", "Node '{0}' has duplicate GameplayTag '{1}' on multiple output pins."),
							FText::FromName(Node->GetComboNodeName()),
							FText::FromString(PinTag.ToString())
						)
					);
					bIsValid = false;
				}
				else
				{
					SeenTags.Add(PinTag);
				}
			}
		}
	}

	// --- Check: no cycles (three-color DFS from root) ---
	// Gray  = InStack    — currently on the DFS call stack (back edge = cycle)
	// Black = Processed  — fully validated, all descendants clean
	// White = neither    — not yet visited
	// Nodes reachable from multiple parents (diamond graphs) are skipped once Black,
	// keeping complexity at true O(V+E) instead of O(2^n).
	UEdGraphNode_ComboNode* Root = EdGraph->GetRootNode();
	if (Root)
	{
		TSet<FGuid> InStack;
		TSet<FGuid> Processed;
		TFunction<bool(const UEdGraphNode_ComboNode*)> DetectCycle;

		DetectCycle = [&](const UEdGraphNode_ComboNode* Node) -> bool
		{
			// Black — already fully validated, safe to skip
			if (Processed.Contains(Node->NodeGuid))
			{
				return false;
			}

			// Gray — back edge found, cycle confirmed
			if (InStack.Contains(Node->NodeGuid))
			{
				CompilerLog.Error(
					FText::Format(
						LOCTEXT("CycleDetected", "Cycle detected at node '{0}'. Combo graphs must be acyclic (DAG)."),
						FText::FromName(Node->GetComboNodeName())
					)
				);
				return true;
			}

			// Mark Gray — entering this node
			InStack.Add(Node->NodeGuid);

			for (const UEdGraphPin* Pin : Node->GetOutputPins())
			{
				if (Pin && Pin->LinkedTo.Num() > 0)
				{
					const UEdGraphNode_ComboNode* ChildNode = Cast<UEdGraphNode_ComboNode>(Pin->LinkedTo[0]->GetOwningNode());
					if (ChildNode && DetectCycle(ChildNode))
					{
						return true;
					}
				}
			}

			// Mark Black — all descendants clean
			InStack.Remove(Node->NodeGuid);
			Processed.Add(Node->NodeGuid);
			return false;
		};

		if (DetectCycle(Root))
		{
			bIsValid = false;
		}
	}

	if (bIsValid)
	{
		CompilerLog.Info(LOCTEXT("ValidationPassed", "Combo graph validation passed."));
	}

	return bIsValid;
}

// -----------------------------------------------------------------------
// Compile
// -----------------------------------------------------------------------

bool UComboGraphAsset::CompileToDataAsset()
{
	if (!ValidateGraph())
	{
		return false;
	}

	UEdGraphNode_ComboNode* RootEdNode = GetRootEdNode();
	if (!RootEdNode)
	{
		return false;
	}

	// Auto-create output asset on first compile
	if (!OutputAsset)
	{
		OutputAsset = CreateOrFindOutputAsset();
		if (!OutputAsset)
		{
			FMessageLog(TEXT("ComboGraphCompiler")).Error(
				LOCTEXT("OutputAssetFailed", "Failed to create output UComboGraphDataAsset. Check Content Browser permissions.")
			);
			return false;
		}
	}

	// DFS compile — builds the full FComboNodeData tree from the EdGraph
	FComboNodeData CompiledRoot;
	TSet<FGuid> VisitedNodes;

	if (!CompileNodeRecursive(RootEdNode, CompiledRoot, VisitedNodes))
	{
		return false;
	}

	// Overwrite the runtime asset's root node in place
	OutputAsset->Modify();
	OutputAsset->RootNode = CompiledRoot;
	OutputAsset->MarkPackageDirty();

	MarkPackageDirty();

	FMessageLog(TEXT("ComboGraphCompiler")).Info(
		FText::Format(
			LOCTEXT("CompileSuccess", "Compiled '{0}' -> '{1}' successfully."),
			FText::FromString(GetName()),
			FText::FromString(OutputAsset->GetName())
		)
	);

	return true;
}

bool UComboGraphAsset::CompileNodeRecursive(
	const UEdGraphNode_ComboNode* EdNode,
	FComboNodeData& OutNodeData,
	TSet<FGuid>& VisitedNodes) const
{
	if (!EdNode)
	{
		return false;
	}

	// Cycle guard — should not trigger if ValidateGraph() passed, but defensive
	if (VisitedNodes.Contains(EdNode->NodeGuid))
	{
		return false;
	}
	VisitedNodes.Add(EdNode->NodeGuid);

	// Populate this node's data from the EdNode properties
	OutNodeData.DisplayName    = EdNode->GetComboNodeName();
	OutNodeData.Montage        = EdNode->GetMontage();
	OutNodeData.MontageSection = EdNode->GetMontageSection();
	OutNodeData.DecayTime      = EdNode->GetDecayTime();
	OutNodeData.Transitions.Empty();

	// Walk each output pin — each connected pin becomes a child FComboNodeData
	for (const UEdGraphPin* Pin : EdNode->GetOutputPins())
	{
		if (!Pin || Pin->LinkedTo.Num() == 0)
		{
			// Unconnected output pin — skip (not an error, designer may leave pins dangling)
			continue;
		}

		const FGameplayTag PinTag = EdNode->GetPinGameplayTag(Pin);
		if (!PinTag.IsValid())
		{
			// Untagged connected pin — ValidateGraph() blocks this, but guard defensively
			continue;
		}

		const UEdGraphNode_ComboNode* ChildEdNode = Cast<UEdGraphNode_ComboNode>(Pin->LinkedTo[0]->GetOwningNode());
		if (!ChildEdNode)
		{
			continue;
		}

		FComboNodeData ChildNodeData;
		if (!CompileNodeRecursive(ChildEdNode, ChildNodeData, VisitedNodes))
		{
			return false;
		}

		// Wrap child data in FInstancedStruct — matches FComboNodeData::Transitions value type
		FInstancedStruct ChildStruct;
		ChildStruct.InitializeAs<FComboNodeData>(ChildNodeData);

		OutNodeData.Transitions.Add(PinTag, MoveTemp(ChildStruct));
	}

	VisitedNodes.Remove(EdNode->NodeGuid);

	return true;
}

// -----------------------------------------------------------------------
// Import
// -----------------------------------------------------------------------

bool UComboGraphAsset::ImportFromDataAsset(const UComboGraphDataAsset* Source)
{
	if (!Source)
	{
		return false;
	}

	if (!EdGraph)
	{
		return false;
	}

	EdGraph->Modify();

	// Clear all existing nodes — RemoveNode handles BreakAllNodeLinks internally
	// and properly unregisters each node from the graph.
	// Iterate a copy — RemoveNode modifies EdGraph->Nodes in place.
	TArray<UEdGraphNode*> NodesCopy = EdGraph->Nodes;
	for (UEdGraphNode* Node : NodesCopy)
	{
		if (Node)
		{
			EdGraph->RemoveNode(Node);
		}
	}

	// Reconstruct from the FComboNodeData tree — root node first
	ImportNodeRecursive(Source->RootNode, nullptr, FGameplayTag(), 0, 0);

	EdGraph->NotifyGraphChanged();

	return true;
}

void UComboGraphAsset::ImportNodeRecursive(
	const FComboNodeData& NodeData,
	UEdGraphNode_ComboNode* ParentEdNode,
	const FGameplayTag& ParentPinTag,
	int32 Depth,
	int32 SiblingIndex)
{
	// Auto-layout: left-to-right by depth, top-to-bottom by sibling index
	const FVector2D Position(Depth * NodeLayoutStepX, SiblingIndex * NodeLayoutStepY);

	UEdGraphNode_ComboNode* NewEdNode = UEdGraphSchema_ComboGraph::CreateComboNode(EdGraph, Position, false);
	if (!NewEdNode)
	{
		return;
	}

	// Populate authored data
	NewEdNode->SetComboNodeName(NodeData.DisplayName);
	NewEdNode->SetMontage(NodeData.Montage);
	NewEdNode->SetMontageSection(NodeData.MontageSection);
	NewEdNode->SetDecayTime(NodeData.DecayTime);

	// Root node: mark as root, no input pin
	if (!ParentEdNode)
	{
		NewEdNode->SetIsRootNode(true);
	}

	// Connect to parent via the parent's output pin for this tag
	if (ParentEdNode && ParentPinTag.IsValid())
	{
		UEdGraphPin* ParentOutputPin = ParentEdNode->CreateTransitionPin(ParentPinTag);
		UEdGraphPin* InputPin = NewEdNode->GetInputPin();

		if (ParentOutputPin && InputPin && EdGraph->GetSchema())
		{
			EdGraph->GetSchema()->TryCreateConnection(ParentOutputPin, InputPin);
		}
	}

	// Recurse into children
	int32 ChildIndex = 0;
	for (const auto& Transition : NodeData.Transitions)
	{
		const FGameplayTag& ChildTag = Transition.Key;
		const FComboNodeData* ChildData = Transition.Value.GetPtr<FComboNodeData>();

		if (ChildData)
		{
			ImportNodeRecursive(*ChildData, NewEdNode, ChildTag, Depth + 1, ChildIndex);
			ChildIndex++;
		}
	}
}

// -----------------------------------------------------------------------
// Output asset creation
// -----------------------------------------------------------------------

UComboGraphDataAsset* UComboGraphAsset::CreateOrFindOutputAsset()
{
	// Derive path from this asset: same folder, name = "<ThisAsset>_Data"
	const FString AssetPath    = GetPathName();
	const FString PackagePath  = FPackageName::GetLongPackagePath(AssetPath);
	const FString OutputName   = GetName() + TEXT("_Data");
	const FString FullPath     = PackagePath / OutputName;

	// Return existing asset if it's already there (e.g. reimport scenario)
	if (UComboGraphDataAsset* Existing = FindObject<UComboGraphDataAsset>(nullptr, *FullPath))
	{
		return Existing;
	}

	// Create a new package and asset
	UPackage* NewPackage = CreatePackage(*FullPath);
	if (!NewPackage)
	{
		return nullptr;
	}

	UComboGraphDataAsset* NewAsset = NewObject<UComboGraphDataAsset>(
		NewPackage,
		*OutputName,
		RF_Public | RF_Standalone | RF_Transactional
	);

	if (NewAsset)
	{
		FAssetRegistryModule::AssetCreated(NewAsset);
		NewAsset->MarkPackageDirty();
	}

	return NewAsset;
}

#undef LOCTEXT_NAMESPACE
