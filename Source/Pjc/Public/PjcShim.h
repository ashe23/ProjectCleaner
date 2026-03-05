// Copyright Ashot Barkhudaryan. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Styling/SlateColor.h"
#include "Runtime/Launch/Resources/Version.h"

class FTabManager;
class FMultiBox;
struct FARFilter;
struct FAssetData;

#if ENGINE_MAJOR_VERSION == 5
#include "Styling/AppStyle.h"
#else
#include "EditorStyleSet.h"
#endif

// This is compatibility layer for handling different engine api versions
namespace PjcShim
{
	const ISlateStyle& GetStyle();
	const FSlateBrush* GetBrush(const FName PropertyName, const ANSICHAR* Specifier = nullptr);
	FSlateColor GetSlateColor(const FName PropertyName, const ANSICHAR* Specifier = nullptr);

	FName GetLevelEditorToolBarName();
	TSet<FName> GetCachedEmptyPackages();
	FAssetPackageData GetAssetPackageData(const FName PackageName);
	FString GetPathExternalActors();
	FString GetPathExternalObjects();

	void SetTabManagerMenuMultiBox(const TSharedPtr<FTabManager>& InTabManager, const TSharedRef<FMultiBox>& InMultiBox);

	void AddFilterClass(FARFilter& InFilter, const UClass* InClass);
	void AddFilterObjectPath(FARFilter& InFilter, const FAssetData& InAssetData);
	void ReserveFilterObjectPaths(FARFilter& InFilter, const int32 InNum);
	bool HasFilterObjectPaths(const FARFilter& InFilter);
	FName GetAssetClassName(const FAssetData& InAssetData);
	FName GetClassName(const UClass* InClass);

	FAssetData GetAssetByObjectPath(const FString& InObjectPath);
	void GetDerivedClassNames(const TArray<FName>& ClassNames, const TSet<FName>& ExcludedClassNames, TSet<FName>& OutDerivedClassNames);
	void OpenAssetEditor(const FAssetData& InAssetData);
	FString GetObjectPathString(const FAssetData& InAssetData);
}	// namespace PjcShim
