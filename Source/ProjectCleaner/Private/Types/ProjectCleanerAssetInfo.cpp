// Copyright Ashot Barkhudaryan. All Rights Reserved.

#include "Types/ProjectCleanerAssetInfo.h"

bool FProjectCleanerAssetInfo::IsValid() const
{
	return !ObjectPath.IsNone();
}

bool FProjectCleanerAssetInfo::operator==(const FProjectCleanerAssetInfo& Other) const
{
	return ObjectPath.IsEqual(Other.ObjectPath, ENameCase::CaseSensitive);
}

bool FProjectCleanerAssetInfo::operator!=(const FProjectCleanerAssetInfo& Other) const
{
	return !ObjectPath.IsEqual(Other.ObjectPath, ENameCase::CaseSensitive);
}

const FName& FProjectCleanerAssetInfo::GetObjectPath() const
{
	return ObjectPath;
}

const FName& FProjectCleanerAssetInfo::GetPackagePath() const
{
	return PackagePath;
}

const FName& FProjectCleanerAssetInfo::GetAssetName() const
{
	return AssetName;
}

const FName& FProjectCleanerAssetInfo::GetAssetClassName() const
{
	return AssetClassName;
}

const UClass* FProjectCleanerAssetInfo::GetAssetClass() const
{
	return AssetClass;
}

EProjectCleanerAssetCategory FProjectCleanerAssetInfo::GetAssetCategory() const
{
	return AssetCategory;
}

