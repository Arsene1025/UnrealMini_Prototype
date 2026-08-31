#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "PuzzleHUD.generated.h"

UCLASS()
class SPP_API APuzzleHUD : public AHUD
{
    GENERATED_BODY()

public:
    virtual void DrawHUD() override;
};
