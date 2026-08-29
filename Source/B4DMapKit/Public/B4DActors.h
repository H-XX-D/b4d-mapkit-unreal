#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "B4DMapKitTypes.h"
#include "B4DActors.generated.h"

class UBoxComponent;
class UBillboardComponent;

/**
 * Root of a campaign. Put one in the level and everything else is found by
 * searching the level around it, so a level can hold one campaign at a time.
 * Derive a Blueprint from this to preset a theme for a whole set of maps.
 */
UCLASS(Blueprintable, meta = (DisplayName = "B4D Campaign"))
class B4DMAPKIT_API AB4DCampaign : public AActor
{
	GENERATED_BODY()

public:
	AB4DCampaign();

	/** Stable key for this map, lower case with underscores. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Campaign")
	FString Id = TEXT("new_campaign");

	/** Campaign slot this map occupies, 0 to 4. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Campaign", meta = (ClampMin = "0", ClampMax = "4"))
	int32 Index = 2;

	/** Material palette, ambient light and wall settings. Must match a theme in the game. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Campaign")
	FString Theme = TEXT("slaughterhouse");

	/** Vehicle waiting at the end of the map. Leave empty for none. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Campaign")
	FString Extraction = TEXT("cattleTruck");

	/** Scatter the shared incidental set dressing through the map. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Campaign")
	bool bQuirkyProps = true;
};

/**
 * A room or corridor, drawn as a box you can drag with the normal scale widget.
 *
 * Zones carry the map. The walkable area is the union of every zone, and where
 * two zones overlap by more than 1.5 metres that overlap becomes the doorway
 * between them. Navigation, spawning and the interior architecture pass are all
 * derived from this list, so a map is mostly a question of drawing boxes.
 */
UCLASS(Blueprintable, meta = (DisplayName = "B4D Zone"))
class B4DMAPKIT_API AB4DZone : public AActor
{
	GENERATED_BODY()

public:
	AB4DZone();

	/** Unique within the map. Shown in nav debugging. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zone")
	FString ZoneName = TEXT("room");

	/** Theme material for the floor, e.g. tile, blood, mud. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zone")
	FString Floor = TEXT("tile");

	/** Ceiling height in metres. Clear it for an open-air zone. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zone")
	FB4DOptionalFloat Roof = FB4DOptionalFloat(true, 9.f);

	/** Height of the flickering ceiling lamps. Clear it for no lamps. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zone")
	FB4DOptionalFloat LampY = FB4DOptionalFloat(true, 8.4f);

	/** Leave at 0 to let the game pick: 2 columns when the zone is wider than 40m, else 1. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zone", meta = (ClampMin = "0"))
	int32 LampCols = 0;

	/** Theme material for the ceiling. Empty uses the theme's wall material. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zone")
	FString RoofMaterial;

	/** Theme material for the lamp cages. Empty uses steel. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zone")
	FString LampMaterial;

	/** Turn off for an outdoor yard that should not be walled in. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zone")
	bool bWalls = true;

	/** Drag this in the viewport to size the zone. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Zone")
	UBoxComponent* Bounds;

	/** Half width on the game's X axis, in metres. */
	UFUNCTION(BlueprintPure, Category = "Zone")
	float GetHalfX() const;

	/** Half depth on the game's Z axis, in metres. */
	UFUNCTION(BlueprintPure, Category = "Zone")
	float GetHalfZ() const;

	/** True when this zone overlaps the other enough to walk between them. */
	UFUNCTION(BlueprintPure, Category = "Zone")
	bool ConnectsTo(const AB4DZone* Other) const;

	/** True when the game-space point falls inside this zone. */
	UFUNCTION(BlueprintPure, Category = "Zone")
	bool ContainsGamePoint(float GameX, float GameZ) const;
};

/**
 * A chapter's device. The squad has to run it, which draws a sustained attack,
 * and only when the work finishes does that chapter's gate open.
 */
UCLASS(Blueprintable, meta = (DisplayName = "B4D Objective"))
class B4DMAPKIT_API AB4DObjective : public AActor
{
	GENERATED_BODY()

public:
	AB4DObjective();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Objective", meta = (ClampMin = "1"))
	int32 Chapter = 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Objective")
	EB4DObjectiveType Type = EB4DObjectiveType::Signal;

	/** Shown to the squad as their current order, e.g. RESTART THE CHAIN LINE. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Objective")
	FString Label = TEXT("DO THE THING");

	/** Short sign text on the device itself, e.g. CHAIN LINE. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Objective")
	FString Verb = TEXT("DEVICE");

	/** Flavour key used for barks and set dressing, e.g. chain, freezer. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Objective")
	FString Kind = TEXT("generic");

	/** Seconds of work once the device is started. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Objective", meta = (ClampMin = "1.0"))
	float Duration = 10.f;

	/** Breakers only: seconds every switch must be held live at the same time. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Objective")
	FB4DOptionalFloat Window;

	/** Escort only: drop any actor here to mark where the cart has to end up. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Objective")
	AActor* CartTo = nullptr;

	/** Secondary stations, placed relative to this device. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Objective")
	TArray<FB4DObjectiveNode> Nodes;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Objective")
	UBillboardComponent* Marker;
};

/** A checkpoint door, opened by its chapter's objective. */
UCLASS(Blueprintable, meta = (DisplayName = "B4D Gate"))
class B4DMAPKIT_API AB4DGate : public AActor
{
	GENERATED_BODY()

public:
	AB4DGate();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gate", meta = (ClampMin = "1.0"))
	float Width = 14.f;

	/** Which chapter this gate closes off. Gates export in chapter order. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gate", meta = (ClampMin = "1"))
	int32 Chapter = 1;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Gate")
	UBoxComponent* Bounds;
};

/** A fuel barrel. Shoot it and it takes the crowd around it with it. */
UCLASS(Blueprintable, meta = (DisplayName = "B4D Fuel Barrel"))
class B4DMAPKIT_API AB4DBarrel : public AActor
{
	GENERATED_BODY()

public:
	AB4DBarrel();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hazard")
	FColor Tint = FColor(0xc9, 0x55, 0x2c);

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Hazard")
	UBillboardComponent* Marker;
};

/**
 * A heavy load hanging from a cable. Shoot the load or the cable and it comes
 * down on whatever is underneath.
 */
UCLASS(Blueprintable, meta = (DisplayName = "B4D Drop Hazard"))
class B4DMAPKIT_API AB4DDropHazard : public AActor
{
	GENERATED_BODY()

public:
	AB4DDropHazard();

	/** Resting height of the load, in metres. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hazard")
	float LoadHeight = 5.4f;

	/** Height the cable is anchored at. Clear it to sit 9m above the load. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hazard")
	FB4DOptionalFloat AnchorHeight = FB4DOptionalFloat(true, 8.6f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hazard")
	float Width = 12.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hazard")
	float Depth = 5.6f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hazard")
	float Height = 2.9f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hazard")
	FColor Tint = FColor(0x8a, 0x3b, 0x2f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hazard", meta = (ClampMin = "0.0"))
	float Damage = 900.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hazard", meta = (ClampMin = "0.0"))
	float Radius = 7.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hazard")
	FString Label = TEXT("DO NOT STAND UNDER");

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Hazard")
	UBoxComponent* Bounds;
};

/**
 * Set dressing. One actor covers every prop type the game can build: pick the
 * type and fill in the numbers that type needs.
 *
 * Simple props that differ only in where they stand are collapsed into a single
 * JSON entry on export, so duplicating a crate around the level stays cheap in
 * the map file.
 */
UCLASS(Blueprintable, meta = (DisplayName = "B4D Prop"))
class B4DMAPKIT_API AB4DProp : public AActor
{
	GENERATED_BODY()

public:
	AB4DProp();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Prop")
	EB4DPropType Type = EB4DPropType::Box;

	/** Theme material name, e.g. steel, rust, tile. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Prop")
	FString Material = TEXT("steel");

	/** Blocks movement. Turn off for a prop players walk through. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Prop")
	bool bSolid = false;

	/** Half extents of the blocking box, in metres. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Prop", meta = (EditCondition = "bSolid"))
	FVector2D ColliderHalfExtents = FVector2D(1.f, 1.f);

	/** Collider label, shown in debug views, e.g. pen-rail. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Prop", meta = (EditCondition = "bSolid"))
	FString ColliderKind = TEXT("prop");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Prop")
	bool bCastShadow = false;

	/** Numbers this prop type needs. Press Reset Fields after changing the type. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Prop")
	TMap<FName, float> Values;

	/** Fills in the fields this type needs, keeping any the old type shared. */
	UFUNCTION(CallInEditor, BlueprintCallable, Category = "Prop")
	void ResetFieldsForType();

	/** The fields a given prop type needs, with the game's defaults. */
	static TMap<FName, float> DefaultFieldsFor(EB4DPropType InType);

	/** Prop types that describe their own extents and are never grouped on export. */
	static bool IsAreaProp(EB4DPropType InType);

#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& Event) override;
#endif

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Prop")
	UBillboardComponent* Marker;
};
