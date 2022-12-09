// Copyright Ashot Barkhudaryan. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ProjectCleanerTreeViewSettings.generated.h"

UCLASS(Config=EditorPerProjectUserSettings)
class UProjectCleanerTreeViewSettings final : public UObject
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Config, meta=(ToolTip="Show tree view organizer lines"))
	bool bShowLines = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Config, meta=(ToolTip="Show empty folders"))
	bool bShowFoldersEmpty = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Config, meta=(ToolTip="Show excluded folders"))
	bool bShowFoldersExcluded = true;

protected:
#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif
};
