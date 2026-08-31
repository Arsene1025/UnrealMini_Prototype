#include "PuzzleGameMode.h"

#include "SubwayPuzzlePrototype.h"
#include "PuzzleBoard.h"
#include "PuzzleHUD.h"
#include "PuzzlePawn.h"
#include "PuzzlePlayerController.h"
#include "Camera/CameraActor.h"
#include "Camera/CameraComponent.h"
#include "Components/DirectionalLightComponent.h"
#include "Components/LightComponent.h"
#include "Engine/DirectionalLight.h"
#include "Engine/World.h"

APuzzleGameMode::APuzzleGameMode()
{
    DefaultPawnClass = APuzzlePawn::StaticClass();
    PlayerControllerClass = APuzzlePlayerController::StaticClass();
    HUDClass = APuzzleHUD::StaticClass();
}

ADirectionalLight* APuzzleGameMode::SpawnRigLight(
    const FVector& Location,
    const FRotator& Rotation,
    const float Intensity,
    const FLinearColor& Color,
    const bool bCastShadows,
    const int32 ForwardShadingPriority)
{
    ADirectionalLight* Light = GetWorld()->SpawnActor<ADirectionalLight>(ADirectionalLight::StaticClass(), Location, Rotation);
    if (!Light)
    {
        UE_LOG(LogSubwayPuzzle, Error, TEXT("Failed to spawn a rig light; the scene will be under-lit."));
        return nullptr;
    }

    // ADirectionalLight defaults to Stationary mobility, which expects baked lighting
    // data a runtime-spawned light in an unbuilt map can never have. Set Movable first
    // and before any other setter: ULightComponent's setters are gated on
    // AreDynamicDataChangesAllowed() and several of them silently drop the call on a
    // non-Movable component, leaving the light at its defaults with no error anywhere.
    Light->SetMobility(EComponentMobility::Movable);

    ULightComponent* LightComponent = Light->GetLightComponent();
    LightComponent->SetIntensity(Intensity);
    LightComponent->SetLightColor(Color);
    LightComponent->SetCastShadows(bCastShadows);

    // Every directional light claims to be the atmosphere sun and the forward-shading
    // light by default. With more than one in the world that is ambiguous and the
    // renderer warns on screen, so give the rig an explicit pecking order.
    if (UDirectionalLightComponent* DirectionalComponent = Cast<UDirectionalLightComponent>(LightComponent))
    {
        DirectionalComponent->SetForwardShadingPriority(ForwardShadingPriority);
        DirectionalComponent->SetAtmosphereSunLight(ForwardShadingPriority > 0);
    }

    return Light;
}

void APuzzleGameMode::BuildLightingRig(const FVector& FocusPoint)
{
    // /Engine/Maps/Entry is intentionally empty: no lights, no sky, no ambient. Without
    // a rig spawned here every lit mesh renders black and the game looks like a dead
    // frame. Three shadow-free-except-key directional lights are used on purpose --
    // directional lights have no attenuation radius, so coverage of the whole board is
    // guaranteed and there is no falloff tuning that can silently go dark.
    SpawnRigLight(
        FocusPoint + FVector(0.0, 0.0, 800.0),
        FRotator(-52.0, -35.0, 0.0),
        9.0f,
        FLinearColor(1.0f, 0.93f, 0.84f),
        true,
        2);

    // Cool fill from the opposite side so faces turned away from the key are readable.
    SpawnRigLight(
        FocusPoint + FVector(0.0, 0.0, 800.0),
        FRotator(-30.0, 135.0, 0.0),
        3.2f,
        FLinearColor(0.55f, 0.68f, 1.0f),
        false,
        0);

    // Upward bounce standing in for ambient, so no surface is ever pure black.
    SpawnRigLight(
        FocusPoint + FVector(0.0, 0.0, -400.0),
        FRotator(55.0, 20.0, 0.0),
        1.2f,
        FLinearColor(0.60f, 0.66f, 0.78f),
        false,
        0);
}

void APuzzleGameMode::StartPlay()
{
    if (!SpawnedBoard)
    {
        SpawnedBoard = GetWorld()->SpawnActor<APuzzleBoard>(APuzzleBoard::StaticClass(), FVector::ZeroVector, FRotator::ZeroRotator);
    }

    if (!SpawnedBoard)
    {
        UE_LOG(LogSubwayPuzzle, Error, TEXT("Failed to spawn APuzzleBoard. Nothing will be visible this session."));
        Super::StartPlay();
        return;
    }

    // The graph now comes from an authored asset, so the old fixed expectations (node 9
    // is the goal, edge 1->2 is blocked) no longer hold. Check the invariant that
    // actually matters for any stage: the goal must be reachable from the start.
    TArray<int32> SmokeTestPath;
    const bool bGoalIsReachable = SpawnedBoard->FindPathAStar(
        SpawnedBoard->GetStartNode(),
        SpawnedBoard->GetGoalNode(),
        SmokeTestPath);

    if (bGoalIsReachable)
    {
        UE_LOG(
            LogSubwayPuzzle,
            Display,
            TEXT("Graph self-check passed: A* found %d step(s) from node %d to node %d."),
            SmokeTestPath.Num(),
            SpawnedBoard->GetStartNode(),
            SpawnedBoard->GetGoalNode());
    }
    else
    {
        UE_LOG(
            LogSubwayPuzzle,
            Error,
            TEXT("Graph self-check FAILED: node %d cannot reach goal node %d. This stage is unsolvable."),
            SpawnedBoard->GetStartNode(),
            SpawnedBoard->GetGoalNode());
    }

    const FVector FocusPoint = SpawnedBoard->GetBoardCenter();

    BuildLightingRig(FocusPoint);

    const FVector CameraLocation = FocusPoint + FVector(-850.0, -900.0, 1150.0);
    const FRotator CameraRotation = (FocusPoint - CameraLocation).Rotation();

    ACameraActor* PuzzleCamera = GetWorld()->SpawnActor<ACameraActor>(ACameraActor::StaticClass(), CameraLocation, CameraRotation);
    if (PuzzleCamera)
    {
        PuzzleCamera->Tags.Add(TEXT("PuzzleCamera"));
        PuzzleCamera->GetCameraComponent()->SetFieldOfView(48.0f);
    }
    else
    {
        UE_LOG(LogSubwayPuzzle, Error, TEXT("Failed to spawn the puzzle camera; the view target will fall back to the pawn."));
    }

    Super::StartPlay();
}
