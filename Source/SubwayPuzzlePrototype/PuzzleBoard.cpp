#include "PuzzleBoard.h"

#include "SubwayPuzzlePrototype.h"
#include "PuzzleGraphAsset.h"
#include "PuzzleSettings.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "UObject/ConstructorHelpers.h"

APuzzleBoard::APuzzleBoard()
{
    PrimaryActorTick.bCanEverTick = false;

    SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
    SetRootComponent(SceneRoot);

    static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeFinder(TEXT("/Engine/BasicShapes/Cube.Cube"));
    CubeMesh = CubeFinder.Object;

    static ConstructorHelpers::FObjectFinder<UMaterialInterface> MaterialFinder(TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
    BaseMaterial = MaterialFinder.Object;
}

void APuzzleBoard::BeginPlay()
{
    Super::BeginPlay();
    BuildVisuals();
}

void APuzzleBoard::PostInitializeComponents()
{
    Super::PostInitializeComponents();

    // The graph is resolved here rather than in the constructor so that an authored
    // asset can be chosen per placed board, per project, or not at all.
    bShowGreyboxVisuals = UPuzzleSettings::Get()->bShowGreyboxVisuals;

    const UPuzzleGraphAsset* ResolvedAsset = GraphAsset;
    if (!ResolvedAsset)
    {
        const TSoftObjectPtr<UPuzzleGraphAsset>& DefaultGraph = UPuzzleSettings::Get()->DefaultGraphAsset;
        if (!DefaultGraph.IsNull())
        {
            ResolvedAsset = DefaultGraph.LoadSynchronous();
            if (!ResolvedAsset)
            {
                UE_LOG(
                    LogSubwayPuzzle,
                    Error,
                    TEXT("Project default graph '%s' could not be loaded. Falling back to the built-in test graph."),
                    *DefaultGraph.ToString());
            }
        }
    }

    if (ResolvedAsset && ResolvedAsset->IsValidGraph())
    {
        BuildGraphFromAsset(*ResolvedAsset);
        UE_LOG(
            LogSubwayPuzzle,
            Display,
            TEXT("Board loaded graph '%s': %d node(s), %d edge(s), start=%d, goal=%d."),
            *ResolvedAsset->GetName(),
            ResolvedAsset->Nodes.Num(),
            ResolvedAsset->Edges.Num(),
            StartNode,
            GoalNode);
    }
    else
    {
        if (ResolvedAsset)
        {
            UE_LOG(
                LogSubwayPuzzle,
                Error,
                TEXT("Graph asset '%s' is invalid (no nodes, or start/goal out of range). Using the built-in test graph."),
                *ResolvedAsset->GetName());
        }
        BuildFallbackGraph();
    }
}

void APuzzleBoard::ResetGraph(const int32 NodeCount)
{
    Nodes.Reset();
    Nodes.AddDefaulted(NodeCount);
    for (int32 Index = 0; Index < NodeCount; ++Index)
    {
        Nodes[Index].Id = Index;
    }
}

void APuzzleBoard::BuildGraphFromAsset(const UPuzzleGraphAsset& Asset)
{
    ResetGraph(Asset.Nodes.Num());
    for (int32 Index = 0; Index < Asset.Nodes.Num(); ++Index)
    {
        Nodes[Index].LocalLocation = Asset.Nodes[Index].LocalLocation;
    }

    for (const FPuzzleEdgeData& Edge : Asset.Edges)
    {
        ConnectNodes(Edge.NodeA, Edge.NodeB, Edge.BlockReason);
    }

    StartNode = Asset.StartNode;
    GoalNode = Asset.GoalNode;
}

void APuzzleBoard::BuildFallbackGraph()
{
    const FVector Positions[] =
    {
        FVector(0.0,   0.0,   0.0), // 0: start
        FVector(220.0, 0.0,   0.0), // 1
        FVector(440.0, 0.0,   0.0), // 2
        FVector(660.0, 0.0,   0.0), // 3
        FVector(0.0,   220.0, 0.0), // 4
        FVector(220.0, 220.0, 0.0), // 5
        FVector(440.0, 220.0, 0.0), // 6
        FVector(660.0, 220.0, 0.0), // 7
        FVector(220.0, 440.0, 0.0), // 8
        FVector(440.0, 440.0, 0.0)  // 9: goal
    };

    ResetGraph(UE_ARRAY_COUNT(Positions));
    for (int32 Index = 0; Index < UE_ARRAY_COUNT(Positions); ++Index)
    {
        Nodes[Index].LocalLocation = Positions[Index];
    }

    ConnectNodes(0, 1);
    ConnectNodes(0, 4);
    ConnectNodes(1, 2, EPuzzleBlockReason::NarrowDoorway);
    ConnectNodes(1, 5);
    ConnectNodes(2, 3);
    ConnectNodes(2, 6);
    ConnectNodes(3, 7);
    ConnectNodes(4, 5);
    ConnectNodes(5, 6, EPuzzleBlockReason::Stairs); // A* must route through nodes 8 and 9.
    ConnectNodes(5, 8);
    ConnectNodes(6, 7);
    ConnectNodes(6, 9);
    ConnectNodes(8, 9);

    StartNode = 0;
    GoalNode = 9;
}

void APuzzleBoard::ConnectNodes(const int32 NodeA, const int32 NodeB, const EPuzzleBlockReason BlockReason)
{
    if (!Nodes.IsValidIndex(NodeA) || !Nodes.IsValidIndex(NodeB) || NodeA == NodeB)
    {
        UE_LOG(LogSubwayPuzzle, Warning, TEXT("Ignoring invalid edge %d-%d."), NodeA, NodeB);
        return;
    }

    Nodes[NodeA].Edges.Add({NodeB, BlockReason});
    Nodes[NodeB].Edges.Add({NodeA, BlockReason});
}

FString APuzzleBoard::DescribeBlockReason(const EPuzzleBlockReason Reason)
{
    switch (Reason)
    {
    case EPuzzleBlockReason::Stairs:        return TEXT("stairs");
    case EPuzzleBlockReason::NarrowDoorway: return TEXT("a doorway that is too narrow");
    case EPuzzleBlockReason::StepGap:       return TEXT("a step you cannot clear");
    case EPuzzleBlockReason::Other:         return TEXT("an obstacle");
    default:                                return TEXT("nothing");
    }
}

FVector APuzzleBoard::GetNodeWorldLocation(const int32 NodeIndex) const
{
    return Nodes.IsValidIndex(NodeIndex)
        ? GetActorTransform().TransformPosition(Nodes[NodeIndex].LocalLocation)
        : GetActorLocation();
}

FVector APuzzleBoard::GetBoardCenter() const
{
    if (Nodes.IsEmpty())
    {
        return GetActorLocation();
    }

    FVector Sum = FVector::ZeroVector;
    for (const FPuzzleNode& Node : Nodes)
    {
        Sum += GetActorTransform().TransformPosition(Node.LocalLocation);
    }
    return Sum / static_cast<double>(Nodes.Num());
}

bool APuzzleBoard::ResolveNodeFromHit(const FHitResult& Hit, int32& OutNodeIndex) const
{
    OutNodeIndex = INDEX_NONE;
    const UPrimitiveComponent* HitComponent = Hit.GetComponent();
    if (!HitComponent)
    {
        return false;
    }

    if (const int32* NodeIndex = ClickableNodeByComponent.Find(HitComponent))
    {
        OutNodeIndex = *NodeIndex;
        return true;
    }
    return false;
}

bool APuzzleBoard::FindDirectionalTarget(
    const int32 FromNode,
    const FVector2D& Direction,
    int32& OutTargetNode,
    EPuzzleBlockReason& OutBlockReason) const
{
    OutTargetNode = INDEX_NONE;
    OutBlockReason = EPuzzleBlockReason::None;

    if (!Nodes.IsValidIndex(FromNode) || Direction.IsNearlyZero())
    {
        return false;
    }

    const FVector2D DesiredDirection = Direction.GetSafeNormal();
    float BestDot = 0.7f;

    for (const FPuzzleEdge& Edge : Nodes[FromNode].Edges)
    {
        if (!Nodes.IsValidIndex(Edge.TargetNode))
        {
            continue;
        }

        const FVector Delta3D = Nodes[Edge.TargetNode].LocalLocation - Nodes[FromNode].LocalLocation;
        const FVector2D EdgeDirection(Delta3D.X, Delta3D.Y);
        const float Dot = FVector2D::DotProduct(DesiredDirection, EdgeDirection.GetSafeNormal());
        if (Dot > BestDot)
        {
            BestDot = Dot;
            OutTargetNode = Edge.TargetNode;
            OutBlockReason = Edge.BlockReason;
        }
    }

    return OutTargetNode != INDEX_NONE;
}

float APuzzleBoard::EstimateCost(const int32 FromNode, const int32 ToNode) const
{
    if (!Nodes.IsValidIndex(FromNode) || !Nodes.IsValidIndex(ToNode))
    {
        return TNumericLimits<float>::Max();
    }
    return FVector::Distance(Nodes[FromNode].LocalLocation, Nodes[ToNode].LocalLocation);
}

bool APuzzleBoard::FindPathAStar(const int32 StartNodeIndex, const int32 GoalNodeIndex, TArray<int32>& OutPath) const
{
    OutPath.Reset();
    if (!Nodes.IsValidIndex(StartNodeIndex) || !Nodes.IsValidIndex(GoalNodeIndex))
    {
        return false;
    }
    if (StartNodeIndex == GoalNodeIndex)
    {
        return true;
    }

    TArray<float> GScore;
    TArray<float> FScore;
    TArray<int32> CameFrom;
    TArray<int32> OpenSet;

    GScore.Init(TNumericLimits<float>::Max(), Nodes.Num());
    FScore.Init(TNumericLimits<float>::Max(), Nodes.Num());
    CameFrom.Init(INDEX_NONE, Nodes.Num());

    GScore[StartNodeIndex] = 0.0f;
    FScore[StartNodeIndex] = EstimateCost(StartNodeIndex, GoalNodeIndex);
    OpenSet.Add(StartNodeIndex);

    while (!OpenSet.IsEmpty())
    {
        int32 BestOpenIndex = 0;
        for (int32 Index = 1; Index < OpenSet.Num(); ++Index)
        {
            if (FScore[OpenSet[Index]] < FScore[OpenSet[BestOpenIndex]])
            {
                BestOpenIndex = Index;
            }
        }

        const int32 CurrentNode = OpenSet[BestOpenIndex];
        OpenSet.RemoveAtSwap(BestOpenIndex);

        if (CurrentNode == GoalNodeIndex)
        {
            int32 TraceNode = GoalNodeIndex;
            while (TraceNode != StartNodeIndex)
            {
                OutPath.Insert(TraceNode, 0);
                TraceNode = CameFrom[TraceNode];
                if (TraceNode == INDEX_NONE)
                {
                    OutPath.Reset();
                    return false;
                }
            }
            return true;
        }

        for (const FPuzzleEdge& Edge : Nodes[CurrentNode].Edges)
        {
            if (Edge.IsBlocked() || !Nodes.IsValidIndex(Edge.TargetNode))
            {
                continue;
            }

            const float TentativeGScore = GScore[CurrentNode] + EstimateCost(CurrentNode, Edge.TargetNode);
            if (TentativeGScore < GScore[Edge.TargetNode])
            {
                CameFrom[Edge.TargetNode] = CurrentNode;
                GScore[Edge.TargetNode] = TentativeGScore;
                FScore[Edge.TargetNode] = TentativeGScore + EstimateCost(Edge.TargetNode, GoalNodeIndex);
                OpenSet.AddUnique(Edge.TargetNode);
            }
        }
    }

    return false;
}

UMaterialInstanceDynamic* APuzzleBoard::CreateColorMaterial(const FName& Name, const FLinearColor& Color)
{
    if (!BaseMaterial)
    {
        return nullptr;
    }

    UMaterialInstanceDynamic* Material = UMaterialInstanceDynamic::Create(BaseMaterial, this, Name);
    Material->SetVectorParameterValue(TEXT("Color"), Color);
    return Material;
}

UStaticMeshComponent* APuzzleBoard::CreateBoxComponent(
    const FString& ComponentName,
    const FVector& RelativeLocation,
    const FRotator& RelativeRotation,
    const FVector& RelativeScale,
    UMaterialInterface* Material,
    const bool bClickable)
{
    UStaticMeshComponent* Component = NewObject<UStaticMeshComponent>(this, *ComponentName);
    AddInstanceComponent(Component);
    Component->SetupAttachment(SceneRoot);
    Component->SetStaticMesh(CubeMesh);
    Component->SetRelativeLocation(RelativeLocation);
    Component->SetRelativeRotation(RelativeRotation);
    Component->SetRelativeScale3D(RelativeScale);
    Component->SetMaterial(0, Material);
    Component->SetGenerateOverlapEvents(false);
    Component->SetCollisionEnabled(bClickable ? ECollisionEnabled::QueryOnly : ECollisionEnabled::NoCollision);
    Component->SetCollisionResponseToAllChannels(ECR_Ignore);
    if (bClickable)
    {
        Component->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
    }
    Component->RegisterComponent();
    return Component;
}

void APuzzleBoard::CreateConnectionVisual(const int32 NodeA, const int32 NodeB, const bool bBlocked)
{
    const FVector Start = Nodes[NodeA].LocalLocation;
    const FVector End = Nodes[NodeB].LocalLocation;
    const FVector Delta = End - Start;
    const FVector Midpoint = (Start + End) * 0.5;

    if (bBlocked)
    {
        const FRotator BarrierRotation = Delta.Rotation() + FRotator(0.0, 90.0, 0.0);
        CreateBoxComponent(
            FString::Printf(TEXT("Blocked_%d_%d"), NodeA, NodeB),
            Midpoint + FVector(0.0, 0.0, 42.0),
            BarrierRotation,
            FVector(0.10, 0.85, 0.65),
            BlockedMaterial,
            false);
        return;
    }

    const float VisibleLength = FMath::Max(Delta.Size() - 110.0f, 10.0f);
    CreateBoxComponent(
        FString::Printf(TEXT("Connection_%d_%d"), NodeA, NodeB),
        Midpoint + FVector(0.0, 0.0, 4.0),
        Delta.Rotation(),
        FVector(VisibleLength / 100.0f, 0.12, 0.05),
        ConnectionMaterial,
        false);
}

void APuzzleBoard::BuildVisuals()
{
    if (NodeTiles.Num() > 0)
    {
        return;
    }

    // A missing engine mesh or material used to abort silently, which shows up as an
    // empty black frame with no clue as to why. /Engine/BasicShapes is not cooked into
    // a packaged build unless DefaultGame.ini lists it under DirectoriesToAlwaysCook,
    // so this is the most likely place for a black screen to come back. Fail loudly.
    if (!CubeMesh || !BaseMaterial)
    {
        UE_LOG(
            LogSubwayPuzzle,
            Error,
            TEXT("PuzzleBoard has no greybox assets (CubeMesh=%s, BaseMaterial=%s). The board ")
            TEXT("cannot be drawn. Confirm /Engine/BasicShapes is available and cooked."),
            CubeMesh ? TEXT("ok") : TEXT("MISSING"),
            BaseMaterial ? TEXT("ok") : TEXT("MISSING"));
        return;
    }

    if (Nodes.IsEmpty())
    {
        UE_LOG(LogSubwayPuzzle, Error, TEXT("PuzzleBoard has no nodes to draw."));
        return;
    }

    NodeMaterial = CreateColorMaterial(TEXT("NodeMaterial"), FLinearColor(0.04f, 0.42f, 0.80f));
    GoalMaterial = CreateColorMaterial(TEXT("GoalMaterial"), FLinearColor(0.08f, 0.80f, 0.25f));
    ConnectionMaterial = CreateColorMaterial(TEXT("ConnectionMaterial"), FLinearColor(0.18f, 0.23f, 0.30f));
    BlockedMaterial = CreateColorMaterial(TEXT("BlockedMaterial"), FLinearColor(0.90f, 0.04f, 0.04f));
    PlatformMaterial = CreateColorMaterial(TEXT("PlatformMaterial"), FLinearColor(0.025f, 0.035f, 0.055f));

    // Fit the platform to whatever graph was authored instead of the old hard-coded
    // extents, which only matched the built-in ten-node layout.
    FBox NodeBounds(ForceInit);
    for (const FPuzzleNode& Node : Nodes)
    {
        NodeBounds += Node.LocalLocation;
    }

    if (bShowGreyboxVisuals)
    {
        const FVector BoundsCenter = NodeBounds.GetCenter();
        const FVector BoundsExtent = NodeBounds.GetExtent();
        constexpr double PlatformMargin = 150.0;
        constexpr double CubeSize = 100.0;

        CreateBoxComponent(
            TEXT("Platform"),
            FVector(BoundsCenter.X, BoundsCenter.Y, NodeBounds.Min.Z - 45.0),
            FRotator::ZeroRotator,
            FVector(
                (BoundsExtent.X * 2.0 + PlatformMargin * 2.0) / CubeSize,
                (BoundsExtent.Y * 2.0 + PlatformMargin * 2.0) / CubeSize,
                0.25),
            PlatformMaterial,
            false);
    }

    // Node tiles are always built, even with the greybox switched off: they are the
    // click targets that ResolveNodeFromHit maps back to node indices. Hiding a
    // component stops it rendering but leaves its collision intact, so the cursor trace
    // on ECC_Visibility still hits them once real geometry replaces the greybox.
    for (const FPuzzleNode& Node : Nodes)
    {
        UStaticMeshComponent* Tile = CreateBoxComponent(
            FString::Printf(TEXT("NodeTile_%d"), Node.Id),
            Node.LocalLocation,
            FRotator::ZeroRotator,
            FVector(0.72, 0.72, 0.12),
            Node.Id == GoalNode ? GoalMaterial : NodeMaterial,
            true);
        Tile->SetVisibility(bShowGreyboxVisuals);
        NodeTiles.Add(Tile);
        ClickableNodeByComponent.Add(Tile, Node.Id);
    }

    if (!bShowGreyboxVisuals)
    {
        UE_LOG(
            LogSubwayPuzzle,
            Display,
            TEXT("Greybox visuals are off: %d invisible node tile(s) kept as click targets."),
            NodeTiles.Num());
        return;
    }

    for (const FPuzzleNode& Node : Nodes)
    {
        for (const FPuzzleEdge& Edge : Node.Edges)
        {
            if (Node.Id < Edge.TargetNode)
            {
                CreateConnectionVisual(Node.Id, Edge.TargetNode, Edge.IsBlocked());
            }
        }
    }
}
