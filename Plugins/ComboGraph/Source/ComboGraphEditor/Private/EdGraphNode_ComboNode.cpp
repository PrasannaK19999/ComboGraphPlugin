// Copyright 2026 Prasanna Keerthivasan. All Rights Reserved.

#include "EdGraphNode_ComboNode.h"
#include "EdGraph/EdGraphPin.h"
#include "EdGraph/EdGraph.h"
#include "Animation/AnimMontage.h"

// -----------------------------------------------------------------------
// Static constants
// -----------------------------------------------------------------------

const FName UEdGraphNode_ComboNode::InputPinName(TEXT("ComboInput"));
const FName UEdGraphNode_ComboNode::OutputPinBaseName(TEXT("ComboOutput"));

// -----------------------------------------------------------------------
// Constructor
// -----------------------------------------------------------------------

UEdGraphNode_ComboNode::UEdGraphNode_ComboNode()
	: bIsRootNode(false)
	, OutputPinCounter(0)
{
	MontageSection = NAME_None;
}

// -----------------------------------------------------------------------
// UEdGraphNode interface
// -----------------------------------------------------------------------

void UEdGraphNode_ComboNode::AllocateDefaultPins()
{
	// Non-root nodes have one input pin (entry point from the parent transition).
	// Root node has no input — nothing connects into it.
	if (!bIsRootNode)
	{
		CreatePin(EGPD_Input, TEXT("ComboTransition"), InputPinName);
	}

	// Output pins are added explicitly via CreateTransitionPin().
	// No default output pin on creation — the graph is empty until the user adds transitions.
}

FText UEdGraphNode_ComboNode::GetNodeTitle(ENodeTitleType::Type TitleType) const
{
	if (bIsRootNode)
	{
		return FText::FromString(TEXT("Root"));
	}

	if (!ComboNodeDisplayName.IsNone())
	{
		return FText::FromName(ComboNodeDisplayName);
	}

	return FText::FromString(TEXT("Combo Node"));
}

FLinearColor UEdGraphNode_ComboNode::GetNodeTitleColor() const
{
	// Root node: gold/amber to visually distinguish it as the unique entry point
	if (bIsRootNode)
	{
		return FLinearColor(1.0f, 0.7f, 0.0f, 1.0f);
	}

	// Standard combo node: dark slate
	return FLinearColor(0.1f, 0.1f, 0.1f, 1.0f);
}

FText UEdGraphNode_ComboNode::GetTooltipText() const
{
	if (bIsRootNode)
	{
		return FText::FromString(TEXT("Root node. Every combo starts here. Cannot be deleted."));
	}

	if (ActionMontage)
	{
		return FText::Format(
			FText::FromString(TEXT("Montage: {0}\nDecay: {1}s")),
			FText::FromString(ActionMontage->GetName()),
			FText::AsNumber(DecayTime)
		);
	}

	return FText::FromString(TEXT("No montage assigned."));
}

bool UEdGraphNode_ComboNode::CanUserDeleteNode() const
{
	// Root node is the graph's mandatory entry point — deletion would leave the graph in an invalid state
	return !bIsRootNode;
}

bool UEdGraphNode_ComboNode::CanDuplicateNode() const
{
	// Duplicating the root would create two roots — invalid by graph rules
	return !bIsRootNode;
}

void UEdGraphNode_ComboNode::DestroyNode()
{
	// Break all connections before the node is removed from the graph
	for (UEdGraphPin* Pin : Pins)
	{
		if (Pin)
		{
			Pin->BreakAllPinLinks();
		}
	}

	Super::DestroyNode();
}

void UEdGraphNode_ComboNode::AutowireNewNode(UEdGraphPin* FromPin)
{
	if (!FromPin)
	{
		return;
	}

	// When the user drags from an output pin and drops a new node, auto-connect
	// the source output pin to this node's input pin
	if (FromPin->Direction == EGPD_Output)
	{
		UEdGraphPin* InputPin = GetInputPin();
		if (InputPin && GetSchema())
		{
			GetSchema()->TryCreateConnection(FromPin, InputPin);
		}
	}
}

// -----------------------------------------------------------------------
// Pin accessors
// -----------------------------------------------------------------------

UEdGraphPin* UEdGraphNode_ComboNode::GetInputPin() const
{
	return FindPin(InputPinName, EGPD_Input);
}

TArray<UEdGraphPin*> UEdGraphNode_ComboNode::GetOutputPins() const
{
	TArray<UEdGraphPin*> OutputPins;
	for (UEdGraphPin* Pin : Pins)
	{
		if (Pin && Pin->Direction == EGPD_Output)
		{
			OutputPins.Add(Pin);
		}
	}
	return OutputPins;
}

UEdGraphPin* UEdGraphNode_ComboNode::CreateTransitionPin(const FGameplayTag& Tag)
{
	// Unique pin name using the serialized counter — safe across save/load cycles
	const FName PinName = *FString::Printf(TEXT("%s_%d"), *OutputPinBaseName.ToString(), OutputPinCounter++);

	// Build display label from the tag's last segment ("Input.Light" -> "Light")
	FString PinLabel;
	if (Tag.IsValid())
	{
		const FString TagString = Tag.ToString();
		int32 DotIndex = INDEX_NONE;
		PinLabel = TagString.FindLastChar(TEXT('.'), DotIndex)
			? TagString.RightChop(DotIndex + 1)
			: TagString;
	}
	else
	{
		PinLabel = TEXT("Untagged");
	}

	Modify();

	UEdGraphPin* NewPin = CreatePin(EGPD_Output, TEXT("ComboTransition"), PinName);
	if (NewPin)
	{
		NewPin->PinFriendlyName = FText::FromString(PinLabel);

		if (Tag.IsValid())
		{
			PinTagMap.Add(NewPin->PinId, Tag);
		}
	}

	return NewPin;
}

void UEdGraphNode_ComboNode::RemoveTransitionPin(UEdGraphPin* Pin)
{
	if (!Pin || Pin->Direction != EGPD_Output)
	{
		return;
	}

	Modify();

	Pin->BreakAllPinLinks();
	PinTagMap.Remove(Pin->PinId);
	RemovePin(Pin);

	if (UEdGraph* Graph = GetGraph())
	{
		Graph->NotifyGraphChanged();
	}
}

// -----------------------------------------------------------------------
// Pin <-> GameplayTag mapping
// -----------------------------------------------------------------------

FGameplayTag UEdGraphNode_ComboNode::GetPinGameplayTag(const UEdGraphPin* Pin) const
{
	if (!Pin || Pin->Direction != EGPD_Output)
	{
		return FGameplayTag();
	}

	const FGameplayTag* Found = PinTagMap.Find(Pin->PinId);
	return Found ? *Found : FGameplayTag();
}

void UEdGraphNode_ComboNode::SetPinGameplayTag(UEdGraphPin* Pin, const FGameplayTag& NewTag)
{
	if (!Pin || Pin->Direction != EGPD_Output)
	{
		return;
	}

	Modify();

	if (NewTag.IsValid())
	{
		PinTagMap.Add(Pin->PinId, NewTag);

		// Update visible label to the tag's short name
		const FString TagString = NewTag.ToString();
		int32 DotIndex = INDEX_NONE;
		const FString ShortName = TagString.FindLastChar(TEXT('.'), DotIndex)
			? TagString.RightChop(DotIndex + 1)
			: TagString;

		Pin->PinFriendlyName = FText::FromString(ShortName);
	}
	else
	{
		PinTagMap.Remove(Pin->PinId);
		Pin->PinFriendlyName = FText::FromString(TEXT("Untagged"));
	}

	if (UEdGraph* Graph = GetGraph())
	{
		Graph->NotifyGraphChanged();
	}
}
