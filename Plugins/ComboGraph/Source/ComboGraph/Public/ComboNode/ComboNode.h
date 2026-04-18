// Copyright 2026 Prasanna Keerthivasan. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "GameplayTagContainer.h"
#include "ComboNode.generated.h"

class UComboPath;
class UAnimMontage;
class UAnimSequenceBase;
class UAnimInstance;
class USkeletalMeshComponent;
class IComboGraphContract;

UCLASS(BlueprintType)
class COMBOGRAPH_API UComboNode : public UObject
{
	GENERATED_BODY()

public:

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combo")
	TMap<FGameplayTag, TObjectPtr<UComboPath>> TransitionMap;

	/** Post-montage idle window in seconds. Independent of montage duration. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combo")
	float DecayTime = 0.5f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combo")
	TObjectPtr<UAnimMontage> ActionMontage;

	/** Optional montage section for minor variants within one montage file. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combo")
	FName MontageSection = NAME_None;

	void Activate(USkeletalMeshComponent* InMesh);

	void ReceiveInput(FGameplayTag Tag);

	/** Called by UComboGraph::NotifyComboWindow() when UComboWindowNotify fires.
	 *  Opens the input window early — decay starts now, montage keeps playing.
	 *  If input was already buffered, transitions immediately. */
	void OpenComboWindow();
	void CloseComboWindow(UAnimSequenceBase* Animation);

protected:

	UFUNCTION()
	void OnMontageCompleted(UAnimMontage* Montage, bool bInterrupted);

	void OnDecayExpired();

	void OnExitState();

	void ResetActivationState();

private:

	IComboGraphContract* GetGraphContract() const;

	void StopDecayTimer();
	void StartDecayTimer();

	FGameplayTag NextComboTag;

	FTimerHandle DecayTimerHandle;

	// Cached when the decay timer starts — lets StopDecayTimer clear it
	// even if CachedMesh becomes invalid (actor death, level transition)
	TWeakObjectPtr<UWorld> CachedTimerWorld;

	bool bMontageCompleted  = false;

	// True once UComboWindowNotify fires — opens input window before montage ends
	bool bComboWindowOpen   = false;

	// Guards against OnExitState being called twice if notify and montage-end race
	bool bExitStateInFlight = false;

	TWeakObjectPtr<USkeletalMeshComponent> CachedMesh;
};
