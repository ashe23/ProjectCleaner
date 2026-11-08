// Copyright Ashot Barkhudaryan. All Rights Reserved.

#include "Slate/PjcActions.h"
#include "Slate/SPjcDeleteDialog.h"
#include "Slate/SPjcScanBar.h"
#include "PjcScanDraft.h"
#include "PjcSettings.h"
#include "PjcShim.h"
#include "PjcSubsystem.h"
#include "PjcUnused.h"
// Engine Headers
#include "Widgets/Notifications/SNotificationList.h"

namespace PjcActions
{
	bool DeleteWithGroup(const TArray<FName>& Packages) {
		if (Packages.Num() == 0) return false;

		const FPjcScanResult& Result = UPjcSubsystem::GetScanResult();

		// a used asset that references them would break: refused, they are not deleted alone
		TArray<FName> Holders;
		FPjcUnusedTree::GetUsedHolders(Result, Packages, Holders);
		if (Holders.Num() > 0) {
			TArray<FString> Names;
			for (int32 Index = 0; Index < Holders.Num() && Index < 5; ++Index) {
				Names.Add(FPackageName::GetShortName(Holders[Index]));
			}
			const FString More = Holders.Num() > Names.Num() ? FString::Printf(TEXT(" and %d more"), Holders.Num() - Names.Num()) : FString {};
			const FString Used = FString::Join(Names, TEXT(", ")) + More;
			const FString Why = TEXT("Used assets reference them, deleting would break them: ") + Used;
			const FString Message = Why + TEXT(".\n\nKeep them, or remove those references first.");
			PjcShim::ShowDialog(FText::FromString(TEXT("Cannot delete")), FText::FromString(Message), EAppMsgType::Ok);
			return false;
		}

		TArray<FName> GroupPackages;
		FPjcUnusedTree::GetDeleteGroup(Result, Packages, GroupPackages);

		// asset data of scanned assets and redirectors
		TMap<FName, const FAssetData*> ByPackage;
		for (const auto& Asset : Result.Assets) {
			ByPackage.Add(Asset.PackageName, &Asset);
		}
		for (const auto& Asset : Result.Redirectors) {
			ByPackage.Add(Asset.PackageName, &Asset);
		}
		const auto ToAssets = [&ByPackage](const TArray<FName>& Names, TArray<FAssetData>& Out) {
			for (const FName Name : Names) {
				if (const FAssetData* const* Found = ByPackage.Find(Name)) {
					Out.Add(**Found);
				}
			}
		};
		TArray<FAssetData> Selected;
		TArray<FAssetData> With;
		ToAssets(Packages, Selected);
		ToAssets(GroupPackages, With);

		// what the last delete could not remove
		TArray<FString> Held;
		const TMap<FName, FString>& Kept = UPjcSubsystem::GetLastCleanupKept();
		for (const auto& Asset : Selected) {
			if (const FString* Holder = Kept.Find(Asset.PackageName)) {
				Held.Add(*Holder);
			}
		}
		for (const auto& Asset : With) {
			if (const FString* Holder = Kept.Find(Asset.PackageName)) {
				Held.Add(*Holder);
			}
		}

		if (!SPjcDeleteDialog::Confirm(Selected, With, Held)) return false;

		TArray<FAssetData> All = Selected;
		All.Append(With);
		UPjcSubsystem::DeleteAssets(All, true, true);
		SPjcScanBar::ScanProject();
		return true;
	}

	void Exclude(const TArray<FString>& Folders, const TArray<FString>& Classes, const TArray<FString>& Assets, const FString& Message) {
		UPjcAssetExcludeSettings* Settings = GetMutableDefault<UPjcAssetExcludeSettings>();
		for (const auto& Folder : Folders) {
			if (!Settings->ExcludedFolders.ContainsByPredicate([&Folder](const FDirectoryPath& Path) {
					return Path.Path.Equals(Folder);
				})) {
				Settings->ExcludedFolders.Add(FDirectoryPath {Folder});
			}
		}
		for (const auto& Class : Classes) {
			Settings->ExcludedClasses.AddUnique(TSoftClassPtr<UObject> {FSoftObjectPath {Class}});
		}
		for (const auto& Asset : Assets) {
			Settings->ExcludedAssets.AddUnique(TSoftObjectPtr<UObject> {FSoftObjectPath {Asset}});
		}
		PjcShim::UpdateDefaultConfigFile(Settings);

		// an edited draft keeps its edits and gets the exclusions too, so Rescan does not drop them
		FPjcScanDraft& Draft = FPjcScanDraft::Get();
		for (const auto& Folder : Folders) {
			Draft.ExcludedFolders.AddUnique(Folder);
		}
		for (const auto& Class : Classes) {
			Draft.ExcludedClasses.AddUnique(Class);
		}
		for (const auto& Asset : Assets) {
			Draft.ExcludedAssets.AddUnique(Asset);
		}
		FPjcScanDraft::OnEdited().Broadcast();

		UPjcSubsystem::ShowNotification(Message + TEXT(" \u00B7 saved to Config/DefaultEditor.ini"), SNotificationItem::CS_Success, 4.0f);
		SPjcScanBar::ScanProject();
	}

	void RemoveExclusions(const TArray<FString>& Folders, const TArray<FString>& Classes, const TArray<FString>& Assets, const FString& Message) {
		UPjcAssetExcludeSettings* Settings = GetMutableDefault<UPjcAssetExcludeSettings>();
		Settings->ExcludedFolders.RemoveAll([&Folders](const FDirectoryPath& Path) {
			return Folders.Contains(Path.Path);
		});
		Settings->ExcludedClasses.RemoveAll([&Classes](const TSoftClassPtr<UObject>& Class) {
			return Classes.Contains(Class.ToSoftObjectPath().ToString());
		});
		Settings->ExcludedAssets.RemoveAll([&Assets](const TSoftObjectPtr<UObject>& Asset) {
			return Assets.Contains(Asset.ToSoftObjectPath().ToString());
		});
		PjcShim::UpdateDefaultConfigFile(Settings);

		// an edited draft keeps its other edits
		FPjcScanDraft& Draft = FPjcScanDraft::Get();
		Draft.ExcludedFolders.RemoveAll([&Folders](const FString& Folder) {
			return Folders.Contains(Folder);
		});
		Draft.ExcludedClasses.RemoveAll([&Classes](const FString& Class) {
			return Classes.Contains(Class);
		});
		Draft.ExcludedAssets.RemoveAll([&Assets](const FString& Asset) {
			return Assets.Contains(Asset);
		});
		FPjcScanDraft::OnEdited().Broadcast();

		UPjcSubsystem::ShowNotification(Message + TEXT(" \u00B7 saved to Config/DefaultEditor.ini"), SNotificationItem::CS_Success, 4.0f);
		SPjcScanBar::ScanProject();
	}
}	// namespace PjcActions
