// Copyright Ashot Barkhudaryan. All Rights Reserved.

#include "PjcOverview.h"

FPjcOverview FPjcOverview::Make(const FPjcScanResult& Result) {
	FPjcOverview Overview;

	for (const auto& Asset : Result.Assets) {
		const int64 Size = Result.GetSize(Asset.PackageName);
		Overview.All.Add(Size);

		switch (Result.GetStatus(Asset.PackageName)) {
			case EPjcAssetStatus::Used :	   Overview.Used.Add(Size); break;
			case EPjcAssetStatus::EditorOnly : Overview.EditorOnly.Add(Size); break;
			case EPjcAssetStatus::Unused :	   Overview.Unused.Add(Size); break;
		}
	}

	return Overview;
}
