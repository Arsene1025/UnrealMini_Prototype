#include "PuzzlePlayerController.h"

#include "SubwayPuzzlePrototype.h"
#include "PuzzleBoard.h"
#include "PuzzlePawn.h"
#include "Camera/CameraActor.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Engine/LocalPlayer.h"
#include "EngineUtils.h"
#include "InputAction.h"
#include "InputCoreTypes.h"
#include "InputMappingContext.h"

APuzzlePlayerController::APuzzlePlayerController()
{
    bShowMouseCursor = true;
    bEnableClickEvents = true;
    bEnableMouseOverEvents = true;
    DefaultMouseCursor = EMouseCursor::Default;
}

void APuzzlePlayerController::BeginPlay()
{
    Super::BeginPlay();

    bool bFoundPuzzleCamera = false;
    for (TActorIterator<ACameraActor> It(GetWorld()); It; ++It)
    {
        if (It->ActorHasTag(TEXT("PuzzleCamera")))
        {
            SetViewTarget(*It);
            bFoundPuzzleCamera = true;
            break;
        }
    }

    if (!bFoundPuzzleCamera)
    {
        // Falling back to the pawn puts the view inside the board, which reads as a
        // black screen. Say so rather than leaving it to guesswork.
        UE_LOG(
            LogSubwayPuzzle,
            Error,
            TEXT("No actor tagged 'PuzzleCamera' was found. APuzzleGameMode::StartPlay must run before ")
            TEXT("the player controller begins play, otherwise the view target stays on the pawn."));
    }

    FInputModeGameAndUI InputMode;
    InputMode.SetHideCursorDuringCapture(false);
    InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
    SetInputMode(InputMode);
}

UInputAction* APuzzlePlayerController::CreateBooleanAction(const FName& Name)
{
    UInputAction* Action = NewObject<UInputAction>(this, Name);
    Action->ValueType = EInputActionValueType::Boolean;
    return Action;
}

void APuzzlePlayerController::CreateInputObjects()
{
    if (MappingContext)
    {
        return;
    }

    MappingContext = NewObject<UInputMappingContext>(this, TEXT("CxxOnlyMappingContext"));
    MoveNorthAction = CreateBooleanAction(TEXT("MoveNorth"));
    MoveSouthAction = CreateBooleanAction(TEXT("MoveSouth"));
    MoveWestAction = CreateBooleanAction(TEXT("MoveWest"));
    MoveEastAction = CreateBooleanAction(TEXT("MoveEast"));
    ClickAction = CreateBooleanAction(TEXT("ClickDestination"));

    MappingContext->MapKey(MoveNorthAction, EKeys::W);
    MappingContext->MapKey(MoveNorthAction, EKeys::Up);
    MappingContext->MapKey(MoveSouthAction, EKeys::S);
    MappingContext->MapKey(MoveSouthAction, EKeys::Down);
    MappingContext->MapKey(MoveWestAction, EKeys::A);
    MappingContext->MapKey(MoveWestAction, EKeys::Left);
    MappingContext->MapKey(MoveEastAction, EKeys::D);
    MappingContext->MapKey(MoveEastAction, EKeys::Right);
    MappingContext->MapKey(ClickAction, EKeys::LeftMouseButton);
}

void APuzzlePlayerController::SetupInputComponent()
{
    Super::SetupInputComponent();
    CreateInputObjects();

    if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer()))
    {
        Subsystem->AddMappingContext(MappingContext, 0);
    }

    if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(InputComponent))
    {
        EnhancedInputComponent->BindAction(MoveNorthAction, ETriggerEvent::Started, this, &APuzzlePlayerController::MoveNorth);
        EnhancedInputComponent->BindAction(MoveSouthAction, ETriggerEvent::Started, this, &APuzzlePlayerController::MoveSouth);
        EnhancedInputComponent->BindAction(MoveWestAction, ETriggerEvent::Started, this, &APuzzlePlayerController::MoveWest);
        EnhancedInputComponent->BindAction(MoveEastAction, ETriggerEvent::Started, this, &APuzzlePlayerController::MoveEast);
        EnhancedInputComponent->BindAction(ClickAction, ETriggerEvent::Started, this, &APuzzlePlayerController::ClickDestination);
    }
}

APuzzlePawn* APuzzlePlayerController::GetPuzzlePawn() const
{
    return Cast<APuzzlePawn>(GetPawn());
}

APuzzleBoard* APuzzlePlayerController::GetPuzzleBoard() const
{
    for (TActorIterator<APuzzleBoard> It(GetWorld()); It; ++It)
    {
        return *It;
    }
    return nullptr;
}

void APuzzlePlayerController::MoveNorth()
{
    if (APuzzlePawn* PuzzlePawn = GetPuzzlePawn())
    {
        PuzzlePawn->RequestDirectionalStep(FVector2D(0.0, 1.0));
    }
}

void APuzzlePlayerController::MoveSouth()
{
    if (APuzzlePawn* PuzzlePawn = GetPuzzlePawn())
    {
        PuzzlePawn->RequestDirectionalStep(FVector2D(0.0, -1.0));
    }
}

void APuzzlePlayerController::MoveWest()
{
    if (APuzzlePawn* PuzzlePawn = GetPuzzlePawn())
    {
        PuzzlePawn->RequestDirectionalStep(FVector2D(-1.0, 0.0));
    }
}

void APuzzlePlayerController::MoveEast()
{
    if (APuzzlePawn* PuzzlePawn = GetPuzzlePawn())
    {
        PuzzlePawn->RequestDirectionalStep(FVector2D(1.0, 0.0));
    }
}

void APuzzlePlayerController::ClickDestination()
{
    APuzzleBoard* Board = GetPuzzleBoard();
    APuzzlePawn* PuzzlePawn = GetPuzzlePawn();
    if (!Board || !PuzzlePawn)
    {
        ShowFeedback(TEXT("Board or player is not ready."), FLinearColor::Red);
        return;
    }

    FHitResult Hit;
    int32 NodeIndex = INDEX_NONE;
    if (GetHitResultUnderCursor(ECC_Visibility, true, Hit) && Board->ResolveNodeFromHit(Hit, NodeIndex))
    {
        PuzzlePawn->RequestPathToNode(NodeIndex);
        return;
    }

    ShowFeedback(TEXT("Click directly on a numbered blue or green tile."), FLinearColor::Red);
}

void APuzzlePlayerController::ShowFeedback(const FString& Message, const FLinearColor& Color)
{
    FeedbackText = Message;
    FeedbackColor = Color;
}
