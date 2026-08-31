#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "PuzzleGraphTypes.h"
#include "PuzzleGraphAsset.generated.h"

/**
 * A whole stage's traversal graph. Authored by placing APuzzleNodeMarker actors in a
 * level and baking them with APuzzleGraphBaker; consumed at runtime by APuzzleBoard.
 * Swapping this asset is what "a second stage without touching code" means.
 */
UCLASS(BlueprintType)
class SPP_API UPuzzleGraphAsset : public UDataAsset
{
    GENERATED_BODY()

public:
    UPROPERTY(EditAnywhere, Category = "Puzzle Graph")
    TArray<FPuzzleNodeData> Nodes;

    UPROPERTY(EditAnywhere, Category = "Puzzle Graph")
    TArray<FPuzzleEdgeData> Edges;

    UPROPERTY(EditAnywhere, Category = "Puzzle Graph")
    int32 StartNode = 0;

    UPROPERTY(EditAnywhere, Category = "Puzzle Graph")
    int32 GoalNode = INDEX_NONE;

    bool IsValidGraph() const
    {
        return Nodes.Num() > 0 && Nodes.IsValidIndex(StartNode) && Nodes.IsValidIndex(GoalNode);
    }
};
