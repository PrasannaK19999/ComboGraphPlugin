// Copyright 2026 Prasanna Keerthivasan. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "EdGraph/EdGraphNode.h"
#include "GameplayTagContainer.h"
#include "EdGraphNode_ComboNode.generated.h"

class UAnimMontage;
class UEdGraphPin;

/**
 * UEdGraphNode_ComboNode
 *
 * Visual representation of one combo node in the editor graph.
 *
 * Pin layout:
 *   [Input]  — single exec-style input pin (top). Root node has none.
 *   [Output] — N output pins, one per transition. Each carries a FGameplayTag
 *              that maps 1:1 to the TMap key in UComboNode::TransitionMap at runtime.
 *
 * Authored data exposed in Details panel when selected:
 *   - ActionMontage (UAnimMontage*)
 *   - MontageSection (FName)
 *   - DecayTime (float)
 *   - DisplayName (FName) — for debug/editor readability
 *
 * Root node rules:
 *   - Auto-spawned on graph creation, exactly one per graph
 *   - No input pin (nothing connects to it)
 *   - Gold/amber border, locked top-left, label "Root"
 *   - Cannot be deleted (CanUserDeleteNode returns false)
 *
 * Pin tag rules:
 *   - Tags assigned via Details panel after connecting
 *   - Same tag across different nodes = always valid
 *   - Same tag twice on same node = blocked by schema (TMap key collision)
 *   - Untagged connected pin = validation warning at compile
 */
UCLASS()
class COMBOGRAPHEDITOR_API UEdGraphNode_ComboNode : public UEdGraphNode
{
	GENERATED_BODY()

public:

	UEdGraphNode_ComboNode();

	// -----------------------------------------------------------------------
	// UEdGraphNode interface
	// -----------------------------------------------------------------------

	virtual void AllocateDefaultPins() override;
	virtual FText GetNodeTitle(ENodeTitleType::Type TitleType) const override;
	virtual FLinearColor GetNodeTitleColor() const override;
	virtual FText GetTooltipText() const override;
	virtual bool CanUserDeleteNode() const override;
	virtual bool CanDuplicateNode() const override;
	virtual void DestroyNode() override;
	virtual void AutowireNewNode(UEdGraphPin* FromPin) override;

	// -----------------------------------------------------------------------
	// Pin accessors
	// -----------------------------------------------------------------------

	/** Returns the single input pin, or nullptr if this is the root node. */
	UEdGraphPin* GetInputPin() const;

	/** Returns all output pins (transition pins). */
	TArray<UEdGraphPin*> GetOutputPins() const;

	/**
	 * Creates a new output pin tagged with the given GameplayTag.
	 * Pin label displays the tag's short name (e.g. "Light", "Heavy").
	 *
	 * @param Tag The GameplayTag for this transition. Can be empty (assigned later via Details).
	 * @return The created output pin, or nullptr on failure.
	 */
	UEdGraphPin* CreateTransitionPin(const FGameplayTag& Tag = FGameplayTag());

	/** Removes a specific output pin and breaks its connections. */
	void RemoveTransitionPin(UEdGraphPin* Pin);

	// -----------------------------------------------------------------------
	// Pin <-> GameplayTag mapping
	// -----------------------------------------------------------------------

	/**
	 * Gets the GameplayTag associated with an output pin.
	 * Returns an empty tag if the pin is untagged or not an output pin.
	 */
	FGameplayTag GetPinGameplayTag(const UEdGraphPin* Pin) const;

	/**
	 * Sets the GameplayTag on an output pin.
	 * Updates the pin's display label to the tag's short name.
	 *
	 * @param Pin The output pin to tag.
	 * @param NewTag The GameplayTag to assign.
	 */
	void SetPinGameplayTag(UEdGraphPin* Pin, const FGameplayTag& NewTag);

	// -----------------------------------------------------------------------
	// Authored data — exposed in Details panel
	// -----------------------------------------------------------------------

	/** Human-readable name for editor/debug display. */
	UPROPERTY(EditAnywhere, Category = "Combo Node")
	FName ComboNodeDisplayName;

	/** The montage this node plays when activated. One montage per node. */
	UPROPERTY(EditAnywhere, Category = "Combo Node")
	TObjectPtr<UAnimMontage> ActionMontage;

	/** Optional montage section. If NAME_None, plays from start. */
	UPROPERTY(EditAnywhere, Category = "Combo Node")
	FName MontageSection;

	/**
	 * Post-montage idle window in seconds.
	 * After the animation finishes, the player has this many seconds to input.
	 * Independent of montage duration. Matches UComboNode::DecayTime default.
	 */
	UPROPERTY(EditAnywhere, Category = "Combo Node", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float DecayTime = 0.5f;

	// -----------------------------------------------------------------------
	// Getters for compile pipeline
	// -----------------------------------------------------------------------

	FName GetComboNodeName() const { return ComboNodeDisplayName; }
	UAnimMontage* GetMontage() const { return ActionMontage; }
	FName GetMontageSection() const { return MontageSection; }
	float GetDecayTime() const { return DecayTime; }

	// -----------------------------------------------------------------------
	// Setters for import pipeline
	// -----------------------------------------------------------------------

	void SetComboNodeName(FName InName) { ComboNodeDisplayName = InName; }
	void SetMontage(UAnimMontage* InMontage) { ActionMontage = InMontage; }
	void SetMontageSection(FName InSection) { MontageSection = InSection; }
	void SetDecayTime(float InDecayTime) { DecayTime = InDecayTime; }

	// -----------------------------------------------------------------------
	// Root node
	// -----------------------------------------------------------------------

	bool IsRootNode() const { return bIsRootNode; }
	void SetIsRootNode(bool bInIsRoot) { bIsRootNode = bInIsRoot; }

private:

	/** True if this is the root node. Exactly one per graph. */
	UPROPERTY()
	bool bIsRootNode;

	/**
	 * Maps output pin IDs to their assigned GameplayTags.
	 * Key: Pin->PinId (FGuid). Value: the transition's GameplayTag.
	 * Serialized — survives save/load/undo.
	 */
	UPROPERTY()
	TMap<FGuid, FGameplayTag> PinTagMap;

	/**
	 * Incrementing counter for unique output pin naming.
	 * Serialized to prevent name collisions after save/load cycles.
	 */
	UPROPERTY()
	int32 OutputPinCounter;

	// -----------------------------------------------------------------------
	// Pin name constants
	// -----------------------------------------------------------------------

	static const FName InputPinName;
	static const FName OutputPinBaseName;
};
