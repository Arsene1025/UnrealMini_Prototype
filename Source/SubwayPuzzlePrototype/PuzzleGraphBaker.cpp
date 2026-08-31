#include "PuzzleGraphBaker.h"

#include "SubwayPuzzlePrototype.h"
#include "PuzzleGraphAsset.h"
#include "PuzzleNodeMarker.h"
#include "Components/SceneComponent.h"
#include "DrawDebugHelpers.h"
#include "Engine/World.h"
#include "EngineUtils.h"

APuzzleGraphBaker::APuzzleGraphBaker()
{
    PrimaryActorTick.bCanEverTick = true;
    PrimaryActorTick.bStartWithTickEnabled = true;

    SetRootComponent(CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot")));
}

void APuzzleGraphBaker::CollectSortedMarkers(TArray<APuzzleNodeMarker*>& OutMarkers) const
{
    OutMarkers.Reset();
    for (TActorIterator<APuzzleNodeMarker> It(GetWorld()); It; ++It)
    {
        OutMarkers.Add(*It);
    }

    // TActorIterator order is not stable between sessions, so impose a deterministic
    // one. Floor-major (Z, then Y, then X) keeps indices readable once the second floor
    // from the design brief exists.
    OutMarkers.Sort([](const APuzzleNodeMarker& A, const APuzzleNodeMarker& B)
    {
        const FVector LocationA = A.GetActorLocation();
        const FVector LocationB = B.GetActorLocation();
        if (!FMath::IsNearlyEqual(LocationA.Z, LocationB.Z, 1.0)) { return LocationA.Z < LocationB.Z; }
        if (!FMath::IsNearlyEqual(LocationA.Y, LocationB.Y, 1.0)) { return LocationA.Y < LocationB.Y; }
        return LocationA.X < LocationB.X;
    });
}

void APuzzleGraphBaker::BakeFromLevel()
{
    if (!TargetAsset)
    {
        UE_LOG(LogSubwayPuzzle, Error, TEXT("Bake failed: assign a Target Asset first."));
        return;
    }

    TArray<APuzzleNodeMarker*> Markers;
    CollectSortedMarkers(Markers);
    if (Markers.IsEmpty())
    {
        UE_LOG(LogSubwayPuzzle, Error, TEXT("Bake failed: no APuzzleNodeMarker actors in this level."));
        return;
    }

    TMap<const APuzzleNodeMarker*, int32> IndexByMarker;
    for (int32 Index = 0; Index < Markers.Num(); ++Index)
    {
        IndexByMarker.Add(Markers[Index], Index);
    }

    TargetAsset->Modify();
    TargetAsset->Nodes.Reset();
    TargetAsset->Edges.Reset();
    TargetAsset->StartNode = INDEX_NONE;
    TargetAsset->GoalNode = INDEX_NONE;

    const FTransform BoardOrigin = GetActorTransform();
    for (int32 Index = 0; Index < Markers.Num(); ++Index)
    {
        const APuzzleNodeMarker* Marker = Markers[Index];

        FPuzzleNodeData& Node = TargetAsset->Nodes.AddDefaulted_GetRef();
        Node.LocalLocation = BoardOrigin.InverseTransformPosition(Marker->GetActorLocation());
        Node.Label = Marker->Label;

        if (Marker->bIsStartNode)
        {
            if (TargetAsset->StartNode != INDEX_NONE)
            {
                UE_LOG(LogSubwayPuzzle, Warning, TEXT("More than one start node; keeping node %d."), TargetAsset->StartNode);
            }
            else
            {
                TargetAsset->StartNode = Index;
            }
        }

        if (Marker->bIsGoalNode)
        {
            if (TargetAsset->GoalNode != INDEX_NONE)
            {
                UE_LOG(LogSubwayPuzzle, Warning, TEXT("More than one goal node; keeping node %d."), TargetAsset->GoalNode);
            }
            else
            {
                TargetAsset->GoalNode = Index;
            }
        }
    }

    // Links are authored per marker but the graph is undirected, so the same connection
    // can be written from either or both ends. Key on the ordered pair to merge them.
    TMap<TPair<int32, int32>, int32> EdgeIndexByPair;
    for (const APuzzleNodeMarker* Marker : Markers)
    {
        const int32 FromIndex = IndexByMarker[Marker];
        for (const FPuzzleMarkerLink& Link : Marker->Links)
        {
            const int32* ToIndexPtr = Link.Target ? IndexByMarker.Find(Link.Target) : nullptr;
            if (!ToIndexPtr)
            {
                UE_LOG(LogSubwayPuzzle, Warning, TEXT("Node %d has a link with no valid target; skipped."), FromIndex);
                continue;
            }

            if (*ToIndexPtr == FromIndex)
            {
                UE_LOG(LogSubwayPuzzle, Warning, TEXT("Node %d links to itself; skipped."), FromIndex);
                continue;
            }

            const TPair<int32, int32> Key(FMath::Min(FromIndex, *ToIndexPtr), FMath::Max(FromIndex, *ToIndexPtr));
            if (const int32* ExistingEdge = EdgeIndexByPair.Find(Key))
            {
                FPuzzleEdgeData& Existing = TargetAsset->Edges[*ExistingEdge];
                if (Existing.BlockReason == EPuzzleBlockReason::None)
                {
                    Existing.BlockReason = Link.BlockReason;
                }
                else if (Link.BlockReason != EPuzzleBlockReason::None && Link.BlockReason != Existing.BlockReason)
                {
                    UE_LOG(
                        LogSubwayPuzzle,
                        Warning,
                        TEXT("Edge %d-%d is authored with two different block reasons; keeping the first."),
                        Key.Key,
                        Key.Value);
                }
                continue;
            }

            FPuzzleEdgeData& Edge = TargetAsset->Edges.AddDefaulted_GetRef();
            Edge.NodeA = Key.Key;
            Edge.NodeB = Key.Value;
            Edge.BlockReason = Link.BlockReason;
            EdgeIndexByPair.Add(Key, TargetAsset->Edges.Num() - 1);
        }
    }

    if (TargetAsset->StartNode == INDEX_NONE)
    {
        TargetAsset->StartNode = 0;
        UE_LOG(LogSubwayPuzzle, Warning, TEXT("No marker flagged as start; defaulting to node 0."));
    }

    if (TargetAsset->GoalNode == INDEX_NONE)
    {
        TargetAsset->GoalNode = Markers.Num() - 1;
        UE_LOG(LogSubwayPuzzle, Warning, TEXT("No marker flagged as goal; defaulting to node %d."), TargetAsset->GoalNode);
    }

    TargetAsset->MarkPackageDirty();

    UE_LOG(
        LogSubwayPuzzle,
        Display,
        TEXT("Baked %d node(s) and %d edge(s) into '%s'. Start=%d Goal=%d. Save the asset to keep it."),
        TargetAsset->Nodes.Num(),
        TargetAsset->Edges.Num(),
        *TargetAsset->GetName(),
        TargetAsset->StartNode,
        TargetAsset->GoalNode);
}

void APuzzleGraphBaker::LoadIntoLevel()
{
    if (!TargetAsset || TargetAsset->Nodes.IsEmpty())
    {
        UE_LOG(LogSubwayPuzzle, Error, TEXT("Load failed: Target Asset is unset or has no nodes."));
        return;
    }

    TArray<APuzzleNodeMarker*> ExistingMarkers;
    CollectSortedMarkers(ExistingMarkers);
    for (APuzzleNodeMarker* Marker : ExistingMarkers)
    {
        Marker->Destroy();
    }

    const FTransform BoardOrigin = GetActorTransform();
    TArray<APuzzleNodeMarker*> Spawned;
    Spawned.Reserve(TargetAsset->Nodes.Num());

    for (int32 Index = 0; Index < TargetAsset->Nodes.Num(); ++Index)
    {
        const FPuzzleNodeData& Node = TargetAsset->Nodes[Index];
        APuzzleNodeMarker* Marker = GetWorld()->SpawnActor<APuzzleNodeMarker>(
            APuzzleNodeMarker::StaticClass(),
            BoardOrigin.TransformPosition(Node.LocalLocation),
            FRotator::ZeroRotator);

        if (!Marker)
        {
            UE_LOG(LogSubwayPuzzle, Error, TEXT("Failed to spawn a marker for node %d."), Index);
            Spawned.Add(nullptr);
            continue;
        }

        Marker->Label = Node.Label;
        Marker->bIsStartNode = (Index == TargetAsset->StartNode);
        Marker->bIsGoalNode = (Index == TargetAsset->GoalNode);
        Spawned.Add(Marker);
    }

    // Author each edge from its lower-index end only; baking treats links as undirected.
    for (const FPuzzleEdgeData& Edge : TargetAsset->Edges)
    {
        if (!Spawned.IsValidIndex(Edge.NodeA) || !Spawned.IsValidIndex(Edge.NodeB)
            || !Spawned[Edge.NodeA] || !Spawned[Edge.NodeB])
        {
            UE_LOG(LogSubwayPuzzle, Warning, TEXT("Edge %d-%d references a missing node; skipped."), Edge.NodeA, Edge.NodeB);
            continue;
        }

        FPuzzleMarkerLink& Link = Spawned[Edge.NodeA]->Links.AddDefaulted_GetRef();
        Link.Target = Spawned[Edge.NodeB];
        Link.BlockReason = Edge.BlockReason;
    }

    UE_LOG(LogSubwayPuzzle, Display, TEXT("Loaded %d marker(s) from '%s'."), Spawned.Num(), *TargetAsset->GetName());
}

void APuzzleGraphBaker::Tick(const float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);

    if (!bDrawGraphPreview || !GetWorld())
    {
        return;
    }

    // Redrawn every tick with a zero lifetime so the preview follows markers as they are
    // dragged around the viewport.
    for (TActorIterator<APuzzleNodeMarker> It(GetWorld()); It; ++It)
    {
        const APuzzleNodeMarker* Marker = *It;
        const FVector From = Marker->GetActorLocation();

        if (Marker->bIsStartNode)
        {
            DrawDebugSphere(GetWorld(), From + FVector(0.0, 0.0, 60.0), 26.0f, 12, FColor::Yellow, false, -1.0f, 0, 2.0f);
        }

        if (Marker->bIsGoalNode)
        {
            DrawDebugSphere(GetWorld(), From + FVector(0.0, 0.0, 60.0), 34.0f, 12, FColor::Green, false, -1.0f, 0, 2.0f);
        }

        for (const FPuzzleMarkerLink& Link : Marker->Links)
        {
            if (!Link.Target)
            {
                continue;
            }

            const FColor LineColor = Link.BlockReason == EPuzzleBlockReason::None ? FColor::Cyan : FColor::Red;
            DrawDebugLine(GetWorld(), From, Link.Target->GetActorLocation(), LineColor, false, -1.0f, 0, 6.0f);
        }
    }
}
