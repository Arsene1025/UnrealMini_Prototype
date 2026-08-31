#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "PuzzleGraphTypes.h"
#include "PuzzleBoard.generated.h"

class UMaterialInstanceDynamic;
class UMaterialInterface;
class UPrimitiveComponent;
class UPuzzleGraphAsset;
class USceneComponent;
class UStaticMesh;
class UStaticMeshComponent;

/** Runtime adjacency entry. Built from FPuzzleEdgeData, never authored directly. */
struct FPuzzleEdge
{
    int32 TargetNode = INDEX_NONE;
    EPuzzleBlockReason BlockReason = EPuzzleBlockReason::None;

    bool IsBlocked() const { return BlockReason != EPuzzleBlockReason::None; }
};

/** Runtime node. Built from FPuzzleNodeData, never authored directly. */
struct FPuzzleNode
{
    int32 Id = INDEX_NONE;
    FVector LocalLocation = FVector::ZeroVector;
    TArray<FPuzzleEdge> Edges;
};

UCLASS()
class SPP_API APuzzleBoard : public AActor
{
    GENERATED_BODY()

public:
    APuzzleBoard();

    /**
     * Graph this board plays. Left unset, BeginPlay falls back to the project's
     * default stage, and then to the built-in test graph.
     */
    UPROPERTY(EditAnywhere, Category = "Puzzle Graph")
    TObjectPtr<UPuzzleGraphAsset> GraphAsset;

    int32 GetNodeCount() const { return Nodes.Num(); }
    int32 GetStartNode() const { return StartNode; }
    int32 GetGoalNode() const { return GoalNode; }
    FVector GetNodeWorldLocation(int32 NodeIndex) const;
    FVector GetBoardCenter() const;

    /** Whether the prototype greybox is drawn. Node tiles stay clickable either way. */
    bool ShouldShowGreybox() const { return bShowGreyboxVisuals; }

    bool ResolveNodeFromHit(const FHitResult& Hit, int32& OutNodeIndex) const;
    bool FindDirectionalTarget(
        int32 FromNode,
        const FVector2D& Direction,
        int32& OutTargetNode,
        EPuzzleBlockReason& OutBlockReason) const;
    bool FindPathAStar(int32 StartNodeIndex, int32 GoalNodeIndex, TArray<int32>& OutPath) const;

    /** Player-facing wording for a refused move. */
    static FString DescribeBlockReason(EPuzzleBlockReason Reason);

protected:
    /**
     * The graph is built here, not in BeginPlay. PostInitializeComponents runs inside
     * SpawnActor, so the board is queryable the moment it exists -- the game mode reads
     * it during StartPlay, and BeginPlay ordering between actors is not guaranteed.
     */
    virtual void PostInitializeComponents() override;
    virtual void BeginPlay() override;

private:
    void BuildGraphFromAsset(const UPuzzleGraphAsset& Asset);
    void BuildFallbackGraph();
    void BuildVisuals();
    void ResetGraph(int32 NodeCount);
    void ConnectNodes(int32 NodeA, int32 NodeB, EPuzzleBlockReason BlockReason = EPuzzleBlockReason::None);
    void CreateConnectionVisual(int32 NodeA, int32 NodeB, bool bBlocked);
    UStaticMeshComponent* CreateBoxComponent(
        const FString& ComponentName,
        const FVector& RelativeLocation,
        const FRotator& RelativeRotation,
        const FVector& RelativeScale,
        UMaterialInterface* Material,
        bool bClickable);
    UMaterialInstanceDynamic* CreateColorMaterial(const FName& Name, const FLinearColor& Color);
    float EstimateCost(int32 FromNode, int32 ToNode) const;

    UPROPERTY(VisibleAnywhere)
    TObjectPtr<USceneComponent> SceneRoot;

    UPROPERTY()
    TObjectPtr<UStaticMesh> CubeMesh;

    UPROPERTY()
    TObjectPtr<UMaterialInterface> BaseMaterial;

    UPROPERTY(Transient)
    TObjectPtr<UMaterialInstanceDynamic> NodeMaterial;

    UPROPERTY(Transient)
    TObjectPtr<UMaterialInstanceDynamic> GoalMaterial;

    UPROPERTY(Transient)
    TObjectPtr<UMaterialInstanceDynamic> ConnectionMaterial;

    UPROPERTY(Transient)
    TObjectPtr<UMaterialInstanceDynamic> BlockedMaterial;

    UPROPERTY(Transient)
    TObjectPtr<UMaterialInstanceDynamic> PlatformMaterial;

    UPROPERTY(Transient)
    TArray<TObjectPtr<UStaticMeshComponent>> NodeTiles;

    TArray<FPuzzleNode> Nodes;
    TMap<const UPrimitiveComponent*, int32> ClickableNodeByComponent;

    int32 StartNode = 0;
    int32 GoalNode = INDEX_NONE;
    bool bShowGreyboxVisuals = true;
};
