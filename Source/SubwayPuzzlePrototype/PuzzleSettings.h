#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "PuzzleSettings.generated.h"

class UPuzzleGraphAsset;

/**
 * Project Settings -> Game -> Subway Puzzle.
 *
 * The game mode is a native class set through GlobalDefaultGameMode, so there is no
 * instance to configure in the editor. This is where the stage to load is chosen, and
 * it is the hook a stage-select screen will read from later.
 */
UCLASS(Config = Game, DefaultConfig, meta = (DisplayName = "Subway Puzzle"))
class SPP_API UPuzzleSettings : public UDeveloperSettings
{
    GENERATED_BODY()

public:
    /** Graph loaded when a board is spawned without one. Empty falls back to the built-in test graph. */
    UPROPERTY(Config, EditAnywhere, Category = "Stages")
    TSoftObjectPtr<UPuzzleGraphAsset> DefaultGraphAsset;

    /**
     * Draw the greybox platform, tiles, connectors and node numbers.
     *
     * Turn this off once real station geometry exists. Node tiles are still created and
     * still block the Visibility channel -- they just stop rendering -- so click-to-move
     * keeps working against invisible targets.
     */
    UPROPERTY(Config, EditAnywhere, Category = "Debug")
    bool bShowGreyboxVisuals = true;

    virtual FName GetCategoryName() const override { return TEXT("Game"); }

    static const UPuzzleSettings* Get() { return GetDefault<UPuzzleSettings>(); }
};
