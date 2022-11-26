// Copyright Ashot Barkhudaryan. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ProjectCleanerTypes.generated.h"

UCLASS(Transient)
class UProjectCleanerStatListItem final : public UObject
{
	GENERATED_BODY()
public:
	FString Name;
	FString Category;
	FString Count;
	FString Size;
	FLinearColor Color = FLinearColor::White;
};

UCLASS(Transient)
class UProjectCleanerStatTreeItem : public UObject
{
	GENERATED_BODY()

public:
	UProjectCleanerStatTreeItem()
	{
	}

	FString PathFull;
	FString Path;
	FString Size;
	FString Files;
	FString Folders;
	FString Unused;
	FString Empty;
	float Progress = 0.0f;

	TWeakObjectPtr<UProjectCleanerStatTreeItem> ParentDir;
	TArray<TWeakObjectPtr<UProjectCleanerStatTreeItem>> SubDirs;
};
