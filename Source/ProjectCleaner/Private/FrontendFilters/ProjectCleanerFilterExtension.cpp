// Copyright Ashot Barkhudaryan. All Rights Reserved.

#include "FrontendFilters/ProjectCleanerFilterExtension.h"
#include "FrontendFilters/ProjectCleanerFrontendFilterPrimary.h"

void UProjectCleanerFilterExtension::AddFrontEndFilterExtensions(TSharedPtr<FFrontendFilterCategory> DefaultCategory, TArray<TSharedRef<FFrontendFilter>>& InOutFilterList) const
{
	Super::AddFrontEndFilterExtensions(DefaultCategory, InOutFilterList);

	const FText ProjectCleanerCategoryName = FText::FromString(TEXT("ProjectCleaner Filters"));
	const TSharedPtr<FFrontendFilterCategory> ProjectCleanerCategory = MakeShareable(
		new FFrontendFilterCategory(
			ProjectCleanerCategoryName,
			ProjectCleanerCategoryName
		)
	);
	InOutFilterList.Add(MakeShareable(new FFrontendFilterPrimaryAssets(ProjectCleanerCategory)));
}
