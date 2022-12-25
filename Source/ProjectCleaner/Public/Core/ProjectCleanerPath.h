// Copyright Ashot Barkhudaryan. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

struct FProjectCleanerPath
{
	explicit FProjectCleanerPath(const FString& InPath);
	bool IsValid() const;
	bool IsFile() const;
	bool IsDirectory() const;
	const FString& GetPathAbs() const;
	const FString& GetPathRel() const;
	const FString& GetExtension() const;
	const FString& GetName() const;

	bool operator==(const FProjectCleanerPath& Other) const
	{
		return IsValid() && PathAbs.Equals(Other.PathAbs, ESearchCase::CaseSensitive);
	}

	bool operator!=(const FProjectCleanerPath& Other) const
	{
		return IsValid() && !PathAbs.Equals(Other.PathAbs, ESearchCase::CaseSensitive);
	}

private:
	FString PathAbs;
	FString PathRel;
	FString Extension;
	FString Name;
};
