// Copyright Ashot Barkhudaryan. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

// Numbers as shown in the plugin UI
namespace PjcFormat
{
	// Count with thousand separators, e.g. 29,167
	inline FString Count(const int64 Num) {
		return FText::AsNumber(Num).ToString();
	}

	// Size on disk with three significant digits, e.g. 512 B, 10.1 KiB, 425 KiB, 1.07 MiB
	inline FString Size(const int64 Bytes) {
		static const TCHAR* Units[] = {TEXT("B"), TEXT("KiB"), TEXT("MiB"), TEXT("GiB"), TEXT("TiB")};

		double Value = static_cast<double>(FMath::Max<int64>(Bytes, 0));
		int32 Unit = 0;
		while (Value >= 999.5 && Unit + 1 < UE_ARRAY_COUNT(Units)) {
			Value /= 1024.0;
			++Unit;
		}

		const int32 Digits = Unit == 0 ? 0 : (Value < 9.995 ? 2 : (Value < 99.95 ? 1 : 0));
		FNumberFormattingOptions Options;
		Options.SetUseGrouping(true).SetMinimumFractionalDigits(Digits).SetMaximumFractionalDigits(Digits);
		return FText::AsNumber(Value, &Options).ToString() + TEXT(" ") + Units[Unit];
	}

	// Local date and time of a UTC time, e.g. Nov 22, 15:48 or Nov 22, 2026, 15:48 (FDateTime::ToString has no month names in UE 4.27)
	inline FString DateTime(const FDateTime& UtcTime, const bool bWithYear) {
		static const TCHAR* Months[] = {
		  TEXT("Jan"),
		  TEXT("Feb"),
		  TEXT("Mar"),
		  TEXT("Apr"),
		  TEXT("May"),
		  TEXT("Jun"),
		  TEXT("Jul"),
		  TEXT("Aug"),
		  TEXT("Sep"),
		  TEXT("Oct"),
		  TEXT("Nov"),
		  TEXT("Dec")
		};
		const FDateTime Local = UtcTime + (FDateTime::Now() - FDateTime::UtcNow());
		const FString Year = bWithYear ? FString::Printf(TEXT(", %d"), Local.GetYear()) : FString {};
		return FString::Printf(TEXT("%s %d%s, %02d:%02d"), Months[Local.GetMonth() - 1], Local.GetDay(), *Year, Local.GetHour(), Local.GetMinute());
	}

	// Share in whole percent, never rounded to 0% or 100% when it is not, e.g. <1%, 58%, 99%
	inline FString Percent(const int64 Part, const int64 Total) {
		if (Total <= 0 || Part <= 0) return TEXT("0%");
		if (Part >= Total) return TEXT("100%");

		const double Share = static_cast<double>(Part) / static_cast<double>(Total);
		const int32 Rounded = FMath::Clamp(FMath::RoundToInt(100.0f * static_cast<float>(Share)), 1, 99);
		return Part * 100 < Total ? FString {TEXT("<1%")} : FString::Printf(TEXT("%d%%"), Rounded);
	}
}	// namespace PjcFormat
