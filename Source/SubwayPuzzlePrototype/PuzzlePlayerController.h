#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "PuzzlePlayerController.generated.h"

class APuzzleBoard;
class APuzzlePawn;
class UInputAction;
class UInputMappingContext;

UCLASS()
class SPP_API APuzzlePlayerController : public APlayerController
{
    GENERATED_BODY()

public:
    APuzzlePlayerController();

    void ShowFeedback(const FString& Message, const FLinearColor& Color);
    const FString& GetFeedbackText() const { return FeedbackText; }
    const FLinearColor& GetFeedbackColor() const { return FeedbackColor; }

protected:
    virtual void BeginPlay() override;
    virtual void SetupInputComponent() override;

private:
    void CreateInputObjects();
    UInputAction* CreateBooleanAction(const FName& Name);
    APuzzlePawn* GetPuzzlePawn() const;
    APuzzleBoard* GetPuzzleBoard() const;

    void MoveNorth();
    void MoveSouth();
    void MoveWest();
    void MoveEast();
    void ClickDestination();

    UPROPERTY(Transient)
    TObjectPtr<UInputMappingContext> MappingContext;

    UPROPERTY(Transient)
    TObjectPtr<UInputAction> MoveNorthAction;

    UPROPERTY(Transient)
    TObjectPtr<UInputAction> MoveSouthAction;

    UPROPERTY(Transient)
    TObjectPtr<UInputAction> MoveWestAction;

    UPROPERTY(Transient)
    TObjectPtr<UInputAction> MoveEastAction;

    UPROPERTY(Transient)
    TObjectPtr<UInputAction> ClickAction;

    FString FeedbackText;
    FLinearColor FeedbackColor = FLinearColor::White;
};
