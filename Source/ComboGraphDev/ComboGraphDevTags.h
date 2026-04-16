// Copyright 2026 Prasanna Keerthivasan. All Rights Reserved.

#pragma once

#include "NativeGameplayTags.h"

// -----------------------------------------------------------------------
// Combo input tags
// Declared here — defined in ComboGraphDevTags.cpp
// Use these anywhere in the project instead of FGameplayTag::RequestGameplayTag()
// -----------------------------------------------------------------------

namespace ComboGraphDevTags
{
	// Starting input: first button in a combo chain
	COMBOGRAPHDEV_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Input_A)
	COMBOGRAPHDEV_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Input_B)
}
