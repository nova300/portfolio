#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "PhotoGradingLibrary.h"
#include "ViewCapture.generated.h"

struct ViewCaptureCommand
{
    float photoSubjectscore;
    int cameraUsed;
};

UCLASS()
class UNREALPROJECT_GP3T6_API AViewCapture : public AActor
{
    GENERATED_BODY()

public:

    // Sets default values for this actor's properties
    AViewCapture(const FObjectInitializer& ObjectInitializer);

    UFUNCTION(BlueprintCallable, Category = "View Capture")
        void CapturePlayersView(TArray<FColor>& ColorData, int32& outputWidth, int32& outputHeight);

    virtual void BeginPlay() override;

    void UpdatePixelArray();

    UPROPERTY()
        TArray<FColor> pixelArray;

    UFUNCTION(BlueprintImplementableEvent, Category = ViewCapture)
        void ScreenshotReady(const FString& filename, const int& cameraUsed, const float& photoSubjectScore);

    UFUNCTION(BlueprintCallable)
        void RequestViewCapture(int cameraUsed, float photoSubjectScore);

private:
    UTexture2D* previewTexture;
    FDelegateHandle DelegateHandle;
    std::vector<ViewCaptureCommand> commandQueue;
};