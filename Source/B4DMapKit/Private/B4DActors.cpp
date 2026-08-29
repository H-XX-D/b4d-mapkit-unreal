#include "B4DActors.h"
#include "Components/BoxComponent.h"
#include "Components/BillboardComponent.h"

// ---------------------------------------------------------------------------
// AB4DCampaign
// ---------------------------------------------------------------------------

AB4DCampaign::AB4DCampaign()
{
	PrimaryActorTick.bCanEverTick = false;
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
}

// ---------------------------------------------------------------------------
// AB4DZone
// ---------------------------------------------------------------------------

AB4DZone::AB4DZone()
{
	PrimaryActorTick.bCanEverTick = false;

	Bounds = CreateDefaultSubobject<UBoxComponent>(TEXT("Bounds"));
	RootComponent = Bounds;
	// 20m by 20m by 9m, sitting on the ground rather than centred on it, so the
	// actor's own location is the zone centre the game expects.
	Bounds->SetBoxExtent(FVector(2000.f, 2000.f, 450.f));
	Bounds->SetRelativeLocation(FVector(0.f, 0.f, 450.f));
	Bounds->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Bounds->SetHiddenInGame(true);
	Bounds->ShapeColor = FColor(102, 217, 255);
	Bounds->bDrawOnlyIfSelected = false;
}

float AB4DZone::GetHalfX() const
{
	return Bounds ? Bounds->GetScaledBoxExtent().X / B4DSpace::UnitsPerMetre : 0.f;
}

float AB4DZone::GetHalfZ() const
{
	return Bounds ? Bounds->GetScaledBoxExtent().Y / B4DSpace::UnitsPerMetre : 0.f;
}

bool AB4DZone::ConnectsTo(const AB4DZone* Other) const
{
	if (!Other || Other == this)
	{
		return false;
	}

	const FVector2D A = B4DSpace::ToGameXZ(GetActorLocation());
	const FVector2D B = B4DSpace::ToGameXZ(Other->GetActorLocation());
	const float AHalfX = GetHalfX(), AHalfZ = GetHalfZ();
	const float BHalfX = Other->GetHalfX(), BHalfZ = Other->GetHalfZ();

	const float OverlapX = FMath::Min(A.X + AHalfX, B.X + BHalfX) - FMath::Max(A.X - AHalfX, B.X - BHalfX);
	const float OverlapZ = FMath::Min(A.Y + AHalfZ, B.Y + BHalfZ) - FMath::Max(A.Y - AHalfZ, B.Y - BHalfZ);

	// The game treats anything narrower than 1.5m as a wall seam, not a doorway.
	return OverlapX > 1.5f && OverlapZ > 1.5f;
}

bool AB4DZone::ContainsGamePoint(float GameX, float GameZ) const
{
	const FVector2D Centre = B4DSpace::ToGameXZ(GetActorLocation());
	return FMath::Abs(GameX - Centre.X) <= GetHalfX() && FMath::Abs(GameZ - Centre.Y) <= GetHalfZ();
}

// ---------------------------------------------------------------------------
// AB4DObjective
// ---------------------------------------------------------------------------

AB4DObjective::AB4DObjective()
{
	PrimaryActorTick.bCanEverTick = false;
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	Marker = CreateDefaultSubobject<UBillboardComponent>(TEXT("Marker"));
	Marker->SetupAttachment(RootComponent);
	Marker->SetHiddenInGame(true);
}

// ---------------------------------------------------------------------------
// AB4DGate
// ---------------------------------------------------------------------------

AB4DGate::AB4DGate()
{
	PrimaryActorTick.bCanEverTick = false;
	Bounds = CreateDefaultSubobject<UBoxComponent>(TEXT("Bounds"));
	RootComponent = Bounds;
	Bounds->SetBoxExtent(FVector(700.f, 30.f, 250.f));
	Bounds->SetRelativeLocation(FVector(0.f, 0.f, 250.f));
	Bounds->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Bounds->SetHiddenInGame(true);
	Bounds->ShapeColor = FColor(230, 77, 77);
}

// ---------------------------------------------------------------------------
// AB4DBarrel
// ---------------------------------------------------------------------------

AB4DBarrel::AB4DBarrel()
{
	PrimaryActorTick.bCanEverTick = false;
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	Marker = CreateDefaultSubobject<UBillboardComponent>(TEXT("Marker"));
	Marker->SetupAttachment(RootComponent);
	Marker->SetHiddenInGame(true);
}

// ---------------------------------------------------------------------------
// AB4DDropHazard
// ---------------------------------------------------------------------------

AB4DDropHazard::AB4DDropHazard()
{
	PrimaryActorTick.bCanEverTick = false;
	Bounds = CreateDefaultSubobject<UBoxComponent>(TEXT("Bounds"));
	RootComponent = Bounds;
	Bounds->SetBoxExtent(FVector(600.f, 280.f, 145.f));
	Bounds->SetRelativeLocation(FVector(0.f, 0.f, 540.f));
	Bounds->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Bounds->SetHiddenInGame(true);
	Bounds->ShapeColor = FColor(138, 59, 47);
}

// ---------------------------------------------------------------------------
// AB4DProp
// ---------------------------------------------------------------------------

AB4DProp::AB4DProp()
{
	PrimaryActorTick.bCanEverTick = false;
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	Marker = CreateDefaultSubobject<UBillboardComponent>(TEXT("Marker"));
	Marker->SetupAttachment(RootComponent);
	Marker->SetHiddenInGame(true);
	Values = DefaultFieldsFor(Type);
}

TMap<FName, float> AB4DProp::DefaultFieldsFor(EB4DPropType InType)
{
	TMap<FName, float> Fields;
	switch (InType)
	{
	case EB4DPropType::Box:
		Fields.Add(TEXT("w"), 4.f); Fields.Add(TEXT("h"), 2.f); Fields.Add(TEXT("d"), 2.f);
		break;
	case EB4DPropType::Cylinder:
		Fields.Add(TEXT("rTop"), 1.f); Fields.Add(TEXT("rBottom"), 1.f);
		Fields.Add(TEXT("h"), 3.f); Fields.Add(TEXT("seg"), 12.f);
		break;
	case EB4DPropType::Grid:
		Fields.Add(TEXT("x0"), -20.f); Fields.Add(TEXT("x1"), 20.f); Fields.Add(TEXT("stepX"), 10.f);
		Fields.Add(TEXT("z0"), -20.f); Fields.Add(TEXT("z1"), 20.f); Fields.Add(TEXT("stepZ"), 10.f);
		break;
	case EB4DPropType::ChainLine:
		Fields.Add(TEXT("y"), 8.6f); Fields.Add(TEXT("length"), 84.f); Fields.Add(TEXT("count"), 14.f);
		Fields.Add(TEXT("startX"), -40.f); Fields.Add(TEXT("spacing"), 6.f);
		Fields.Add(TEXT("hookY"), 8.4f); Fields.Add(TEXT("carcassEvery"), 2.f);
		break;
	case EB4DPropType::CarcassRows:
		Fields.Add(TEXT("rows"), 5.f); Fields.Add(TEXT("z0"), -34.f); Fields.Add(TEXT("rowStep"), 17.f);
		Fields.Add(TEXT("perRow"), 9.f); Fields.Add(TEXT("spacing"), 6.4f); Fields.Add(TEXT("railLength"), 58.f);
		Fields.Add(TEXT("railXA"), -6.f); Fields.Add(TEXT("railXB"), 10.f); Fields.Add(TEXT("railY"), 6.4f);
		Fields.Add(TEXT("startXA"), -34.f); Fields.Add(TEXT("startXB"), -18.f);
		Fields.Add(TEXT("carcassY"), 3.6f); Fields.Add(TEXT("chainY"), 5.6f);
		break;
	case EB4DPropType::LightPole:
		Fields.Add(TEXT("h"), 13.f); Fields.Add(TEXT("lightY"), 12.f);
		Fields.Add(TEXT("rTop"), 0.35f); Fields.Add(TEXT("rBottom"), 0.5f);
		Fields.Add(TEXT("intensity"), 1.4f); Fields.Add(TEXT("distance"), 74.f);
		break;
	case EB4DPropType::Vat:
		Fields.Add(TEXT("rTop"), 5.2f); Fields.Add(TEXT("rBottom"), 5.6f); Fields.Add(TEXT("h"), 4.4f);
		break;
	case EB4DPropType::PipeRun:
		Fields.Add(TEXT("y0"), 6.5f); Fields.Add(TEXT("yStep"), 0.9f); Fields.Add(TEXT("z0"), -160.f);
		Fields.Add(TEXT("zStep"), 12.f); Fields.Add(TEXT("count"), 6.f);
		Fields.Add(TEXT("length"), 88.f); Fields.Add(TEXT("radius"), 0.4f);
		break;
	}
	return Fields;
}

bool AB4DProp::IsAreaProp(EB4DPropType InType)
{
	return InType == EB4DPropType::Grid
		|| InType == EB4DPropType::ChainLine
		|| InType == EB4DPropType::CarcassRows
		|| InType == EB4DPropType::PipeRun;
}

void AB4DProp::ResetFieldsForType()
{
	const TMap<FName, float> Wanted = DefaultFieldsFor(Type);
	TMap<FName, float> Kept;
	for (const TPair<FName, float>& Field : Wanted)
	{
		// Keep any value the old type already had under the same name, so
		// switching between two similar props does not lose your numbers.
		const float* Existing = Values.Find(Field.Key);
		Kept.Add(Field.Key, Existing ? *Existing : Field.Value);
	}
	Values = Kept;
}

#if WITH_EDITOR
void AB4DProp::PostEditChangeProperty(FPropertyChangedEvent& Event)
{
	Super::PostEditChangeProperty(Event);
	const FName Changed = Event.GetPropertyName();
	if (Changed == GET_MEMBER_NAME_CHECKED(AB4DProp, Type))
	{
		ResetFieldsForType();
	}
}
#endif
