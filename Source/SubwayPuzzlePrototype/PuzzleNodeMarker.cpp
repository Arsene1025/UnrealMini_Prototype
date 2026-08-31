#include "PuzzleNodeMarker.h"

#include "Components/StaticMeshComponent.h"
#include "UObject/ConstructorHelpers.h"

APuzzleNodeMarker::APuzzleNodeMarker()
{
    PrimaryActorTick.bCanEverTick = false;

    MarkerMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MarkerMesh"));
    SetRootComponent(MarkerMesh);
    MarkerMesh->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    MarkerMesh->SetCollisionResponseToAllChannels(ECR_Ignore);
    MarkerMesh->SetRelativeScale3D(FVector(0.72, 0.72, 0.12));

    static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeFinder(TEXT("/Engine/BasicShapes/Cube.Cube"));
    if (CubeFinder.Succeeded())
    {
        MarkerMesh->SetStaticMesh(CubeFinder.Object);
    }

    static ConstructorHelpers::FObjectFinder<UMaterialInterface> MaterialFinder(TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
    if (MaterialFinder.Succeeded())
    {
        MarkerMesh->SetMaterial(0, MaterialFinder.Object);
    }
}
