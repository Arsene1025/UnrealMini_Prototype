#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "PuzzleGraphBaker.generated.h"

class APuzzleNodeMarker;
class UPuzzleGraphAsset;

/**
 * Editor-only authoring tool. Drop one into the authoring level, point it at a
 * UPuzzleGraphAsset, and use the Bake / Load buttons in the details panel.
 *
 * This actor's transform is the board origin: baked node positions are stored relative
 * to it, so the runtime board reproduces the layout wherever it is spawned.
 */
UCLASS(HideCategories = (Rendering, Physics, Collision, Networking, Input, LOD, Cooking, HLOD))
class SPP_API APuzzleGraphBaker : public AActor
{
    GENERATED_BODY()

public:
    APuzzleGraphBaker();

    virtual void Tick(float DeltaSeconds) override;
    virtual bool ShouldTickIfViewportsOnly() const override { return true; }

    /** Asset written by Bake and read by Load. */
    UPROPERTY(EditAnywhere, Category = "Puzzle Graph")
    TObjectPtr<UPuzzleGraphAsset> TargetAsset;

    /** Draw the authored graph in the editor viewport. */
    UPROPERTY(EditAnywhere, Category = "Puzzle Graph")
    bool bDrawGraphPreview = true;

    /** Collect every APuzzleNodeMarker in this level and overwrite TargetAsset. */
    UFUNCTION(CallInEditor, Category = "Puzzle Graph", meta = (DisplayName = "Bake Level -> Asset"))
    void BakeFromLevel();

    /** Spawn markers from TargetAsset. Existing markers in the level are removed first. */
    UFUNCTION(CallInEditor, Category = "Puzzle Graph", meta = (DisplayName = "Load Asset -> Level"))
    void LoadIntoLevel();

private:
    void CollectSortedMarkers(TArray<APuzzleNodeMarker*>& OutMarkers) const;
};
