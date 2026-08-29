#include "B4DExporter.h"
#include "B4DActors.h"
#include "EngineUtils.h"
#include "Dom/JsonObject.h"
#include "Dom/JsonValue.h"
#include "Serialization/JsonWriter.h"
#include "Serialization/JsonSerializer.h"
#include "Misc/FileHelper.h"
#include "Engine/World.h"

namespace
{
	/** Trims float noise so exported maps diff cleanly between saves. */
	double Tidy(float Value)
	{
		const double Rounded = FMath::RoundToDouble(static_cast<double>(Value) * 10000.0) / 10000.0;
		return Rounded == 0.0 ? 0.0 : Rounded;
	}

	FString TypeName(EB4DObjectiveType Type)
	{
		switch (Type)
		{
		case EB4DObjectiveType::Breakers:    return TEXT("breakers");
		case EB4DObjectiveType::Fuel:        return TEXT("fuel");
		case EB4DObjectiveType::Triangulate: return TEXT("triangulate");
		case EB4DObjectiveType::Escort:      return TEXT("escort");
		default:                             return TEXT("signal");
		}
	}

	FString TypeName(EB4DPropType Type)
	{
		switch (Type)
		{
		case EB4DPropType::Cylinder:    return TEXT("cylinder");
		case EB4DPropType::Grid:        return TEXT("grid");
		case EB4DPropType::ChainLine:   return TEXT("chainLine");
		case EB4DPropType::CarcassRows: return TEXT("carcassRows");
		case EB4DPropType::LightPole:   return TEXT("lightPole");
		case EB4DPropType::Vat:         return TEXT("vat");
		case EB4DPropType::PipeRun:     return TEXT("pipeRun");
		default:                        return TEXT("box");
		}
	}

	int32 ToHex(const FColor& Colour)
	{
		return (Colour.R << 16) | (Colour.G << 8) | Colour.B;
	}

	/** Every actor of a type in the campaign's world. The campaign root is a
	 *  marker rather than a parent, so a level holds one campaign at a time. */
	template <typename T>
	TArray<T*> Gather(const AB4DCampaign* Campaign)
	{
		TArray<T*> Found;
		if (!Campaign || !Campaign->GetWorld())
		{
			return Found;
		}
		for (TActorIterator<T> It(Campaign->GetWorld()); It; ++It)
		{
			Found.Add(*It);
		}
		return Found;
	}

	/** Two props share a JSON entry when everything but their transform matches. */
	FString GroupKey(const AB4DProp* Prop)
	{
		TArray<FName> Keys;
		Prop->Values.GetKeys(Keys);
		Keys.Sort(FNameLexicalLess());

		FString Fields;
		for (const FName& Key : Keys)
		{
			Fields += FString::Printf(TEXT("%s=%f;"), *Key.ToString(), Prop->Values[Key]);
		}
		return FString::Printf(TEXT("%s|%s|%d|%f|%f|%s|%d|%s"),
			*TypeName(Prop->Type), *Prop->Material, Prop->bSolid ? 1 : 0,
			Prop->ColliderHalfExtents.X, Prop->ColliderHalfExtents.Y,
			*Prop->ColliderKind, Prop->bCastShadow ? 1 : 0, *Fields);
	}
}

AB4DCampaign* UB4DExporter::FindCampaign(UObject* WorldContext)
{
	const UWorld* World = GEngine ? GEngine->GetWorldFromContextObject(WorldContext, EGetWorldErrorMode::ReturnNull) : nullptr;
	if (!World)
	{
		return nullptr;
	}
	for (TActorIterator<AB4DCampaign> It(const_cast<UWorld*>(World)); It; ++It)
	{
		return *It;
	}
	return nullptr;
}

TArray<FB4DProblem> UB4DExporter::Validate(AB4DCampaign* Campaign)
{
	TArray<FB4DProblem> Problems;
	if (!Campaign)
	{
		Problems.Emplace(EB4DProblemLevel::Error, TEXT("no B4D Campaign actor in the level"));
		return Problems;
	}

	auto Error = [&Problems](const FString& Message, AActor* Context = nullptr)
	{
		Problems.Emplace(EB4DProblemLevel::Error, Message, Context);
	};
	auto Warn = [&Problems](const FString& Message, AActor* Context = nullptr)
	{
		Problems.Emplace(EB4DProblemLevel::Warning, Message, Context);
	};

	if (Campaign->Id.IsEmpty())    Error(TEXT("the campaign id is empty"), Campaign);
	if (Campaign->Theme.IsEmpty()) Error(TEXT("the campaign theme is empty"), Campaign);

	const TArray<AB4DZone*> Zones = Gather<AB4DZone>(Campaign);
	if (Zones.Num() == 0)
	{
		Error(TEXT("the campaign has no zones, so there is nowhere to walk"), Campaign);
		return Problems;
	}

	TSet<FString> SeenNames;
	for (AB4DZone* Zone : Zones)
	{
		if (Zone->ZoneName.IsEmpty())
		{
			Error(TEXT("a zone has no name"), Zone);
		}
		else if (SeenNames.Contains(Zone->ZoneName))
		{
			Error(FString::Printf(TEXT("two zones are both named \"%s\""), *Zone->ZoneName), Zone);
		}
		SeenNames.Add(Zone->ZoneName);

		if (Zone->GetHalfX() <= 0.f || Zone->GetHalfZ() <= 0.f)
		{
			Error(FString::Printf(TEXT("zone \"%s\" has no area"), *Zone->ZoneName), Zone);
		}
		if (Zone->Floor.IsEmpty())
		{
			Error(FString::Printf(TEXT("zone \"%s\" has no floor material"), *Zone->ZoneName), Zone);
		}
		if (FMath::Abs(Zone->GetActorLocation().Z) > 1.f)
		{
			Warn(FString::Printf(TEXT("zone \"%s\" is off the ground plane; only its X and Y are exported"), *Zone->ZoneName), Zone);
		}
	}

	// A zone touching nothing else can never be entered.
	if (Zones.Num() > 1)
	{
		for (AB4DZone* Zone : Zones)
		{
			bool bConnected = false;
			for (AB4DZone* Other : Zones)
			{
				if (Zone->ConnectsTo(Other)) { bConnected = true; break; }
			}
			if (!bConnected)
			{
				Error(FString::Printf(TEXT("zone \"%s\" overlaps no other zone, so it is unreachable"), *Zone->ZoneName), Zone);
			}
		}
	}

	auto InAnyZone = [&Zones](float GameX, float GameZ)
	{
		for (const AB4DZone* Zone : Zones)
		{
			if (Zone->ContainsGamePoint(GameX, GameZ)) return true;
		}
		return false;
	};

	const TArray<AB4DObjective*> Objectives = Gather<AB4DObjective>(Campaign);
	TMap<int32, int32> ChapterCounts;
	for (AB4DObjective* Objective : Objectives)
	{
		ChapterCounts.FindOrAdd(Objective->Chapter)++;
		const FVector2D Position = B4DSpace::ToGameXZ(Objective->GetActorLocation());

		if (Objective->Label.IsEmpty()) Error(FString::Printf(TEXT("the chapter %d objective has no label"), Objective->Chapter), Objective);
		if (Objective->Verb.IsEmpty())  Error(FString::Printf(TEXT("the chapter %d objective has no sign text"), Objective->Chapter), Objective);
		if (Objective->Kind.IsEmpty())  Error(FString::Printf(TEXT("the chapter %d objective has no kind"), Objective->Chapter), Objective);
		if (Objective->Duration <= 0.f) Error(FString::Printf(TEXT("objective \"%s\" has no duration"), *Objective->Label), Objective);

		if (Objective->Type == EB4DObjectiveType::Escort && !Objective->CartTo)
		{
			Error(FString::Printf(TEXT("objective \"%s\" is an escort but has no cart destination"), *Objective->Label), Objective);
		}
		if (Objective->Type == EB4DObjectiveType::Breakers && !Objective->Window.bSet)
		{
			Warn(FString::Printf(TEXT("objective \"%s\" is a breakers puzzle with no window, so the switches never have to line up"), *Objective->Label), Objective);
		}
		if (!InAnyZone(Position.X, Position.Y))
		{
			Warn(FString::Printf(TEXT("objective \"%s\" sits outside every zone and will be relocated to the nearest clear spot at load"), *Objective->Label), Objective);
		}
		for (const FB4DObjectiveNode& Node : Objective->Nodes)
		{
			if (!InAnyZone(Position.X + Node.DX, Position.Y + Node.DZ))
			{
				Warn(FString::Printf(TEXT("node \"%s\" of \"%s\" sits outside every zone and will be relocated at load"), *Node.Label, *Objective->Label), Objective);
			}
		}
	}

	for (const TPair<int32, int32>& Pair : ChapterCounts)
	{
		if (Pair.Value > 1)
		{
			Error(FString::Printf(TEXT("chapter %d has %d objectives, it can only have one"), Pair.Key, Pair.Value), Campaign);
		}
	}

	const TArray<AB4DGate*> Gates = Gather<AB4DGate>(Campaign);
	if (Objectives.Num() > 0 && Gates.Num() != Objectives.Num())
	{
		Warn(FString::Printf(TEXT("the map has %d objectives but %d gates; the game expects one gate per chapter"), Objectives.Num(), Gates.Num()), Campaign);
	}

	for (AB4DBarrel* Barrel : Gather<AB4DBarrel>(Campaign))
	{
		const FVector2D Position = B4DSpace::ToGameXZ(Barrel->GetActorLocation());
		if (!InAnyZone(Position.X, Position.Y))
		{
			Warn(FString::Printf(TEXT("a fuel barrel at %.1f, %.1f sits outside every zone and can never be shot"), Position.X, Position.Y), Barrel);
		}
	}

	for (AB4DDropHazard* Drop : Gather<AB4DDropHazard>(Campaign))
	{
		const FVector2D Position = B4DSpace::ToGameXZ(Drop->GetActorLocation());
		if (!InAnyZone(Position.X, Position.Y))
		{
			Warn(FString::Printf(TEXT("drop hazard \"%s\" at %.1f, %.1f sits outside every zone and can never be triggered"), *Drop->Label, Position.X, Position.Y), Drop);
		}
		if (Drop->AnchorHeight.bSet && Drop->AnchorHeight.Value <= Drop->LoadHeight)
		{
			Error(FString::Printf(TEXT("drop hazard \"%s\" has its cable anchor at or below the load"), *Drop->Label), Drop);
		}
	}

	for (AB4DProp* Prop : Gather<AB4DProp>(Campaign))
	{
		if (Prop->Material.IsEmpty())
		{
			Error(FString::Printf(TEXT("a %s prop has no material"), *TypeName(Prop->Type)), Prop);
		}
		if (Prop->bSolid && (Prop->ColliderHalfExtents.X <= 0.f || Prop->ColliderHalfExtents.Y <= 0.f))
		{
			Error(FString::Printf(TEXT("a solid %s prop has a zero sized collider"), *TypeName(Prop->Type)), Prop);
		}
	}

	return Problems;
}

FString UB4DExporter::BuildJson(AB4DCampaign* Campaign)
{
	if (!Campaign)
	{
		return FString();
	}

	const TSharedRef<FJsonObject> Root = MakeShared<FJsonObject>();
	Root->SetNumberField(TEXT("schema"), 1);
	Root->SetStringField(TEXT("id"), Campaign->Id);
	Root->SetNumberField(TEXT("index"), Campaign->Index);
	Root->SetStringField(TEXT("theme"), Campaign->Theme);
	if (!Campaign->Extraction.IsEmpty())
	{
		Root->SetStringField(TEXT("extraction"), Campaign->Extraction);
	}
	if (!Campaign->bQuirkyProps)
	{
		Root->SetBoolField(TEXT("quirkyProps"), false);
	}

	// Zones
	TArray<TSharedPtr<FJsonValue>> ZoneValues;
	for (const AB4DZone* Zone : Gather<AB4DZone>(Campaign))
	{
		const FVector2D Position = B4DSpace::ToGameXZ(Zone->GetActorLocation());
		const TSharedRef<FJsonObject> Json = MakeShared<FJsonObject>();
		Json->SetStringField(TEXT("name"), Zone->ZoneName);
		Json->SetNumberField(TEXT("x"), Tidy(Position.X));
		Json->SetNumberField(TEXT("z"), Tidy(Position.Y));
		Json->SetNumberField(TEXT("halfX"), Tidy(Zone->GetHalfX()));
		Json->SetNumberField(TEXT("halfZ"), Tidy(Zone->GetHalfZ()));
		Json->SetStringField(TEXT("floor"), Zone->Floor);
		if (Zone->Roof.bSet)             Json->SetNumberField(TEXT("roof"), Tidy(Zone->Roof.Value));
		if (!Zone->RoofMaterial.IsEmpty()) Json->SetStringField(TEXT("roofMaterial"), Zone->RoofMaterial);
		if (Zone->LampY.bSet)            Json->SetNumberField(TEXT("lampY"), Tidy(Zone->LampY.Value));
		if (Zone->LampCols > 0)          Json->SetNumberField(TEXT("lampCols"), Zone->LampCols);
		if (!Zone->LampMaterial.IsEmpty()) Json->SetStringField(TEXT("lampMaterial"), Zone->LampMaterial);
		if (!Zone->bWalls)               Json->SetBoolField(TEXT("walls"), false);
		ZoneValues.Add(MakeShared<FJsonValueObject>(Json));
	}
	Root->SetArrayField(TEXT("zones"), ZoneValues);

	// Props, grouping the simple ones that differ only in where they stand.
	TArray<AB4DProp*> Props = Gather<AB4DProp>(Campaign);
	TMap<FString, TArray<AB4DProp*>> Groups;
	TArray<FString> GroupOrder;
	for (AB4DProp* Prop : Props)
	{
		const FString Key = AB4DProp::IsAreaProp(Prop->Type)
			? FString::Printf(TEXT("unique:%s"), *Prop->GetName())
			: GroupKey(Prop);
		if (!Groups.Contains(Key))
		{
			Groups.Add(Key, TArray<AB4DProp*>());
			GroupOrder.Add(Key);
		}
		Groups[Key].Add(Prop);
	}

	TArray<TSharedPtr<FJsonValue>> PropValues;
	for (const FString& Key : GroupOrder)
	{
		const TArray<AB4DProp*>& Bucket = Groups[Key];
		AB4DProp* First = Bucket[0];

		const TSharedRef<FJsonObject> Json = MakeShared<FJsonObject>();
		Json->SetStringField(TEXT("type"), TypeName(First->Type));
		Json->SetStringField(TEXT("material"), First->Material);
		for (const TPair<FName, float>& Field : First->Values)
		{
			Json->SetNumberField(Field.Key.ToString(), Tidy(Field.Value));
		}
		if (First->bCastShadow)
		{
			Json->SetBoolField(TEXT("shadow"), true);
		}
		if (First->bSolid)
		{
			TArray<TSharedPtr<FJsonValue>> Extents;
			Extents.Add(MakeShared<FJsonValueNumber>(Tidy(First->ColliderHalfExtents.X)));
			Extents.Add(MakeShared<FJsonValueNumber>(Tidy(First->ColliderHalfExtents.Y)));
			Json->SetArrayField(TEXT("collide"), Extents);
			Json->SetStringField(TEXT("kind"), First->ColliderKind);
		}

		const bool bSingle = Bucket.Num() == 1
			&& FMath::IsNearlyZero(First->GetActorRotation().Yaw)
			&& !AB4DProp::IsAreaProp(First->Type);
		const FVector2D FirstPosition = B4DSpace::ToGameXZ(First->GetActorLocation());

		if (AB4DProp::IsAreaProp(First->Type) || bSingle)
		{
			Json->SetNumberField(TEXT("x"), Tidy(FirstPosition.X));
			Json->SetNumberField(TEXT("z"), Tidy(FirstPosition.Y));
		}
		else
		{
			TArray<TSharedPtr<FJsonValue>> Spots;
			for (const AB4DProp* Prop : Bucket)
			{
				const FVector2D Position = B4DSpace::ToGameXZ(Prop->GetActorLocation());
				const float Yaw = Prop->GetActorRotation().Yaw;

				TArray<TSharedPtr<FJsonValue>> Spot;
				Spot.Add(MakeShared<FJsonValueNumber>(Tidy(Position.X)));
				Spot.Add(MakeShared<FJsonValueNumber>(Tidy(Position.Y)));
				if (!FMath::IsNearlyZero(Yaw))
				{
					Spot.Add(MakeShared<FJsonValueNumber>(Tidy(FMath::DegreesToRadians(Yaw))));
				}
				Spots.Add(MakeShared<FJsonValueArray>(Spot));
			}
			Json->SetArrayField(TEXT("at"), Spots);
		}

		PropValues.Add(MakeShared<FJsonValueObject>(Json));
	}
	if (PropValues.Num() > 0)
	{
		Root->SetArrayField(TEXT("props"), PropValues);
	}

	// Hazards
	const TSharedRef<FJsonObject> Hazards = MakeShared<FJsonObject>();

	TArray<TSharedPtr<FJsonValue>> BarrelValues;
	for (const AB4DBarrel* Barrel : Gather<AB4DBarrel>(Campaign))
	{
		const FVector2D Position = B4DSpace::ToGameXZ(Barrel->GetActorLocation());
		const TSharedRef<FJsonObject> Json = MakeShared<FJsonObject>();
		Json->SetNumberField(TEXT("x"), Tidy(Position.X));
		Json->SetNumberField(TEXT("z"), Tidy(Position.Y));
		if (ToHex(Barrel->Tint) != 0xc9552c)
		{
			Json->SetNumberField(TEXT("color"), ToHex(Barrel->Tint));
		}
		BarrelValues.Add(MakeShared<FJsonValueObject>(Json));
	}
	if (BarrelValues.Num() > 0)
	{
		Hazards->SetArrayField(TEXT("barrels"), BarrelValues);
	}

	TArray<TSharedPtr<FJsonValue>> DropValues;
	for (const AB4DDropHazard* Drop : Gather<AB4DDropHazard>(Campaign))
	{
		const FVector2D Position = B4DSpace::ToGameXZ(Drop->GetActorLocation());
		const TSharedRef<FJsonObject> Json = MakeShared<FJsonObject>();
		Json->SetNumberField(TEXT("x"), Tidy(Position.X));
		Json->SetNumberField(TEXT("z"), Tidy(Position.Y));
		Json->SetNumberField(TEXT("y"), Tidy(Drop->LoadHeight));
		if (Drop->AnchorHeight.bSet)
		{
			Json->SetNumberField(TEXT("anchorY"), Tidy(Drop->AnchorHeight.Value));
		}
		Json->SetNumberField(TEXT("width"), Tidy(Drop->Width));
		Json->SetNumberField(TEXT("depth"), Tidy(Drop->Depth));
		Json->SetNumberField(TEXT("height"), Tidy(Drop->Height));
		Json->SetNumberField(TEXT("color"), ToHex(Drop->Tint));
		Json->SetNumberField(TEXT("damage"), Tidy(Drop->Damage));
		if (!FMath::IsNearlyEqual(Drop->Radius, 7.f))
		{
			Json->SetNumberField(TEXT("radius"), Tidy(Drop->Radius));
		}
		Json->SetStringField(TEXT("label"), Drop->Label);
		DropValues.Add(MakeShared<FJsonValueObject>(Json));
	}
	if (DropValues.Num() > 0)
	{
		Hazards->SetArrayField(TEXT("drops"), DropValues);
	}
	if (BarrelValues.Num() > 0 || DropValues.Num() > 0)
	{
		Root->SetObjectField(TEXT("hazards"), Hazards);
	}

	// Gates, in chapter order.
	TArray<AB4DGate*> Gates = Gather<AB4DGate>(Campaign);
	Gates.Sort([](const AB4DGate& A, const AB4DGate& B) { return A.Chapter < B.Chapter; });
	TArray<TSharedPtr<FJsonValue>> GateValues;
	for (const AB4DGate* Gate : Gates)
	{
		const FVector2D Position = B4DSpace::ToGameXZ(Gate->GetActorLocation());
		const TSharedRef<FJsonObject> Json = MakeShared<FJsonObject>();
		Json->SetNumberField(TEXT("width"), Tidy(Gate->Width));
		Json->SetNumberField(TEXT("x"), Tidy(Position.X));
		GateValues.Add(MakeShared<FJsonValueObject>(Json));
	}
	if (GateValues.Num() > 0)
	{
		Root->SetArrayField(TEXT("gates"), GateValues);
	}

	// Objectives, in chapter order.
	TArray<AB4DObjective*> Objectives = Gather<AB4DObjective>(Campaign);
	Objectives.Sort([](const AB4DObjective& A, const AB4DObjective& B) { return A.Chapter < B.Chapter; });
	TArray<TSharedPtr<FJsonValue>> ObjectiveValues;
	for (const AB4DObjective* Objective : Objectives)
	{
		const FVector2D Position = B4DSpace::ToGameXZ(Objective->GetActorLocation());
		const TSharedRef<FJsonObject> Json = MakeShared<FJsonObject>();
		Json->SetNumberField(TEXT("chapter"), Objective->Chapter);
		Json->SetNumberField(TEXT("x"), Tidy(Position.X));
		Json->SetNumberField(TEXT("z"), Tidy(Position.Y));
		Json->SetStringField(TEXT("label"), Objective->Label);
		Json->SetStringField(TEXT("verb"), Objective->Verb);
		Json->SetNumberField(TEXT("duration"), Tidy(Objective->Duration));
		Json->SetStringField(TEXT("kind"), Objective->Kind);
		Json->SetStringField(TEXT("type"), TypeName(Objective->Type));
		if (Objective->Window.bSet)
		{
			Json->SetNumberField(TEXT("window"), Tidy(Objective->Window.Value));
		}
		if (Objective->CartTo)
		{
			const FVector2D CartPosition = B4DSpace::ToGameXZ(Objective->CartTo->GetActorLocation());
			const TSharedRef<FJsonObject> Cart = MakeShared<FJsonObject>();
			Cart->SetNumberField(TEXT("x"), Tidy(CartPosition.X));
			Cart->SetNumberField(TEXT("z"), Tidy(CartPosition.Y));
			Json->SetObjectField(TEXT("cartTo"), Cart);
		}
		if (Objective->Nodes.Num() > 0)
		{
			TArray<TSharedPtr<FJsonValue>> NodeValues;
			for (const FB4DObjectiveNode& Node : Objective->Nodes)
			{
				const TSharedRef<FJsonObject> NodeJson = MakeShared<FJsonObject>();
				NodeJson->SetNumberField(TEXT("dx"), Tidy(Node.DX));
				NodeJson->SetNumberField(TEXT("dz"), Tidy(Node.DZ));
				NodeJson->SetStringField(TEXT("label"), Node.Label);
				NodeValues.Add(MakeShared<FJsonValueObject>(NodeJson));
			}
			Json->SetArrayField(TEXT("nodes"), NodeValues);
		}
		ObjectiveValues.Add(MakeShared<FJsonValueObject>(Json));
	}
	if (ObjectiveValues.Num() > 0)
	{
		Root->SetArrayField(TEXT("objectives"), ObjectiveValues);
	}

	FString Output;
	const TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&Output);
	FJsonSerializer::Serialize(Root, Writer);
	return Output;
}

bool UB4DExporter::ExportToFile(AB4DCampaign* Campaign, const FString& FilePath, TArray<FB4DProblem>& OutProblems)
{
	OutProblems = Validate(Campaign);
	for (const FB4DProblem& Problem : OutProblems)
	{
		if (Problem.Level == EB4DProblemLevel::Error)
		{
			return false;
		}
	}
	return FFileHelper::SaveStringToFile(BuildJson(Campaign), *FilePath);
}

FString UB4DExporter::BuildLivePreviewSnippet(AB4DCampaign* Campaign)
{
	if (!Campaign)
	{
		return FString();
	}
	return FString::Printf(TEXT("window.B4D_CAMPAIGN_OVERRIDES = { \"%s\": %s };"),
		*Campaign->Id, *BuildJson(Campaign));
}
