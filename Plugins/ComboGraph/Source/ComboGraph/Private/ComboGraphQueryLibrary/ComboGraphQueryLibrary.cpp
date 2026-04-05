// Copyright 2026 Prasanna Keerthivasan. All Rights Reserved.

#include "ComboGraphQueryLibrary/ComboGraphQueryLibrary.h"
#include "ComboGraph/ComboGraph.h"
#include "ComboGraphModule.h"

int32 UComboGraphQueryLibrary::GetCurrentComboDepth(const UComboGraph* Graph)
{
	if (!Graph)
	{
		UE_LOG(LogComboGraph, Warning, TEXT("GetCurrentComboDepth called with null Graph"));
		return 0;
	}

	return Graph->GetCurrentDepth();
}

TArray<FGameplayTag> UComboGraphQueryLibrary::GetComboHistory(const UComboGraph* Graph)
{
	if (!Graph)
	{
		UE_LOG(LogComboGraph, Warning, TEXT("GetComboHistory called with null Graph"));
		return TArray<FGameplayTag>();
	}

	return Graph->GetComboHistory();
}

FGameplayTag UComboGraphQueryLibrary::GetLastComboTag(const UComboGraph* Graph)
{
	if (!Graph)
	{
		UE_LOG(LogComboGraph, Warning, TEXT("GetLastComboTag called with null Graph"));
		return FGameplayTag();
	}

	const TArray<FGameplayTag> History = Graph->GetComboHistory();

	if (History.Num() == 0)
	{
		return FGameplayTag();
	}

	return History.Last();
}

bool UComboGraphQueryLibrary::IsComboActive(const UComboGraph* Graph)
{
	if (!Graph)
	{
		UE_LOG(LogComboGraph, Warning, TEXT("IsComboActive called with null Graph"));
		return false;
	}

	return Graph->IsActive();
}
