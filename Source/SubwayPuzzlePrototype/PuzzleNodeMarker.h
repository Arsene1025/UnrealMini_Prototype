#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "PuzzleGraphTypes.h"
#include "PuzzleNodeMarker.generated.h"

class UStaticMeshComponent;

class APuzzleNodeMarker;

/** One authored connection from the owning marker to another marker. */
USTRUCT()
struct FPuzzleMarkerLink
{
    GENERATED_BODY()

    /** Pick with the details-panel eyedropper, or drag the target marker in from the outliner. */
    UPROPERTY(EditAnywhere, Category = "Puzzle Graph")
    TObjectPtr<APuzzleNodeMarker> Target;

    UPROPERTY(EditAnywhere, Category = "Puzzle Graph")
    EPuzzleBlockReason BlockReason = EPuzzleBlockReason::None;
};

/**
 * Editor-only authoring actor: one of these per graph node. Place them in a level and
 * move them with the normal transform gizmo, then bake with APuzzleGraphBaker. These
 * never ship -- only the baked UPuzzleGraphAsset is loaded at runtime.
 */
UCLASS(HideCategories = (Rendering, Physics, Collision, Networking, Input, LOD, Cooking, HLOD, DataLayers, WorldPartition))
class SPP_API APuzzleNodeMarker : public AActor
{
    GENERATED_BODY()

public:
    APuzzleNodeMarker();

    /** Connections authored from this node. Links are undirected once baked. */
    UPROPERTY(EditAnywhere, Category = "Puzzle Graph")
    TArray<FPuzzleMarkerLink> Links;

    UPROPERTY(EditAnywhere, Category = "Puzzle Graph")
    bool bIsStartNode = false;

    UPROPERTY(EditAnywhere, Category = "Puzzle Graph")
    bool bIsGoalNode = false;

    /** Author-facing label copied into the baked asset. Purely informational. */
    UPROPERTY(EditAnywhere, Category = "Puzzle Graph")
    FName Label;

private:
    UPROPERTY(VisibleAnywhere, Category = "Puzzle Graph")
    TObjectPtr<UStaticMeshComponent> MarkerMesh;
};
