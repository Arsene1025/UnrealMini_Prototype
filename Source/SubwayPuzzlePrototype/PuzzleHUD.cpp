#include "PuzzleHUD.h"

#include "PuzzleBoard.h"
#include "PuzzlePawn.h"
#include "PuzzlePlayerController.h"
#include "Engine/Engine.h"
#include "EngineUtils.h"

void APuzzleHUD::DrawHUD()
{
    Super::DrawHUD();

    APuzzlePlayerController* PuzzleController = Cast<APuzzlePlayerController>(GetOwningPlayerController());
    APuzzlePawn* PuzzlePawn = PuzzleController ? Cast<APuzzlePawn>(PuzzleController->GetPawn()) : nullptr;

    DrawText(TEXT("SUBWAY PUZZLE - C++ GRAPH PROTOTYPE"), FLinearColor::White, 35.0f, 30.0f, GEngine->GetMediumFont(), 1.1f);
    DrawText(TEXT("WASD / Arrow Keys: one-node move"), FLinearColor(0.75f, 0.82f, 0.90f), 35.0f, 65.0f);
    DrawText(TEXT("Left Click: A* path to a tile"), FLinearColor(0.75f, 0.82f, 0.90f), 35.0f, 88.0f);
    DrawText(TEXT("Red barriers: blocked graph edges"), FLinearColor(1.0f, 0.35f, 0.35f), 35.0f, 111.0f);

    if (PuzzlePawn)
    {
        DrawText(
            FString::Printf(TEXT("Current Node: %d%s"), PuzzlePawn->GetCurrentNode(), PuzzlePawn->IsFollowingPath() ? TEXT("  (moving)") : TEXT("")),
            FLinearColor(1.0f, 0.82f, 0.2f),
            35.0f,
            145.0f);
    }

    if (PuzzleController)
    {
        DrawText(PuzzleController->GetFeedbackText(), PuzzleController->GetFeedbackColor(), 35.0f, 178.0f, GEngine->GetMediumFont());
    }

    APuzzleBoard* Board = nullptr;
    for (TActorIterator<APuzzleBoard> It(GetWorld()); It; ++It)
    {
        Board = *It;
        break;
    }

    // Node numbers belong to the greybox. Once real geometry replaces it the tiles are
    // invisible click targets, and floating numbers over them would be noise.
    if (Board && PuzzleController && Board->ShouldShowGreybox())
    {
        for (int32 NodeIndex = 0; NodeIndex < Board->GetNodeCount(); ++NodeIndex)
        {
            FVector2D ScreenLocation;
            const FVector LabelLocation = Board->GetNodeWorldLocation(NodeIndex) + FVector(0.0, 0.0, 48.0);
            if (PuzzleController->ProjectWorldLocationToScreen(LabelLocation, ScreenLocation))
            {
                const FLinearColor LabelColor = NodeIndex == Board->GetGoalNode()
                    ? FLinearColor(0.2f, 1.0f, 0.3f)
                    : FLinearColor::White;
                DrawText(FString::FromInt(NodeIndex), LabelColor, ScreenLocation.X - 5.0f, ScreenLocation.Y - 8.0f, GEngine->GetMediumFont(), 1.1f);
            }
        }
    }
}
