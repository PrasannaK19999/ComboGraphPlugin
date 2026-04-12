// Copyright 2026 Prasanna Keerthivasan. All Rights Reserved.

#include "EdGraphSchema_ComboGraph.h"
#include "EdGraphNode_ComboNode.h"
#include "EdGraph_ComboGraph.h"
#include "EdGraph/EdGraphPin.h"
#include "Framework/Commands/GenericCommands.h"
#include "ToolMenu.h"
#include "GraphEditorActions.h"

#define LOCTEXT_NAMESPACE "ComboGraphSchema"

// Pin category shared between input and output pins — only same-category pins connect
static const FName ComboTransitionCategory(TEXT("ComboTransition"));

// -----------------------------------------------------------------------
// FComboGraphSchemaAction_AddNode
// -----------------------------------------------------------------------

UEdGraphNode* FComboGraphSchemaAction_AddNode::PerformAction(
	UEdGraph* ParentGraph,
	UEdGraphPin* FromPin,
	const FVector2D Location,
	bool bSelectNewNode)
{
	UEdGraphNode_ComboNode* NewNode = UEdGraphSchema_ComboGraph::CreateComboNode(ParentGraph, Location, bSelectNewNode);

	// If dragged from an output pin, auto-wire the connection
	if (NewNode && FromPin)
	{
		NewNode->AutowireNewNode(FromPin);
	}

	return NewNode;
}

// -----------------------------------------------------------------------
// UEdGraphSchema_ComboGraph
// -----------------------------------------------------------------------

void UEdGraphSchema_ComboGraph::GetGraphContextActions(FGraphContextMenuBuilder& ContextMenuBuilder) const
{
	// "Add Combo Node" — available on right-click anywhere in empty graph space
	TSharedPtr<FComboGraphSchemaAction_AddNode> AddNodeAction(
		new FComboGraphSchemaAction_AddNode(
			LOCTEXT("ComboNodeCategory", "Combo Graph"),
			LOCTEXT("AddComboNode", "Add Combo Node"),
			LOCTEXT("AddComboNodeTooltip", "Creates a new combo node. Connect it to a parent node's output pin to define a transition."),
			0
		)
	);

	ContextMenuBuilder.AddAction(AddNodeAction);
}

void UEdGraphSchema_ComboGraph::GetContextMenuActions(UToolMenu* Menu, UGraphNodeContextMenuContext* Context) const
{
	if (!Menu || !Context)
	{
		return;
	}

	// Node-level context actions
	if (Context->Node)
	{
		const UEdGraphNode_ComboNode* ComboNode = Cast<UEdGraphNode_ComboNode>(Context->Node);
		if (ComboNode && !ComboNode->IsRootNode())
		{
			FToolMenuSection& Section = Menu->AddSection(
				TEXT("ComboGraphNodeActions"),
				LOCTEXT("NodeActionsHeader", "Combo Node")
			);

			// Add a new transition output pin to this node
			Section.AddMenuEntry(
				TEXT("AddTransitionPin"),
				LOCTEXT("AddTransitionPin", "Add Transition Pin"),
				LOCTEXT("AddTransitionPinTooltip", "Adds a new output pin to this node for a new combo transition."),
				FSlateIcon(),
				FUIAction(
					FExecuteAction::CreateLambda([WeakNode = MakeWeakObjectPtr(const_cast<UEdGraphNode_ComboNode*>(ComboNode))]()
					{
						if (UEdGraphNode_ComboNode* Node = WeakNode.Get())
						{
							Node->CreateTransitionPin();
							Node->GetGraph()->NotifyGraphChanged();
						}
					})
				)
			);
		}
	}

	// Pin-level context actions
	if (Context->Pin)
	{
		const UEdGraphPin* Pin = Context->Pin;
		if (Pin->Direction == EGPD_Output)
		{
			UEdGraphNode_ComboNode* ComboNode = Cast<UEdGraphNode_ComboNode>(const_cast<UEdGraphPin*>(Pin)->GetOwningNode());
			if (ComboNode)
			{
				FToolMenuSection& PinSection = Menu->AddSection(
					TEXT("ComboGraphPinActions"),
					LOCTEXT("PinActionsHeader", "Transition Pin")
				);

				// Remove this output pin
				PinSection.AddMenuEntry(
					TEXT("RemoveTransitionPin"),
					LOCTEXT("RemovePin", "Remove Transition Pin"),
					LOCTEXT("RemovePinTooltip", "Removes this transition pin and breaks its connection."),
					FSlateIcon(),
					FUIAction(
						FExecuteAction::CreateLambda([WeakNode = MakeWeakObjectPtr(ComboNode), PinId = Pin->PinId]()
						{
							if (UEdGraphNode_ComboNode* Node = WeakNode.Get())
							{
								// Find pin by ID — the pointer may be stale by the time lambda fires
								for (UEdGraphPin* NodePin : Node->Pins)
								{
									if (NodePin && NodePin->PinId == PinId)
									{
										Node->RemoveTransitionPin(NodePin);
										break;
									}
								}
							}
						})
					)
				);
			}
		}
	}

	Super::GetContextMenuActions(Menu, Context);
}

const FPinConnectionResponse UEdGraphSchema_ComboGraph::CanCreateConnection(const UEdGraphPin* A, const UEdGraphPin* B) const
{
	if (!A || !B)
	{
		return FPinConnectionResponse(CONNECT_RESPONSE_DISALLOW, LOCTEXT("NullPin", "Invalid pin."));
	}

	// Rule 1: Cannot connect a node to itself
	if (A->GetOwningNode() == B->GetOwningNode())
	{
		return FPinConnectionResponse(CONNECT_RESPONSE_DISALLOW, LOCTEXT("SameNode", "Cannot connect a node to itself."));
	}

	// Rule 2: Must be opposite directions — Output -> Input only
	if (A->Direction == B->Direction)
	{
		return FPinConnectionResponse(CONNECT_RESPONSE_DISALLOW, LOCTEXT("SameDirection", "Cannot connect two input pins or two output pins."));
	}

	// Rule 3: Both pins must be ComboTransition category
	if (A->PinType.PinCategory != ComboTransitionCategory || B->PinType.PinCategory != ComboTransitionCategory)
	{
		return FPinConnectionResponse(CONNECT_RESPONSE_DISALLOW, LOCTEXT("WrongCategory", "Only ComboTransition pins can connect."));
	}

	// Identify which is input and which is output for the remaining checks
	const UEdGraphPin* InputPin  = (A->Direction == EGPD_Input)  ? A : B;
	const UEdGraphPin* OutputPin = (A->Direction == EGPD_Output) ? A : B;

	// Which original parameter maps to the input pin — determines BREAK_OTHERS_A vs B
	const bool bAIsInput = (A == InputPin);

	// Rule 4: Input pin accepts exactly one connection (one parent per node)
	if (InputPin->LinkedTo.Num() > 0)
	{
		return FPinConnectionResponse(
			bAIsInput ? CONNECT_RESPONSE_BREAK_OTHERS_A : CONNECT_RESPONSE_BREAK_OTHERS_B,
			LOCTEXT("InputAlreadyConnected", "Input pin already has a connection. The existing connection will be replaced.")
		);
	}

	// Rule 5: Output pin accepts exactly one connection (one target per transition)
	if (OutputPin->LinkedTo.Num() > 0)
	{
		return FPinConnectionResponse(
			bAIsInput ? CONNECT_RESPONSE_BREAK_OTHERS_B : CONNECT_RESPONSE_BREAK_OTHERS_A,
			LOCTEXT("OutputAlreadyConnected", "Output pin already has a connection. The existing connection will be replaced.")
		);
	}

	return FPinConnectionResponse(CONNECT_RESPONSE_MAKE, LOCTEXT("ConnectionValid", ""));
}

bool UEdGraphSchema_ComboGraph::TryCreateConnection(UEdGraphPin* A, UEdGraphPin* B) const
{
	// Validate before attempting — avoids partial state changes on failure
	const FPinConnectionResponse Response = CanCreateConnection(A, B);
	if (Response.Response == CONNECT_RESPONSE_DISALLOW)
	{
		return false;
	}

	// Break existing connections as needed based on the response
	if (Response.Response == CONNECT_RESPONSE_BREAK_OTHERS_A)
	{
		A->BreakAllPinLinks();
	}
	else if (Response.Response == CONNECT_RESPONSE_BREAK_OTHERS_B)
	{
		B->BreakAllPinLinks();
	}

	return Super::TryCreateConnection(A, B);
}

void UEdGraphSchema_ComboGraph::CreateDefaultNodesForGraph(UEdGraph& Graph) const
{
	// Auto-spawn the root node — the graph is invalid without one
	UEdGraphNode_ComboNode* RootNode = CreateComboNode(&Graph, FVector2D(0.f, 0.f), false);
	if (RootNode)
	{
		RootNode->SetIsRootNode(true);
		RootNode->SetComboNodeName(FName(TEXT("Root")));

		// Root node is locked in the top-left — prevent accidental repositioning
		RootNode->bCanRenameNode = false;
	}
}

FLinearColor UEdGraphSchema_ComboGraph::GetPinTypeColor(const FEdGraphPinType& PinType) const
{
	// All ComboTransition pins are white — clean, neutral, readable against any node color
	if (PinType.PinCategory == ComboTransitionCategory)
	{
		return FLinearColor::White;
	}

	return Super::GetPinTypeColor(PinType);
}

void UEdGraphSchema_ComboGraph::BreakNodeLinks(UEdGraphNode& TargetNode) const
{
	Super::BreakNodeLinks(TargetNode);
	TargetNode.GetGraph()->NotifyGraphChanged();
}

void UEdGraphSchema_ComboGraph::BreakPinLinks(UEdGraphPin& TargetPin, bool bSendsNodeNotification) const
{
	Super::BreakPinLinks(TargetPin, bSendsNodeNotification);

	if (bSendsNodeNotification)
	{
		TargetPin.GetOwningNode()->GetGraph()->NotifyGraphChanged();
	}
}

void UEdGraphSchema_ComboGraph::BreakSinglePinLink(UEdGraphPin* SourcePin, UEdGraphPin* TargetPin) const
{
	Super::BreakSinglePinLink(SourcePin, TargetPin);

	if (SourcePin)
	{
		SourcePin->GetOwningNode()->GetGraph()->NotifyGraphChanged();
	}
}

// -----------------------------------------------------------------------
// Helpers
// -----------------------------------------------------------------------

UEdGraphNode_ComboNode* UEdGraphSchema_ComboGraph::CreateComboNode(
	UEdGraph* Graph,
	const FVector2D& Position,
	bool bSelectNewNode)
{
	if (!Graph)
	{
		return nullptr;
	}

	Graph->Modify();

	UEdGraphNode_ComboNode* NewNode = NewObject<UEdGraphNode_ComboNode>(Graph);
	NewNode->NodePosX = FMath::RoundToInt(Position.X);
	NewNode->NodePosY = FMath::RoundToInt(Position.Y);
	NewNode->SetFlags(RF_Transactional);

	Graph->AddNode(NewNode, true, bSelectNewNode);
	NewNode->CreateNewGuid();
	NewNode->PostPlacedNewNode();
	NewNode->AllocateDefaultPins();

	Graph->NotifyGraphChanged();

	return NewNode;
}

#undef LOCTEXT_NAMESPACE
