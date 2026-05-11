// Copyright 2026 Prasanna Keerthivasan. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameplayTagContainer.h"
#include "Interfaces/ComboWindowListener.h"
#include "ComboManagerComponent.generated.h"

class UDataTable;
class UComboGraph;
class USkeletalMeshComponent;

/**
 * UComboManagerComponent
 *
 * Drop this onto any character. Assign a ComboDataTable and call
 * OnComboInput() from your input bindings to drive the combo system.
 * Swap ComboDataTable at runtime (SetWeaponDataTable) to change weapons.
 */
UCLASS(ClassGroup=(Combat), meta=(BlueprintSpawnableComponent))
class COMBOGRAPH_API UComboManagerComponent : public UActorComponent, public IComboWindowListener
{
	GENERATED_BODY()

public:

	UComboManagerComponent();

	virtual void BeginPlay() override;

	// IComboWindowListener
	virtual void OnComboWindowOpened_Implementation() override;
	virtual void OnComboWindowClosed_Implementation(UAnimSequenceBase* Animation) override;

	// -----------------------------------------------------------------------
	// Configuration
	// -----------------------------------------------------------------------

	/** DataTable (FComboGraphTableRow rows) for the currently equipped weapon. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combo")
	TObjectPtr<UDataTable> ComboDataTable;

	// -----------------------------------------------------------------------
	// Runtime API
	// -----------------------------------------------------------------------

	/**
	 * Call from input bindings whenever the player presses a combo button.
	 * On first press (no active combo): looks up the matching chain and starts it.
	 * During an active combo: routes the tag as a buffered continuation input.
	 * If the tag has no matching row in the DataTable, the call is silently ignored.
	 */
	UFUNCTION(BlueprintCallable, Category = "Combo")
	void OnComboInput(FGameplayTag InputTag);

	/**
	 * Swap the active weapon's DataTable mid-game.
	 * Interrupts any combo in progress and rebuilds the graph map.
	 */
	UFUNCTION(BlueprintCallable, Category = "Combo")
	void SetWeaponDataTable(UDataTable* NewTable);

	/** Returns the active UComboGraph while a combo is running, nullptr when idle.
	 *  Pass to UComboGraphQueryLibrary functions to inspect combo state in Blueprint. */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Combo")
	UComboGraph* GetActiveGraph() const;

private:

	// Built from ComboDataTable at BeginPlay / weapon swap
	UPROPERTY()
	TMap<FGameplayTag, TObjectPtr<UComboGraph>> ComboGraphMap;

	// Graph currently executing (null when idle)
	UPROPERTY()
	TObjectPtr<UComboGraph> ActiveGraph;

	// Skeletal mesh cached from owner for montage playback
	UPROPERTY()
	TWeakObjectPtr<USkeletalMeshComponent> CachedMesh;

	// Rebuilds ComboGraphMap from ComboDataTable
	void RebuildGraphs();

	// Bound to ActiveGraph->OnComboEnd — clears ActiveGraph and resets transitioning flag
	UFUNCTION()
	void OnActiveComboEnded();
};
