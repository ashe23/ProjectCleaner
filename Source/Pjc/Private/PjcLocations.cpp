// Copyright Ashot Barkhudaryan. All Rights Reserved.

#include "PjcLocations.h"
#include "PjcInspector.h"
#include "PjcShim.h"
#include "PjcSubsystem.h"
// Engine Headers
#include "Components/ActorComponent.h"
#include "EdGraph/EdGraph.h"
#include "EdGraph/EdGraphNode.h"
#include "EdGraph/EdGraphPin.h"
#include "Engine/Blueprint.h"
#include "Engine/BlueprintGeneratedClass.h"
#include "Misc/PackageName.h"
#include "PackageTools.h"
#include "UObject/Package.h"
#include "UObject/UnrealType.h"
#include "UObject/UObjectHash.h"

namespace
{
	// unity builds join files: names unique to this file
	constexpr int32 PjcLocationsMaxDepth = 10;
	constexpr int32 PjcLocationsMaxPlaces = 5000;

	// Walks the properties of the objects of one package and notes every reference to another package
	struct FPjcLocationWalker
	{
		FName Self;
		TArray<FPjcLocation>* Out = nullptr;
		TSet<FString> Seen;
		// the object walked now is the class defaults of a Blueprint
		bool bClassDefaults = false;

		void Add(const FString& PackageName, const TArray<FString>& Parts, const FString& Property) {
			if (PackageName.IsEmpty() || PackageName.StartsWith(TEXT("/Script/")) || PackageName.StartsWith(TEXT("/Temp/"))) return;

			const FName Target {*PackageName};
			if (Target == Self || Out->Num() >= PjcLocationsMaxPlaces) return;

			const FString Key = PackageName + TEXT("|") + FString::Join(Parts, TEXT("|"));
			if (Seen.Contains(Key)) return;

			Seen.Add(Key);
			FPjcLocation& Place = Out->AddDefaulted_GetRef();
			Place.Target = Target;
			Place.Parts = Parts;
			Place.Property = Property;
		}

		void AddObject(const UObject* Object, const TArray<FString>& Parts, const FString& Property) {
			if (!Object) return;

			const UPackage* Package = Object->GetOutermost();
			if (Package && !Package->HasAnyFlags(RF_Transient)) Add(Package->GetName(), Parts, Property);
		}

		void Value(const FProperty* Property, const void* Data, TArray<FString>& Parts, const FString& Label, const int32 Depth) {
			if (!Property || !Data || Depth > PjcLocationsMaxDepth) return;
			// not what the user set: old names kept for loading, and lists the editor works out from the rest. Transient
			// properties stay: a mesh keeps its materials in one
			if (Property->HasAnyPropertyFlags(CPF_Deprecated)) return;
			static const TSet<FName> Derived {
			  TEXT("CachedDependencies"),
			  TEXT("TextureStreamingData"),
			  TEXT("TextureStreamingDataMissingEntries"),
			  TEXT("CachedExpressionData"),
			  TEXT("CachedReferencedTextures"),
			  TEXT("ReferencedTextures"),
			  TEXT("ReferencedDefaultTextures"),
			  TEXT("FunctionInputs"),
			  TEXT("FunctionOutputs")
			};
			if (Derived.Contains(Property->GetFName())) return;

			if (const FSoftObjectProperty* Soft = CastField<FSoftObjectProperty>(Property)) {
				Add(Soft->GetPropertyValue(Data).ToSoftObjectPath().GetLongPackageName(), Parts, Label);
				return;
			}
			if (const FObjectPropertyBase* Object = CastField<FObjectPropertyBase>(Property)) {
				AddObject(Object->GetObjectPropertyValue(Data), Parts, Label);
				return;
			}
			if (const FStructProperty* Struct = CastField<FStructProperty>(Property)) {
				if (Struct->Struct == TBaseStructure<FSoftObjectPath>::Get() || Struct->Struct == TBaseStructure<FSoftClassPath>::Get()) {
					Add(static_cast<const FSoftObjectPath*>(Data)->GetLongPackageName(), Parts, Label);
					return;
				}
				// the compiled copy of an anim graph node: the node in the graph says the same, by its name
				if (Struct->Struct->GetName().StartsWith(TEXT("AnimNode_")) && Depth == 0 && bClassDefaults) return;

				// "Node" is the property every anim graph node keeps its values in: no part of its own
				const bool bOwnPart = Label != TEXT("Node");
				if (bOwnPart) Parts.Add(Label);
				for (TFieldIterator<FProperty> It {Struct->Struct}; It; ++It) {
					Value(*It, It->ContainerPtrToValuePtr<void>(Data), Parts, LabelOf(*It), Depth + 1);
				}
				if (bOwnPart) Parts.Pop();
				return;
			}
			if (const FArrayProperty* Array = CastField<FArrayProperty>(Property)) {
				FScriptArrayHelper Helper {Array, Data};
				for (int32 Index = 0; Index < Helper.Num(); ++Index) {
					Value(Array->Inner, Helper.GetRawPtr(Index), Parts, FString::Printf(TEXT("%s[%d]"), *Label, Index), Depth + 1);
				}
				return;
			}
			if (const FSetProperty* Set = CastField<FSetProperty>(Property)) {
				FScriptSetHelper Helper {Set, Data};
				for (int32 Index = 0; Index < Helper.GetMaxIndex(); ++Index) {
					if (Helper.IsValidIndex(Index)) Value(Set->ElementProp, Helper.GetElementPtr(Index), Parts, Label, Depth + 1);
				}
				return;
			}
			if (const FMapProperty* Map = CastField<FMapProperty>(Property)) {
				FScriptMapHelper Helper {Map, Data};
				for (int32 Index = 0; Index < Helper.GetMaxIndex(); ++Index) {
					if (!Helper.IsValidIndex(Index)) continue;

					Value(Map->KeyProp, Helper.GetKeyPtr(Index), Parts, Label + TEXT(" (key)"), Depth + 1);
					Value(Map->ValueProp, Helper.GetValuePtr(Index), Parts, Label, Depth + 1);
				}
			}
		}

		static FString LabelOf(const FProperty* Property) {
			const FString Label = Property->GetDisplayNameText().ToString();
			return Label.IsEmpty() ? Property->GetName() : Label;
		}
	};

	// Name of one object on the way from the asset to a property: what the editor calls it
	FString PjcLocationsPartOf(const UObject* Object) {
		if (Object->HasAnyFlags(RF_ClassDefaultObject)) return TEXT("Class defaults");
		if (Object->IsA<UBlueprintGeneratedClass>()) return TEXT("Class");
		if (const UEdGraphNode* Node = Cast<UEdGraphNode>(Object)) {
			FString Title = Node->GetNodeTitle(ENodeTitleType::ListView).ToString();
			Title.ReplaceInline(TEXT("\r"), TEXT(""));
			Title.ReplaceInline(TEXT("\n"), TEXT(" "));
			return Title.IsEmpty() ? Node->GetName() : Title;
		}
		FString Name = Object->GetName();
		Name.RemoveFromEnd(TEXT("_GEN_VARIABLE"));
		// a node of a material graph is named after its class
		if (Name.RemoveFromStart(TEXT("MaterialExpression"))) Name.ReplaceInline(TEXT("_"), TEXT(" "));
		return Name;
	}

	// The parts from the package down to the object. The asset itself has no part of its own
	void PjcLocationsPartsOf(const UObject* Object, const UPackage* Package, TArray<FString>& OutParts) {
		TArray<const UObject*> Chain;
		for (const UObject* Step = Object; Step && Step != Package; Step = Step->GetOuter()) {
			Chain.Insert(Step, 0);
		}
		for (int32 Index = 0; Index < Chain.Num(); ++Index) {
			const UObject* Step = Chain[Index];
			// the asset itself is no part; of a Blueprint only when the property is its own
			const bool bAsset = Index == 0 && Step->IsAsset();
			if (bAsset && (!Step->IsA<UBlueprint>() || Chain.Num() > 1)) continue;
			if (Step->IsA<UActorComponent>() && Index > 0 && Chain[Index - 1]->IsA<UClass>()) {
				// a component of a Blueprint: under Components, not under its class
				OutParts.Reset();
				OutParts.Add(TEXT("Components"));
			}
			if (Step->GetName().StartsWith(TEXT("MaterialExpression")) && OutParts.Num() == 0) OutParts.Add(TEXT("Material graph"));
			const FString Part = bAsset ? FString {TEXT("Blueprint")} : PjcLocationsPartOf(Step);
			// a state and the graph inside it have one name
			if (OutParts.Num() == 0 || OutParts.Last() != Part) OutParts.Add(Part);
		}
		if (OutParts.Num() == 0) OutParts.Add(TEXT("Asset"));
	}

	bool PjcLocationsSkip(const UObject* Object) {
		if (!Object || Object->IsA<UPackage>() || Object->HasAnyFlags(RF_Transient)) return true;
		// what the compiler makes from the graphs: classes, functions, bindings. The graphs say it where the user set it
		if (Object->IsA<UStruct>() || Object->GetClass()->GetName().EndsWith(TEXT("DelegateBinding"))) return true;

		// classes the Blueprint compiler keeps beside the real one
		for (const UObject* Step = Object; Step; Step = Step->GetOuter()) {
			const FString Name = Step->GetName();
			if (Name.StartsWith(TEXT("SKEL_")) || Name.StartsWith(TEXT("REINST_")) || Name.StartsWith(TEXT("Default__SKEL_"))) return true;
		}
		return false;
	}
}	// namespace

bool FPjcLocationReader::CanRead(const FName Package) {
	if (Package.IsNone() || FPjcInspectorModel::IsExternal(Package)) return false;

	TArray<FAssetData> Assets;
	UPjcSubsystem::GetModuleAssetRegistry().Get().GetAssetsByPackageName(Package, Assets);
	if (Assets.Num() == 0) return false;

	for (const FAssetData& Asset : Assets) {
		// the class is a bare name before UE 5.1 and a path after
		const FString ClassName = PjcShim::GetAssetClassName(Asset).ToString();
		if (ClassName == TEXT("World") || ClassName.EndsWith(TEXT(".World"))) return false;
	}
	return true;
}

bool FPjcLocationReader::IsLoaded(const FName Package) {
	const UPackage* Loaded = FindPackage(nullptr, *Package.ToString());
	return Loaded && Loaded->IsFullyLoaded();
}

int64 FPjcLocationReader::GetLoadSize(const FName Package) {
	// the package and everything it loads with it (hard references) that is not in memory yet
	const FPjcScanResult& Result = UPjcSubsystem::GetScanResult();
	IAssetRegistry& Registry = UPjcSubsystem::GetModuleAssetRegistry().Get();
	TArray<FName> Queue {Package};
	TSet<FName> Seen {Package};
	TArray<FAssetDependency> Dependencies;
	int64 Size = 0;
	for (int32 Head = 0; Head < Queue.Num() && Head < 4000; ++Head) {
		const FName Current = Queue[Head];
		if (IsLoaded(Current)) continue;

		Size += Result.GetSize(Current);
		Dependencies.Reset();
		Registry.GetDependencies(FAssetIdentifier {Current}, Dependencies, UE::AssetRegistry::EDependencyCategory::Package);
		for (const FAssetDependency& Dependency : Dependencies) {
			const FName Name = Dependency.AssetId.PackageName;
			if (!EnumHasAnyFlags(Dependency.Properties, UE::AssetRegistry::EDependencyProperty::Hard) || Seen.Contains(Name)) continue;
			if (Name.ToString().StartsWith(TEXT("/Script/"))) continue;

			Seen.Add(Name);
			Queue.Add(Name);
		}
	}
	return Size;
}

bool FPjcLocationReader::Read(const FName Package, FPjcLocations& Out) {
	Out = FPjcLocations {};
	if (!CanRead(Package)) return false;

	Out.bLoadedHere = !IsLoaded(Package);
	UPackage* Loaded = LoadPackage(nullptr, *Package.ToString(), LOAD_None);
	if (!Loaded) return false;

	FPjcLocationWalker Walker;
	Walker.Self = Package;
	Walker.Out = &Out.Places;

	TArray<UObject*> Objects;
	GetObjectsWithOuter(Loaded, Objects);
	TArray<FString> Parts;
	for (const UObject* Object : Objects) {
		if (PjcLocationsSkip(Object)) continue;

		Parts.Reset();
		PjcLocationsPartsOf(Object, Loaded, Parts);
		Walker.bClassDefaults = Object->HasAnyFlags(RF_ClassDefaultObject);
		for (TFieldIterator<FProperty> It {Object->GetClass()}; It; ++It) {
			Walker.Value(*It, It->ContainerPtrToValuePtr<void>(Object), Parts, FPjcLocationWalker::LabelOf(*It), 0);
		}
		// a node of a graph holds assets on its pins too, they are not properties
		if (const UEdGraphNode* Node = Cast<UEdGraphNode>(Object)) {
			for (const UEdGraphPin* Pin : Node->Pins) {
				if (!Pin || !Pin->DefaultObject) continue;

				const FString PinName = Pin->GetDisplayName().ToString();
				Walker.AddObject(Pin->DefaultObject, Parts, PinName.IsEmpty() ? Pin->PinName.ToString() : PinName);
			}
		}
	}

	// what the asset registry lists and no property holds
	TSet<FName> Found;
	for (const FPjcLocation& Place : Out.Places) {
		Found.Add(Place.Target);
	}
	TArray<FPjcInspectorLink> Links;
	int32 Through = 0;
	FPjcInspectorModel::GetLinks(Package, false, Links, Through);
	TSet<FName> Listed;
	for (const FPjcInspectorLink& Link : Links) {
		Listed.Add(Link.Package);
		if (!Found.Contains(Link.Package)) Out.NotFound.Add(Link.Package);
	}
	// the lists of the Inspector are the asset registry's: a place of something it does not list would have no row there
	Out.Places.RemoveAll([&Listed](const FPjcLocation& Place) {
		return !Listed.Contains(Place.Target);
	});
	return true;
}

int32 FPjcLocationReader::Unload(const TArray<FName>& Packages) {
	TArray<UPackage*> Loaded;
	for (const FName Package : Packages) {
		if (UPackage* Found = FindPackage(nullptr, *Package.ToString())) Loaded.Add(Found);
	}
	if (Loaded.Num() == 0) return 0;

	UPackageTools::UnloadPackages(Loaded);
	int32 Gone = 0;
	for (const FName Package : Packages) {
		Gone += IsLoaded(Package) ? 0 : 1;
	}
	return Gone;
}
