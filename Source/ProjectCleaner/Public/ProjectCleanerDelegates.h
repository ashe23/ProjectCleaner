// Copyright Ashot Barkhudaryan. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

DECLARE_MULTICAST_DELEGATE(FProjectCleanerDelegateProjectScanned);
DECLARE_MULTICAST_DELEGATE_OneParam(FProjectCleanerDelegateFilterChanged, const bool)
DECLARE_DELEGATE_OneParam(FProjectCleanerDelegateTreeViewPathSelected, const TSet<FString>&)
