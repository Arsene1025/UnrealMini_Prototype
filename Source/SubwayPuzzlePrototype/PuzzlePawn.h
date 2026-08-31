#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "PuzzlePawn.generated.h"

class APuzzleBoard;
class UMaterialInstanceDynamic;
class USceneComponent;
class UStaticMeshComponent;

UCLASS()
class SPP_API APuzzlePawn : public APawn
{
    GENERATED_BODY()

public:
    APuzzlePawn();

    virtual void Tick(float DeltaSeconds) override;

    void RequestDirectionalStep(const FVector2D& Direction);
    void RequestPathToNode(int32 GoalNode);

    int32 GetCurrentNode() const { return CurrentNode; }
    bool IsFollowingPath() const { return PendingPath.Num() > 0; }

protected:
    virtual void BeginPlay() override;

private:
    bool EnsureBoard();
    void ReportFeedback(const FString& Message, const FLinearColor& Color) const;

    UPROPERTY(VisibleAnywhere)
    TObjectPtr<USceneComponent> SceneRoot;

    UPROPERTY(VisibleAnywhere)
    TObjectPtr<UStaticMeshComponent> BodyMesh;

    UPROPERTY(Transient)
    TObjectPtr<UMaterialInstanceDynamic> BodyMaterial;

    UPROPERTY(Transient)
    TObjectPtr<APuzzleBoard> Board;

    TArray<int32> PendingPath;
    int32 CurrentNode = INDEX_NONE;
    float MoveSpeed = 360.0f;
    float HeightAboveTile = 72.0f;
};
