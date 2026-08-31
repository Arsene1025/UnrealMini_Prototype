#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "PuzzleGameMode.generated.h"

class ADirectionalLight;
class APuzzleBoard;

UCLASS()
class SPP_API APuzzleGameMode : public AGameModeBase
{
    GENERATED_BODY()

public:
    APuzzleGameMode();
    virtual void StartPlay() override;

private:
    ADirectionalLight* SpawnRigLight(
        const FVector& Location,
        const FRotator& Rotation,
        float Intensity,
        const FLinearColor& Color,
        bool bCastShadows,
        int32 ForwardShadingPriority);
    void BuildLightingRig(const FVector& FocusPoint);

    UPROPERTY(Transient)
    TObjectPtr<APuzzleBoard> SpawnedBoard;
};
