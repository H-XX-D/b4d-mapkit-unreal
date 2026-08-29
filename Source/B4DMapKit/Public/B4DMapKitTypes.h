#pragma once

#include "CoreMinimal.h"
#include "B4DMapKitTypes.generated.h"

/** What a chapter's device asks the squad to do. */
UENUM(BlueprintType)
enum class EB4DObjectiveType : uint8
{
	Signal       UMETA(DisplayName = "Signal (hold one device)"),
	Breakers     UMETA(DisplayName = "Breakers (hold switches together)"),
	Fuel         UMETA(DisplayName = "Fuel (carry cans to stations)"),
	Triangulate  UMETA(DisplayName = "Triangulate (run three points)"),
	Escort       UMETA(DisplayName = "Escort (push a cart)")
};

/** Set dressing the game knows how to build from numbers alone. */
UENUM(BlueprintType)
enum class EB4DPropType : uint8
{
	Box,
	Cylinder,
	Grid,
	ChainLine,
	CarcassRows,
	LightPole,
	Vat,
	PipeRun
};

/** A secondary station belonging to an objective, placed relative to the device. */
USTRUCT(BlueprintType)
struct FB4DObjectiveNode
{
	GENERATED_BODY()

	/** Offset from the device, in metres. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Blox 4 Dead")
	float DX = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Blox 4 Dead")
	float DZ = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Blox 4 Dead")
	FString Label = TEXT("NODE");
};

/** A number that may be absent. The game tells "no roof" apart from "a roof at
 *  height zero", so the kit has to carry that difference through the details panel. */
USTRUCT(BlueprintType)
struct FB4DOptionalFloat
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Blox 4 Dead")
	bool bSet = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Blox 4 Dead", meta = (EditCondition = "bSet"))
	float Value = 0.f;

	FB4DOptionalFloat() {}
	FB4DOptionalFloat(bool bInSet, float InValue) : bSet(bInSet), Value(InValue) {}
};

UENUM(BlueprintType)
enum class EB4DProblemLevel : uint8
{
	Error,
	Warning
};

/** One thing wrong with the map, ready to show in the kit's panel. */
USTRUCT(BlueprintType)
struct FB4DProblem
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Blox 4 Dead")
	EB4DProblemLevel Level = EB4DProblemLevel::Error;

	UPROPERTY(BlueprintReadOnly, Category = "Blox 4 Dead")
	FString Message;

	UPROPERTY(BlueprintReadOnly, Category = "Blox 4 Dead")
	TWeakObjectPtr<AActor> Context;

	FB4DProblem() {}
	FB4DProblem(EB4DProblemLevel InLevel, const FString& InMessage, AActor* InContext = nullptr)
		: Level(InLevel), Message(InMessage), Context(InContext) {}
};

/**
 * Unreal works in centimetres with Z up. The game works in metres on the X/Z
 * ground plane with Y up. Every coordinate crossing the boundary goes through
 * here so the convention lives in exactly one place.
 */
namespace B4DSpace
{
	static constexpr float UnitsPerMetre = 100.f;

	/** Unreal world location to the game's ground plane coordinates, in metres. */
	FORCEINLINE FVector2D ToGameXZ(const FVector& UnrealLocation)
	{
		return FVector2D(UnrealLocation.X / UnitsPerMetre, UnrealLocation.Y / UnitsPerMetre);
	}

	/** Unreal height to the game's up axis, in metres. */
	FORCEINLINE float ToGameY(float UnrealZ)
	{
		return UnrealZ / UnitsPerMetre;
	}

	FORCEINLINE FVector ToUnreal(float GameX, float GameZ, float GameY = 0.f)
	{
		return FVector(GameX * UnitsPerMetre, GameZ * UnitsPerMetre, GameY * UnitsPerMetre);
	}
}
