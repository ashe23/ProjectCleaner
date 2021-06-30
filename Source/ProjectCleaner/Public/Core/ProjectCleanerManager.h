#pragma once

#include "CoreMinimal.h"
#include "StructsContainer.h"

enum class ERelationType
{
	Reference,
	Dependency
};

class ProjectCleanerManager
{
public:
	ProjectCleanerManager();
	FProjectCleanerData* GetCleanerData();
	void UpdateData();
private:
	void GetRelatedAssets(const FName& PackageName, const ERelationType RelationType, TArray<FName>& RelatedAssets) const;

	FProjectCleanerData CleanerData;
	UCleanerConfigs* CleanerConfigs;
	UExcludeOptions* ExcludeOptions;
};
