#include "ViewCapture.h"
#include "UnrealClient.h"

// Sets default values
AViewCapture::AViewCapture(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
    PrimaryActorTick.bCanEverTick = false;

}

void AViewCapture::CapturePlayersView(TArray<FColor>& ColorData, int32& outputWidth, int32& outputHeight)
{
    ColorData = pixelArray; // copy array that was taken in draw

    outputWidth = GEngine->GameViewport->Viewport->GetSizeXY().X;
    outputHeight = GEngine->GameViewport->Viewport->GetSizeXY().Y;
}

void AViewCapture::BeginPlay()
{
    Super::BeginPlay();
}

void AViewCapture::UpdatePixelArray()
{
    GEngine->GameViewport->OnDrawn().Remove(DelegateHandle);
    TArray<FColor> colorArray;
    GEngine->GameViewport->Viewport->ReadPixels(colorArray);
    int width = GEngine->GameViewport->Viewport->GetSizeXY().X;

    if (commandQueue.size() < 1) return;
    ViewCaptureCommand command = commandQueue.back();
    int cameraUsed = command.cameraUsed;
    float photoSubjectScore = command.photoSubjectscore;
    commandQueue.pop_back();



    AsyncTask(ENamedThreads::AnyHiPriThreadHiPriTask, [this, cameraUsed, photoSubjectScore, width, colorArray]()
        {
            FString dir = FPaths::ProjectSavedDir();
            FGuid guid = FGuid::NewGuid();
            std::string g = std::string(TCHAR_TO_UTF8(*(guid.ToString())));
            std::string fn = std::string(TCHAR_TO_UTF8(*dir));
            fn.append("photo_");
            fn.append("camera");
            fn.append(std::to_string(cameraUsed));
            fn.append("_");
            fn.append(g);
            fn.append(".png");
            FString filePath(fn.c_str());

            UPhotoGradingLibrary::SaveImageToFile(filePath, colorArray, width, cameraUsed, false);



            AsyncTask(ENamedThreads::GameThread, [this, filePath, cameraUsed, photoSubjectScore]()
                {
                    UE_LOG(LogTemp, Display, TEXT("[AsyncSaving] async stuff is done"));

                    if (GetWorld() == nullptr)
                    {
                        UE_LOG(LogTemp, Error, TEXT("[AsyncSaving] Seems like the game is quitting, cancelling async"));
                        return;
                    }

                    ScreenshotReady(filePath, cameraUsed, photoSubjectScore);
                });
        });

}

void AViewCapture::RequestViewCapture(int cameraUsed, float photoSubjectScore)
{
    ViewCaptureCommand command;
    command.cameraUsed = cameraUsed;
    command.photoSubjectscore = photoSubjectScore;
    commandQueue.push_back(command);
    DelegateHandle = GEngine->GameViewport->OnDrawn().AddUObject(this, &AViewCapture::UpdatePixelArray);
}

