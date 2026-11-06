// Copyright Ashot Barkhudaryan. All Rights Reserved.

#include "PjcOverview.h"
#include "PjcShim.h"

FPjcOverview FPjcOverview::Make(const FPjcScanResult& Result) {
	FPjcOverview Overview;
	TMap<EPjcUsageReason, FTotal> Reasons;
	TMap<FName, FClass> Classes;

	for (const auto& Asset : Result.Assets) {
		const int64 Size = Result.GetSize(Asset.PackageName);
		const EPjcAssetStatus Status = Result.GetStatus(Asset.PackageName);
		Overview.All.Add(Size);

		const FName ClassName = PjcShim::GetAssetClassName(Asset);
		FClass& Class = Classes.FindOrAdd(ClassName);
		Class.Name = ClassName;
		Class.Total.Add(Size);

		switch (Status) {
			case EPjcAssetStatus::Used :
				Overview.Used.Add(Size);
				Class.SizeUsed += Size;
				break;
			case EPjcAssetStatus::EditorOnly :
				Overview.EditorOnly.Add(Size);
				Class.SizeEditorOnly += Size;
				break;
			case EPjcAssetStatus::Unused :
				Overview.Unused.Add(Size);
				Class.SizeUnused += Size;
				break;
		}

		const EPjcUsageReason Reason = Status == EPjcAssetStatus::Unused ? EPjcUsageReason::None : GetGroupReason(Result, Asset.PackageName);
		Reasons.FindOrAdd(Reason).Add(Size);
	}

	for (const auto& Pair : Reasons) {
		Overview.Reasons.Add(FReason {Pair.Key, Pair.Value});
	}
	Overview.Reasons.Sort([](const FReason& A, const FReason& B) {
		return A.Total.Size != B.Total.Size ? A.Total.Size > B.Total.Size : A.Total.Num > B.Total.Num;
	});

	Classes.GenerateValueArray(Overview.Classes);
	Overview.Classes.Sort([](const FClass& A, const FClass& B) {
		return A.Total.Size != B.Total.Size ? A.Total.Size > B.Total.Size : A.Name.LexicalLess(B.Name);
	});

	return Overview;
}

EPjcUsageReason FPjcOverview::GetGroupReason(const FPjcScanResult& Result, const FName Package) {
	const EPjcUsageReason Reason = Result.GetChainReason(Package);
	return Reason == EPjcUsageReason::FolderLabelAsset ? EPjcUsageReason::Primary : Reason;
}
