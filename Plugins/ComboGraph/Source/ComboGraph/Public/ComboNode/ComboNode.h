#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "GameplayTagContainer.h"
#include "ComboNode.generated.h"

class UComboPath;
class UAnimMontage;
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

protected:

	UFUNCTION()
	void OnMontageCompleted(UAnimMontage* Montage, bool bInterrupted);

	void OnDecayExpired();

	void OnExitState();

	void ResetActivationState();

private:

	IComboGraphContract* GetGraphContract() const;

	void StopDecayTimer();

	FGameplayTag NextComboTag;

	FTimerHandle DecayTimerHandle;

	bool bMontageCompleted = false;

	TWeakObjectPtr<USkeletalMeshComponent> CachedMesh;
};
