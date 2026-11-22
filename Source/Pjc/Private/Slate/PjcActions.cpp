// Copyright Ashot Barkhudaryan. All Rights Reserved.

#include "Slate/PjcActions.h"
#include "Slate/SPjcDeleteDialog.h"
#include "Slate/SPjcDialog.h"
#include "Slate/SPjcScanBar.h"
#include "PjcFormat.h"
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
			const FString Why = TEXT("Used assets reference them. Deleting them breaks these assets: ") + Used;
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

	void MarkUnused(const TArray<FName>& Packages) {
		const FPjcScanResult& Result = UPjcSubsystem::GetScanResult();
		TArray<FString> Paths;
		TArray<TPair<FString, FString>> Rows;
		int64 Size = 0;
		for (const auto& Asset : Result.Assets) {
			if (!Packages.Contains(Asset.PackageName)) continue;

			Paths.Add(PjcShim::GetObjectPathString(Asset));
			Size += Result.GetSize(Asset.PackageName);
			if (Rows.Num() < 12) Rows.Emplace(Asset.AssetName.ToString(), PjcFormat::Size(Result.GetSize(Asset.PackageName)));
		}
		if (Paths.Num() == 0) return;

		// the assets to review only they keep go with them
		int32 NumWith = 0;
		for (const auto& Pair : Result.Status) {
			if (Pair.Value != EPjcAssetStatus::Review || Packages.Contains(Pair.Key)) continue;

			NumWith += Packages.Contains(Result.GetChainRoot(Pair.Key)) ? 1 : 0;
		}

		TArray<FPjcDialogPart> Parts;
		FPjcDialogPart& What = Parts.AddDefaulted_GetRef();
		What.Note
			= TEXT("They leave Review and count as unused. Nothing is deleted now. ") TEXT("Clean project or the Assets tab deletes them later.");
		FPjcDialogPart& List = Parts.AddDefaulted_GetRef();
		List.Heading = TEXT("Assets \u00B7 ") + PjcFormat::Size(Size);
		List.Rows = Rows;
		if (Paths.Num() > Rows.Num()) List.Rows.Emplace(FString::Printf(TEXT("and %d more"), Paths.Num() - Rows.Num()), FString {});
		if (NumWith > 0) {
			FPjcDialogPart& With = Parts.AddDefaulted_GetRef();
			With.Note
				= FString::Printf(TEXT("%s more assets to review also become unused. Only these assets keep them."), *PjcFormat::Count(NumWith));
			With.NoteColor = TEXT("ProjectCleaner.Color.Yellow");
		}
		FPjcDialogPart& Saved = Parts.AddDefaulted_GetRef();
		Saved.Note = TEXT("Saved in Config/DefaultEditor.ini. Your team shares this file. ")
			TEXT("Issues \u203A Assets to review \u203A Marked unused lists them. ") TEXT("If a used asset references one, it stays used.");
		const TCHAR* Plural = Paths.Num() == 1 ? TEXT("") : TEXT("s");
		const FString Title = FString::Printf(TEXT("Mark %s asset%s unused"), *PjcFormat::Count(Paths.Num()), Plural);
		if (!SPjcDialog::Show(Title, Parts, TEXT("Mark unused"))) return;

		UPjcAssetExcludeSettings* Settings = GetMutableDefault<UPjcAssetExcludeSettings>();
		for (const auto& Path : Paths) {
			Settings->MarkedUnusedAssets.AddUnique(TSoftObjectPtr<UObject> {FSoftObjectPath {Path}});
		}
		PjcShim::UpdateDefaultConfigFile(Settings);

		const FString Message = FString::Printf(
			TEXT("Marked %s asset%s unused"), *PjcFormat::Count(Paths.Num() + NumWith), Paths.Num() + NumWith == 1 ? TEXT("") : TEXT("s")
		);
		UPjcSubsystem::ShowNotification(Message + TEXT(" \u00B7 saved to Config/DefaultEditor.ini"), SNotificationItem::CS_Success, 4.0f);
		SPjcScanBar::ScanProject();
	}

	void RemoveMarks(const TArray<FName>& Packages) {
		const auto IsOneOf = [&Packages](const FString& Path) {
			return Packages.Contains(FName {*FSoftObjectPath {Path}.GetLongPackageName()});
		};
		UPjcAssetExcludeSettings* Settings = GetMutableDefault<UPjcAssetExcludeSettings>();
		const int32 Num = Settings->MarkedUnusedAssets.RemoveAll([&IsOneOf](const TSoftObjectPtr<UObject>& Asset) {
			return IsOneOf(Asset.ToSoftObjectPath().ToString());
		});
		if (Num == 0) return;

		PjcShim::UpdateDefaultConfigFile(Settings);

		const FString Message
			= FString::Printf(TEXT("Removed %s mark%s \u00B7 to review again"), *PjcFormat::Count(Num), Num == 1 ? TEXT("") : TEXT("s"));
		UPjcSubsystem::ShowNotification(Message, SNotificationItem::CS_Success, 4.0f);
		SPjcScanBar::ScanProject();
	}

	FString GetForceDeleteWarning() {
		return TEXT("Uses the engine's Force Delete, as in Content Browser. ")
			TEXT("It clears references to these assets in memory and in open asset editors. Then it deletes them. ")
				TEXT("Unsaved changes in those editors can be lost. You cannot undo it.");
	}

	int32 ForceDelete(const TArray<FName>& Packages, const bool bAsk, const bool bScan, int32* OutInMemory) {
		if (Packages.Num() == 0) return 0;

		if (bAsk) {
			TArray<FPjcDialogPart> Parts;
			FPjcDialogPart& Warning = Parts.AddDefaulted_GetRef();
			Warning.Note = GetForceDeleteWarning();
			Warning.NoteColor = TEXT("ProjectCleaner.Color.Yellow");
			FPjcDialogPart& What = Parts.AddDefaulted_GetRef();
			What.Heading = TEXT("Failed to delete, held by");
			const TMap<FName, FString>& Kept = UPjcSubsystem::GetLastCleanupKept();
			for (int32 Index = 0; Index < Packages.Num() && Index < 12; ++Index) {
				const FString* Holder = Kept.Find(Packages[Index]);
				What.Rows.Emplace(FPackageName::GetShortName(Packages[Index]), Holder ? *Holder : FString {TEXT("something in memory")});
			}
			if (Packages.Num() > 12) What.Rows.Emplace(FString::Printf(TEXT("and %d more"), Packages.Num() - 12), FString {});
			Parts.Add(SPjcDialog::MakeVersionControlNote());
			const FString Title = FString::Printf(TEXT("Force delete %s"), *PjcFormat::Count(Packages.Num()));
			if (!SPjcDialog::Show(Title, Parts, TEXT("Force delete"), TEXT("ProjectCleaner.Color.UnusedFill"))) return 0;
		}

		const int32 NumGone = UPjcSubsystem::ForceDeleteAssets(Packages);
		int32 NumInMemory = 0;
		for (const FName Package : Packages) {
			if (!FPackageName::DoesPackageExist(Package.ToString()) && FindPackage(nullptr, *Package.ToString())) ++NumInMemory;
		}
		if (OutInMemory) *OutInMemory = NumInMemory;
		if (bAsk) {
			FString Text = FString::Printf(TEXT("Force deleted %s of %s assets"), *PjcFormat::Count(NumGone), *PjcFormat::Count(Packages.Num()));
			if (NumInMemory > 0) Text += TEXT(". Restart the editor to remove them from the lists");
			UPjcSubsystem::ShowNotification(Text, NumGone == Packages.Num() ? SNotificationItem::CS_Success : SNotificationItem::CS_Fail, 5.0f);
		}
		if (bScan) SPjcScanBar::ScanProject();
		return NumGone;
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
