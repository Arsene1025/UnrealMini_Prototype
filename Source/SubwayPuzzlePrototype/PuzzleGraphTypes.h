#pragma once

#include "CoreMinimal.h"
#include "PuzzleGraphTypes.generated.h"

/**
 * Why an edge cannot be traversed. The design brief lists stairs, narrow doorways and
 * step gaps as distinct obstacles, so this carries the reason rather than a bare bool:
 * the HUD can explain the refusal, and later stages can make traversability depend on
 * the obstacle type instead of a single blocked flag.
 */
UENUM()
enum class EPuzzleBlockReason : uint8
{
    None            UMETA(DisplayName = "Open"),
    Stairs          UMETA(DisplayName = "Stairs"),
    NarrowDoorway   UMETA(DisplayName = "Narrow doorway"),
    StepGap         UMETA(DisplayName = "Step / gap"),
    Other           UMETA(DisplayName = "Other")
};

/** One node of the traversal graph, in board-local space. */
USTRUCT()
struct FPuzzleNodeData
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, Category = "Puzzle Graph")
    FVector LocalLocation = FVector::ZeroVector;

    /** Optional author-facing label. Not used by gameplay. */
    UPROPERTY(EditAnywhere, Category = "Puzzle Graph")
    FName Label;
};

/** One undirected connection between two nodes, by index into the node array. */
USTRUCT()
struct FPuzzleEdgeData
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, Category = "Puzzle Graph")
    int32 NodeA = INDEX_NONE;

    UPROPERTY(EditAnywhere, Category = "Puzzle Graph")
    int32 NodeB = INDEX_NONE;

    UPROPERTY(EditAnywhere, Category = "Puzzle Graph")
    EPuzzleBlockReason BlockReason = EPuzzleBlockReason::None;

    bool IsBlocked() const { return BlockReason != EPuzzleBlockReason::None; }
};
