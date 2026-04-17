// Copyright 2026 Prasanna Keerthivasan. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "GameplayTagContainer.h"
#include "Interfaces/ComboGraphContract.h"
#include "ComboGraph.generated.h"

class UComboNode;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnComboBegin);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnComboOnGoing);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnComboEnd);

UCLASS(BlueprintType)
class COMBOGRAPH_API UComboGraph : public UObject, public IComboGraphContract
{
	GENERATED_BODY()

public:

	virtual UComboNode* GetRootNode() const override;

	virtual void NotifyTransition(const FGameplayTag ComboTag) override;

	virtual void NotifyComboEnd() override;

	virtual void NotifyNodeActivated(UComboNode* Node) override;

	virtual void NotifyInterruption() override;

	void InitializeFromNodes(TArray<UComboNode*> Nodes, UComboNode* Root);

	int32 GetCurrentDepth() const { return CurrentDepth; }
	TArray<FGameplayTag> GetComboHistory() const { return ComboHistory; }
	bool IsActive() const { return CurrentDepth > 0; }

	virtual bool IsTransitioning() const override { return bIsTransitioning; }
	virtual void SetTransitioning(bool bValue) override { bIsTransitioning = bValue; }

	void RouteInput(FGameplayTag Tag);

	/** Called by IComboWindowListener when UComboWindowNotify fires.
	 *  Forwards to the active node to open its input window early. */
	void NotifyComboWindow();
	void NotifyComboWindowClosed();

	UPROPERTY(BlueprintAssignable, Category = "Combo")
	FOnComboBegin OnComboBegin;

	UPROPERTY(BlueprintAssignable, Category = "Combo")
	FOnComboOnGoing OnComboOnGoing;

	UPROPERTY(BlueprintAssignable, Category = "Combo")
	FOnComboEnd OnComboEnd;

private:

	UPROPERTY()
	TArray<TObjectPtr<UComboNode>> AllNodes;

	UPROPERTY()
	TObjectPtr<UComboNode> RootNode;

	int32 CurrentDepth = 0;

	TArray<FGameplayTag> ComboHistory;

	bool bIsTransitioning = false;

	UPROPERTY()
	TWeakObjectPtr<UComboNode> ActiveNode;

	// The node that opened the current notify-state window — may differ from ActiveNode
	// after a rapid transition. Stale NotifyEnd events are discarded if this is null.
	UPROPERTY()
	TWeakObjectPtr<UComboNode> WindowNode;
};
