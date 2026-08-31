#include "PuzzlePawn.h"

#include "PuzzleBoard.h"
#include "PuzzlePlayerController.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "EngineUtils.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "UObject/ConstructorHelpers.h"

APuzzlePawn::APuzzlePawn()
{
    PrimaryActorTick.bCanEverTick = true;

    SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
    SetRootComponent(SceneRoot);

    BodyMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BodyMesh"));
    BodyMesh->SetupAttachment(SceneRoot);
    BodyMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    BodyMesh->SetRelativeScale3D(FVector(0.48));

    static ConstructorHelpers::FObjectFinder<UStaticMesh> SphereFinder(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
    if (SphereFinder.Succeeded())
    {
        BodyMesh->SetStaticMesh(SphereFinder.Object);
    }

    static ConstructorHelpers::FObjectFinder<UMaterialInterface> MaterialFinder(TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
    if (MaterialFinder.Succeeded())
    {
        BodyMesh->SetMaterial(0, MaterialFinder.Object);
    }
}

void APuzzlePawn::BeginPlay()
{
    Super::BeginPlay();

    if (UMaterialInterface* BaseMaterial = BodyMesh->GetMaterial(0))
    {
        BodyMaterial = UMaterialInstanceDynamic::Create(BaseMaterial, this, TEXT("PlayerMaterial"));
        BodyMaterial->SetVectorParameterValue(TEXT("Color"), FLinearColor(1.0f, 0.72f, 0.03f));
        BodyMesh->SetMaterial(0, BodyMaterial);
    }

    if (EnsureBoard())
    {
        CurrentNode = Board->GetStartNode();
        SetActorLocation(Board->GetNodeWorldLocation(CurrentNode) + FVector(0.0, 0.0, HeightAboveTile));
        ReportFeedback(TEXT("Ready. Use WASD / Arrow Keys or click a numbered tile."), FLinearColor::White);
    }
}

bool APuzzlePawn::EnsureBoard()
{
    if (IsValid(Board))
    {
        return true;
    }

    for (TActorIterator<APuzzleBoard> It(GetWorld()); It; ++It)
    {
        Board = *It;
        return true;
    }
    return false;
}

void APuzzlePawn::ReportFeedback(const FString& Message, const FLinearColor& Color) const
{
    if (APuzzlePlayerController* PuzzleController = Cast<APuzzlePlayerController>(GetController()))
    {
        PuzzleController->ShowFeedback(Message, Color);
    }
}

void APuzzlePawn::RequestDirectionalStep(const FVector2D& Direction)
{
    if (!EnsureBoard() || CurrentNode == INDEX_NONE)
    {
        ReportFeedback(TEXT("Board is not ready."), FLinearColor::Red);
        return;
    }
    if (IsFollowingPath())
    {
        ReportFeedback(TEXT("Finish the current move first."), FLinearColor(1.0f, 0.65f, 0.05f));
        return;
    }

    int32 TargetNode = INDEX_NONE;
    EPuzzleBlockReason BlockReason = EPuzzleBlockReason::None;
    if (!Board->FindDirectionalTarget(CurrentNode, Direction, TargetNode, BlockReason))
    {
        ReportFeedback(TEXT("No node exists in that direction."), FLinearColor::Red);
        return;
    }
    if (BlockReason != EPuzzleBlockReason::None)
    {
        ReportFeedback(
            FString::Printf(
                TEXT("Node %d is blocked by %s."),
                TargetNode,
                *APuzzleBoard::DescribeBlockReason(BlockReason)),
            FLinearColor::Red);
        return;
    }

    PendingPath.Add(TargetNode);
    ReportFeedback(FString::Printf(TEXT("Step to Node %d"), TargetNode), FLinearColor(0.45f, 0.85f, 1.0f));
}

void APuzzlePawn::RequestPathToNode(const int32 GoalNode)
{
    if (!EnsureBoard() || CurrentNode == INDEX_NONE)
    {
        ReportFeedback(TEXT("Board is not ready."), FLinearColor::Red);
        return;
    }
    if (IsFollowingPath())
    {
        ReportFeedback(TEXT("Finish the current path first."), FLinearColor(1.0f, 0.65f, 0.05f));
        return;
    }
    if (GoalNode == CurrentNode)
    {
        ReportFeedback(FString::Printf(TEXT("Already at Node %d."), CurrentNode), FLinearColor::White);
        return;
    }

    TArray<int32> NewPath;
    if (!Board->FindPathAStar(CurrentNode, GoalNode, NewPath) || NewPath.IsEmpty())
    {
        ReportFeedback(
            FString::Printf(TEXT("Node %d is unreachable because of blocked routes."), GoalNode),
            FLinearColor::Red);
        return;
    }

    PendingPath = MoveTemp(NewPath);
    ReportFeedback(
        FString::Printf(TEXT("A* path accepted: %d step(s) to Node %d."), PendingPath.Num(), GoalNode),
        FLinearColor(0.45f, 0.85f, 1.0f));
}

void APuzzlePawn::Tick(const float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);

    if (!EnsureBoard() || PendingPath.IsEmpty())
    {
        return;
    }

    const int32 TargetNode = PendingPath[0];
    const FVector TargetLocation = Board->GetNodeWorldLocation(TargetNode) + FVector(0.0, 0.0, HeightAboveTile);
    const FVector NewLocation = FMath::VInterpConstantTo(GetActorLocation(), TargetLocation, DeltaSeconds, MoveSpeed);
    SetActorLocation(NewLocation);

    if (NewLocation.Equals(TargetLocation, 0.5f))
    {
        SetActorLocation(TargetLocation);
        CurrentNode = TargetNode;
        PendingPath.RemoveAt(0);

        if (CurrentNode == Board->GetGoalNode())
        {
            ReportFeedback(TEXT("Goal reached! Node 9 is the green tile."), FLinearColor(0.15f, 1.0f, 0.25f));
        }
        else if (PendingPath.IsEmpty())
        {
            ReportFeedback(FString::Printf(TEXT("Arrived at Node %d."), CurrentNode), FLinearColor::White);
        }
    }
}
