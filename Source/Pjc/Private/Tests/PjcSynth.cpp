// Copyright Ashot Barkhudaryan. All Rights Reserved.

#include "Tests/PjcTestFixture.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "AssetRegistry/AssetRegistryModule.h"
#include "Curves/CurveFloat.h"
#include "Curves/CurveLinearColor.h"
#include "Engine/ObjectLibrary.h"
#include "Engine/PrimaryAssetLabel.h"
#include "FileHelpers.h"
#include "Math/RandomStream.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"

namespace
{
	/** What one kit (character, environment set, prop group, ...) is made of, counts are min..max */
	struct FPjcSynthKitShape
	{
		const TCHAR* Area;
		const TCHAR* Group;
		int32 Weight;
		FIntPoint Textures;
		FIntPoint Materials;
		FIntPoint Meshes;
		FIntPoint Extras;
		const TCHAR* ExtraPrefix;
		float ExtraSizeKb;
		FIntPoint Blueprints;
	};

	// clang-format off
	const FPjcSynthKitShape KitShapes[] = {
		{TEXT("Environment"), TEXT("Forest"),      12, {10, 30}, {5, 15}, {10, 40}, {0, 0},   TEXT(""),     0.0f, {2, 6}},
		{TEXT("Environment"), TEXT("Desert"),       8, {10, 30}, {5, 15}, {10, 40}, {0, 0},   TEXT(""),     0.0f, {2, 6}},
		{TEXT("Environment"), TEXT("Town"),        10, {10, 30}, {5, 15}, {10, 40}, {0, 0},   TEXT(""),     0.0f, {2, 6}},
		{TEXT("Characters"),  TEXT("Heroes"),       6, {6, 12},  {2, 4},  {1, 2},   {10, 40}, TEXT("A_"),  24.0f, {1, 2}},
		{TEXT("Characters"),  TEXT("Enemies"),      8, {6, 12},  {2, 4},  {1, 2},   {10, 40}, TEXT("A_"),  24.0f, {1, 2}},
		{TEXT("Characters"),  TEXT("NPC"),          4, {4, 8},   {1, 3},  {1, 2},   {5, 15},  TEXT("A_"),  24.0f, {1, 1}},
		{TEXT("Props"),       TEXT("Furniture"),    7, {4, 12},  {2, 6},  {5, 20},  {0, 0},   TEXT(""),     0.0f, {1, 5}},
		{TEXT("Props"),       TEXT("Industrial"),   7, {4, 12},  {2, 6},  {5, 20},  {0, 0},   TEXT(""),     0.0f, {1, 5}},
		{TEXT("Weapons"),     TEXT("Ranged"),       5, {4, 8},   {2, 3},  {1, 3},   {5, 15},  TEXT("S_"),  40.0f, {1, 2}},
		{TEXT("Weapons"),     TEXT("Melee"),        3, {4, 8},   {2, 3},  {1, 3},   {3, 8},   TEXT("S_"),  40.0f, {1, 2}},
		{TEXT("Vehicles"),    TEXT("Cars"),         4, {6, 14},  {3, 6},  {2, 6},   {4, 10},  TEXT("S_"),  40.0f, {1, 2}},
		{TEXT("Audio"),       TEXT("Ambience"),     4, {0, 0},   {0, 0},  {0, 0},   {10, 40}, TEXT("S_"),  60.0f, {2, 6}},
		{TEXT("Audio"),       TEXT("Music"),        2, {0, 0},   {0, 0},  {0, 0},   {4, 12},  TEXT("S_"), 200.0f, {1, 3}},
		{TEXT("UI"),          TEXT("Menus"),        5, {5, 20},  {1, 3},  {0, 0},   {0, 0},   TEXT(""),     0.0f, {1, 4}},
		{TEXT("UI"),          TEXT("HUD"),          3, {5, 20},  {1, 3},  {0, 0},   {0, 0},   TEXT(""),     0.0f, {1, 4}},
		{TEXT("FX"),          TEXT("Impacts"),      4, {3, 8},   {2, 4},  {0, 0},   {2, 6},   TEXT("NS_"),  8.0f, {1, 3}},
		{TEXT("FX"),          TEXT("Weather"),      2, {3, 8},   {2, 4},  {0, 0},   {2, 6},   TEXT("NS_"),  8.0f, {1, 3}},
		{TEXT("Developers"),  TEXT("Tests"),        2, {2, 8},   {1, 4},  {1, 6},   {0, 0},   TEXT(""),     0.0f, {1, 3}},
		{TEXT("Old"),         TEXT("Prototype"),    3, {6, 20},  {3, 10}, {5, 25},  {0, 0},   TEXT(""),     0.0f, {1, 4}},
	};
	// clang-format on

	/**
	 * Game like project made of cheap assets: curves stand in for textures, sounds and animations (size set by key count),
	 * object libraries for materials, meshes and blueprints (hard references), primary asset labels for maps.
	 * Kits use shared textures and materials, maps place blueprints of used kits. Unused: old and developer content,
	 * some whole kits, material variants and leftover textures.
	 */
	class FPjcSynth
	{
	  public:

		FPjcSynth(const int32 InNumAssets, const int32 Seed) : Random(Seed), NumAssetsMax(InNumAssets) {}

		bool Generate(FAutomationTestBase& Test) {
			// shared content first, kits use it
			const int32 NumShared = FMath::Max(20, NumAssetsMax / 150);
			for (int32 Index = 0; Index < NumShared; ++Index) {
				SharedTextures.Add(
					NewCurve(TEXT("Shared/Textures"), FString::Printf(TEXT("T_Shared_%04d"), Index), RandomSize(16.0f, 1.0f, 2048.0f))
				);
			}
			for (int32 Index = 0; Index < FMath::Max(5, NumAssetsMax / 600); ++Index) {
				SharedMaterials.Add(
					NewLibrary(TEXT("Shared/Materials"), FString::Printf(TEXT("M_Master_%03d"), Index), PickSome(SharedTextures, 1, 3))
				);
			}

			int32 TotalWeight = 0;
			for (const auto& Shape : KitShapes) {
				TotalWeight += Shape.Weight;
			}

			TMap<FString, int32> KitCounts;
			while (NumAssets < NumAssetsMax * 0.97f) {
				int32 Roll = Random.RandRange(0, TotalWeight - 1);
				const FPjcSynthKitShape* Shape = &KitShapes[0];
				for (const auto& Candidate : KitShapes) {
					Shape = &Candidate;
					Roll -= Candidate.Weight;
					if (Roll < 0) break;
				}

				const FString Group = FString::Printf(TEXT("%s/%s"), Shape->Area, Shape->Group);
				const int32 KitIndex = KitCounts.FindOrAdd(Group)++;
				const bool bOldContent = FCString::Strcmp(Shape->Area, TEXT("Old")) == 0 || FCString::Strcmp(Shape->Area, TEXT("Developers")) == 0;
				Kit(*Shape, Group, FString::Printf(TEXT("%s_%03d"), Shape->Group, KitIndex), !bOldContent && Random.FRand() > 0.1f);

				if (Pending.Num() >= 2000 && !Flush(Test)) return false;
			}

			// maps place blueprints and some meshes of used kits, every used kit is in at least one map
			const int32 NumMaps = FMath::Max(3, NumAssetsMax / 1500);
			TArray<TArray<UObject*>> MapContents;
			MapContents.SetNum(NumMaps);
			for (int32 KitIndex = 0; KitIndex < Placeable.Num(); ++KitIndex) {
				MapContents[KitIndex % NumMaps].Append(Placeable[KitIndex]);
				if (Random.FRand() < 0.3f) {
					MapContents[Random.RandRange(0, NumMaps - 1)].Append(Placeable[KitIndex]);
				}
			}
			for (int32 Map = 0; Map < NumMaps; ++Map) {
				NewMap(FString::Printf(TEXT("Maps/Region_%02d"), Map / 4), FString::Printf(TEXT("L_Map_%03d"), Map), MapContents[Map]);
			}

			if (!Flush(Test)) return false;

			Test.AddInfo(FString::Printf(TEXT("Generated %d assets under %s"), NumAssets, *Root));
			return true;
		}

	  private:

		void Kit(const FPjcSynthKitShape& Shape, const FString& Group, const FString& Name, const bool bUsed) {
			const FString Path = Group / Name;

			TArray<UObject*> Textures;
			const int32 NumTextures = RandomCount(Shape.Textures);
			for (int32 Index = 0; Index < NumTextures; ++Index) {
				Textures.Add(NewCurve(Path / TEXT("Textures"), FString::Printf(TEXT("T_%s_%02d"), *Name, Index), RandomSize(20.0f, 1.0f, 2048.0f)));
			}

			// some materials are variants nothing uses
			TArray<UObject*> Materials;
			const int32 NumMaterials = RandomCount(Shape.Materials);
			for (int32 Index = 0; Index < NumMaterials; ++Index) {
				TArray<UObject*> Uses = PickSome(Textures, 1, 3);
				if (Random.FRand() < 0.6f) {
					Uses.Add(Pick(SharedMaterials));
				}
				if (Random.FRand() < 0.3f) {
					Uses.Add(Pick(SharedTextures));
				}

				UObject* Material = NewLibrary(Path / TEXT("Materials"), FString::Printf(TEXT("M_%s_%02d"), *Name, Index), Uses);
				if (Random.FRand() > 0.15f || Materials.Num() == 0) {
					Materials.Add(Material);
				}
			}

			TArray<UObject*> Meshes;
			const int32 NumMeshes = RandomCount(Shape.Meshes);
			for (int32 Index = 0; Index < NumMeshes; ++Index) {
				Meshes.Add(NewLibrary(Path / TEXT("Meshes"), FString::Printf(TEXT("SM_%s_%02d"), *Name, Index), PickSome(Materials, 1, 2)));
			}

			TArray<UObject*> Extras;
			const int32 NumExtras = RandomCount(Shape.Extras);
			for (int32 Index = 0; Index < NumExtras; ++Index) {
				const FString ExtraName = FString::Printf(TEXT("%s%s_%02d"), Shape.ExtraPrefix, *Name, Index);
				if (FCString::Strcmp(Shape.ExtraPrefix, TEXT("NS_")) == 0) {
					TArray<UObject*> Uses = PickSome(Textures, 1, 2);
					Uses.Append(PickSome(Materials, 0, 1));
					Extras.Add(NewLibrary(Path / TEXT("FX"), ExtraName, Uses));
				}
				else {
					const TCHAR* Folder = FCString::Strcmp(Shape.ExtraPrefix, TEXT("A_")) == 0 ? TEXT("Animations") : TEXT("Audio");
					Extras.Add(NewCurve(Path / Folder, ExtraName, RandomSize(Shape.ExtraSizeKb, 0.8f, 4096.0f)));
				}
			}

			// blueprints hold the kit together, UI screens are widgets
			TArray<UObject*> Placed;
			const bool bWidgets = FCString::Strcmp(Shape.Area, TEXT("UI")) == 0;
			const int32 NumBlueprints = RandomCount(Shape.Blueprints);
			for (int32 Index = 0; Index < NumBlueprints; ++Index) {
				TArray<UObject*> Uses = PickSome(Meshes, 1, 3);
				if (bWidgets) {
					Uses.Append(PickSome(Textures, 1, 6));
				}
				if (Uses.Num() == 0) {
					Uses.Append(PickSome(Materials, 1, 2));
				}

				// first blueprint takes most animations and sounds of kit, like an anim blueprint or sound cue would
				for (UObject* Extra : Extras) {
					if (Index == 0 ? Random.FRand() < 0.85f : Random.FRand() < 0.1f) {
						Uses.AddUnique(Extra);
					}
				}
				Placed.Add(
					NewLibrary(Path / TEXT("Blueprints"), FString::Printf(TEXT("%s%s_%02d"), bWidgets ? TEXT("W_") : TEXT("BP_"), *Name, Index), Uses)
				);
			}

			// maps place most meshes directly
			for (UObject* Mesh : Meshes) {
				if (Random.FRand() < 0.85f) {
					Placed.Add(Mesh);
				}
			}

			// leftovers nobody uses
			if (Random.FRand() < 0.3f) {
				const int32 NumLeftovers = Random.RandRange(1, 3);
				for (int32 Index = 0; Index < NumLeftovers; ++Index) {
					NewCurve(Path / TEXT("Textures"), FString::Printf(TEXT("T_%s_Old_%02d"), *Name, Index), RandomSize(20.0f, 1.0f, 2048.0f));
				}
			}

			if (bUsed && Placed.Num() > 0) {
				Placeable.Add(Placed);
			}
		}

		UObject* NewCurve(const FString& Folder, const FString& Name, const int64 SizeBytes) {
			UCurveFloat* Curve = NewAsset<UCurveFloat>(Folder, Name);

			// serialized key takes about 27 bytes
			const int32 NumKeys = static_cast<int32>(FMath::Max<int64>(1, SizeBytes / 27));
			for (int32 Key = 0; Key < NumKeys; ++Key) {
				Curve->FloatCurve.AddKey(static_cast<float>(Key), Random.FRand());
			}
			return Curve;
		}

		UObject* NewLibrary(const FString& Folder, const FString& Name, const TArray<UObject*>& Objects) {
			UObjectLibrary* Library = NewAsset<UObjectLibrary>(Folder, Name);
			Library->ObjectBaseClass = UObject::StaticClass();
			for (UObject* Object : Objects) {
				if (Object) {
					Library->AddObject(Object);
				}
			}
			return Library;
		}

		void NewMap(const FString& Folder, const FString& Name, const TArray<UObject*>& Objects) {
			UPrimaryAssetLabel* Label = NewAsset<UPrimaryAssetLabel>(Folder, Name);
			for (UObject* Object : Objects) {
				Label->ExplicitAssets.Add(TSoftObjectPtr<UObject> {Object});
			}
		}

		template<typename T>
		T* NewAsset(const FString& Folder, const FString& Name) {
			UPackage* Package = CreatePackage(*(Root / Folder / Name));
			T* Asset = NewObject<T>(Package, T::StaticClass(), FName {*Name}, RF_Public | RF_Standalone);
			FAssetRegistryModule::AssetCreated(Asset);
			Package->MarkPackageDirty();
			Pending.Add(Package);
			++NumAssets;
			return Asset;
		}

		bool Flush(FAutomationTestBase& Test) {
			if (Pending.Num() > 0 && !UEditorLoadingAndSavingUtils::SavePackages(Pending, false)) {
				Test.AddError(TEXT("Failed to save synthetic packages"));
				return false;
			}
			Pending.Reset();
			return true;
		}

		/** Log normal around median, most assets small, few big */
		int64 RandomSize(const float MedianKb, const float Spread, const float MaxKb) {
			const float U1 = FMath::Max(Random.FRand(), 1e-6f);
			const float U2 = Random.FRand();
			const float Normal = FMath::Sqrt(-2.0f * FMath::Loge(U1)) * FMath::Cos(6.2831853f * U2);
			return static_cast<int64>(FMath::Min(MedianKb * FMath::Exp(Spread * Normal), MaxKb) * 1024.0f);
		}

		int32 RandomCount(const FIntPoint& Range) {
			return Range.Y <= 0 ? 0 : Random.RandRange(Range.X, Range.Y);
		}

		UObject* Pick(const TArray<UObject*>& From) {
			return From.Num() > 0 ? From[Random.RandRange(0, From.Num() - 1)] : nullptr;
		}

		TArray<UObject*> PickSome(const TArray<UObject*>& From, const int32 Min, const int32 Max) {
			TArray<UObject*> Result;
			const int32 Num = FMath::Min(From.Num(), Random.RandRange(Min, Max));
			while (Result.Num() < Num) {
				Result.AddUnique(Pick(From));
			}
			return Result;
		}

		const FString Root = TEXT("/Game/PjcSynth");
		FRandomStream Random;
		int32 NumAssetsMax = 0;
		int32 NumAssets = 0;
		TArray<UPackage*> Pending;
		TArray<UObject*> SharedTextures;
		TArray<UObject*> SharedMaterials;
		TArray<TArray<UObject*>> Placeable;
	};
}	// namespace

/**
 * Not part of ProjectCleaner.* tests. Fills /Game/PjcSynth with a large game like project (Scripts/pjc.py synth),
 * -PjcSynthAssets=N sets asset count, -PjcSynthSeed=N makes another project of same size.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPjcFixtureSynth, "PjcFixture.Synth", PJC_TEST_FLAGS)

bool FPjcFixtureSynth::RunTest(const FString& Parameters) {
	if (!FPjcTestFixture::IsTestHost()) {
		AddError(TEXT("Synthetic project is generated only in test host projects (-PjcTestHost)"));
		return false;
	}

	int32 NumAssets = 30000;
	int32 Seed = 1;
	FParse::Value(FCommandLine::Get(), TEXT("PjcSynthAssets="), NumAssets);
	FParse::Value(FCommandLine::Get(), TEXT("PjcSynthSeed="), Seed);

	FPjcSynth Synth {FMath::Max(100, NumAssets), Seed};
	return Synth.Generate(*this);
}

#endif
