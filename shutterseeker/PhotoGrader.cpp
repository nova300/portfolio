// Fill out your copyright notice in the Description page of Project Settings.


#include "PhotoGrader.h"

#include <filesystem>

#include "Kismet/KismetSystemLibrary.h"
#include "UnrealProject_GP3T6/PhotoGradingLibrary.h"

// Sets default values
APhotoGrader::APhotoGrader()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

}

// Called when the game starts or when spawned
void APhotoGrader::BeginPlay()
{
	Super::BeginPlay();
	
}

void APhotoGrader::AddGradingJob(FString InPhotoFilePath)
{
    int CameraUsed = 1;
    if(InPhotoFilePath.Contains("camera2"))
    {
        CameraUsed = 2;
    }
    if(InPhotoFilePath.Contains("camera3"))
    {
        CameraUsed = 3;
    }
    std::string InFilepath = std::string(TCHAR_TO_UTF8(*InPhotoFilePath));

    cv::Mat InImage;
    if (std::filesystem::exists(InFilepath)) // check if file exists before trying to read it 
    {
        GradingQueued.Broadcast();
        GradingQueue.Add(InPhotoFilePath);
        
        if(GradingInProgress)
        {
            return;
        }

        GradingInProgress = true;
        InImage = cv::imread(InFilepath, cv::IMREAD_ANYCOLOR);
        AsyncTask(ENamedThreads::AnyHiPriThreadNormalTask, [this, CameraUsed, InImage, InPhotoFilePath]()
        {
            int BestMSEIndex;
            int BestFMIndex;
            double BestMSE = 9999;
            double BestFM = 9999;
            
        for (int i = 0; i < ComparisonPhotosVector.size(); i++)
        {
            auto a = ComparisonPhotosVector[i];
            if (CameraUsed != a.CameraUsed)
            {
                continue; // only compare with same camera
            }
            cv::Mat image1;
            cv::Mat image2;
            UPhotoGradingLibrary::DownscaleComparison(InImage, a.MatData, image1, image2);
            
            bool outputDebug = true;
#if UE_BUILD_SHIPPING
        outputDebug = false; // dont output debug images when in shipping build
#endif
            
            double ThisMSE = UPhotoGradingLibrary::CalculateMSE(image1, image2);
            if(GetWorld() == nullptr)
            {
                UE_LOG(LogTemp, Error, TEXT("[AsyncGrading] Seems like the game is quitting, cancelling async (after MSE)"));
                GradingInProgress = false;
                return;
            }

            double ThisFM = UPhotoGradingLibrary::CalculateFeatureMatching(image1, image2, outputDebug);
            if(GetWorld() == nullptr)
            {
                UE_LOG(LogTemp, Error, TEXT("[AsyncGrading] Seems like the game is quitting, cancelling async (after FM)"));
                GradingInProgress = false;
                return;
            }
            
            if (ThisMSE < BestMSE)
            {
                BestMSE = ThisMSE;
                BestMSEIndex = i;
            }
            if(ThisFM < BestFM)
            {
                BestFM = ThisFM;
                BestFMIndex = i;
            }
        }

        AsyncTask(ENamedThreads::GameThread, [this, InPhotoFilePath, BestMSEIndex, BestMSE, BestFMIndex, BestFM]()
        {
            UE_LOG(LogTemp, Display, TEXT("[AsyncGrading] async stuff is done"));

            // cancel if game is exiting
            
            if(GetWorld() == nullptr)
            {
                UE_LOG(LogTemp, Error, TEXT("[AsyncGrading] Seems like the game is quitting, cancelling async (GameThread)"));
                GradingInProgress = false;
                return;
            }

            // add to cache
            FCacheEntry NewCacheEntry;
            NewCacheEntry.FilePath = InPhotoFilePath;
            NewCacheEntry.IndexMSE = BestMSEIndex;
            NewCacheEntry.ValueMSE = BestMSE;
            NewCacheEntry.IndexFM = BestFMIndex;
            NewCacheEntry.ValueFM = BestFM;
            
            CachedRatings.push_back(NewCacheEntry);
            UE_LOG(LogTemp, Display, TEXT("[AsyncGrading] Added to cache"));
            
            GradingQueue.Remove(InPhotoFilePath);
            GradingComplete.Broadcast();
            
            GradingInProgress = false;
            if(GradingQueue.Num() > 0)
            {
                UE_LOG(LogTemp, Display, TEXT("[AsyncGrading] There are %i photos queued"), GradingQueue.Num());
                AddGradingJob(GradingQueue[0]);
            } else
            {
                UE_LOG(LogTemp, Display, TEXT("[AsyncGrading] No more jobs in queue"));
            }

            
        });
    });
        
    }
    
}

int APhotoGrader::GradingJobsQueued()
{
    return GradingQueue.Num();
}

bool APhotoGrader::PhotoIsBeingGraded(FString PhotoFilePath)
{
    return GradingQueue.Contains(PhotoFilePath);
}

void APhotoGrader::FindBestMatchMSE(FString PhotoFilePath, int Width, bool& Success, double& MSEValue, int CameraUsed, int& ComparisonIndexMSE, double& FMValue, int& ComparisonIndexFM)
{
    Success = true; // optimistic arent we
    
    // check if rating is cached and if so use it
    int CachedIndex = PhotoRatingCacheIndex(PhotoFilePath);
    if(CachedIndex != -1)
    {
        auto CachedEntry = CachedRatings[CachedIndex];
        MSEValue = CachedEntry.ValueMSE;
        ComparisonIndexMSE = CachedEntry.IndexMSE;
        FMValue = CachedEntry.ValueFM;
        ComparisonIndexFM = CachedEntry.IndexFM;
        UE_LOG(LogTemp, Log, TEXT("[FindBestMatchMSE] Using cached rating"));
        return;
    }
    UE_LOG(LogTemp, Log, TEXT("[FindBestMatchMSE] Photo rating is not cached"));
}

void APhotoGrader::ParseRatingsExcel(const FString FilePathIn)     
{
    std::string fp = std::string(TCHAR_TO_UTF8(*FilePathIn));
    std::ifstream infile(fp);

    if(!infile.is_open())
    {
        UE_LOG(LogTemp, Error, TEXT("Couldnt open %s"), *FilePathIn);
        return; // failed to open file
    }

    std::string line;
    bool FirstLineRead = false;
    while (std::getline(infile,line))
    {
        if(!FirstLineRead) // ignore the top line (with column names)
        {
            FirstLineRead = true;
            continue;
        }
        
        std::vector<std::string> row;
        std::istringstream iss(line);
        std::string token;
        while(std::getline(iss,token,'\t'))
        {
            row.push_back(token);
        }

        // FileName-Camera-Category-Rating-Comment
        auto filename = row[0];

        // UE_LOG(LogTemp, Error, TEXT("LUL: %s"), UTF8_TO_TCHAR(row[1].c_str()));
        
        auto camera = std::stoi(row[1]);
        auto categories = row[2];
        auto rating = std::stoi(row[3]);
        //auto comment = row[4]; // dont need this


        auto fullpath = UKismetSystemLibrary::GetProjectDirectory();
        fullpath += "ComparisonPhotos/";
        fullpath += UTF8_TO_TCHAR(filename.c_str());

        if (!FPaths::FileExists(fullpath))
        {
            UE_LOG(LogTemp, Error, TEXT("File doesnt exist, skipping %s"), *fullpath);
            continue;
        }
        
        FComparisonPhoto NewEntry;
        NewEntry.MatData = UPhotoGradingLibrary::CVMatFromImageFile(fullpath);
        NewEntry.FileName = UTF8_TO_TCHAR(filename.c_str());
        NewEntry.FullPath = fullpath;
        NewEntry.CameraUsed = camera;

        // split categories by ,
        TArray<FString> Categories;
        std::istringstream iss_categories(categories);
        std::string token_category;
        while(std::getline(iss_categories,token_category,','))
        {
            FString ThisCategory = FString::Printf(TEXT("%s"), UTF8_TO_TCHAR(token_category.c_str()));
            Categories.Add(ThisCategory);
            UE_LOG(LogTemp, Display, TEXT("Category parsed: %s"), *ThisCategory);
        }

        NewEntry.Categories = Categories;
        NewEntry.Rating = rating;
        ComparisonPhotosVector.push_back(NewEntry);
    }

    infile.close();
}

int APhotoGrader::HowManyInComparisonVector()
{
    return ComparisonPhotosVector.size();
}

void APhotoGrader::GetInfoAboutComparison(int index, FString& FullPath, FString& FileName, float& Rating,
    TArray<FString>& Categories)
{
    auto RequestedComparison = ComparisonPhotosVector[index];
    FullPath = RequestedComparison.FullPath;
    FileName = RequestedComparison.FileName;
    Rating = RequestedComparison.Rating;
    Categories = RequestedComparison.Categories;
    UE_LOG(LogTemp, Display, TEXT("Categories array length: %i"), Categories.Num());
}

bool APhotoGrader::PhotoRatingIsCached(FString PhotoFilePath)
{
    for(auto & a : CachedRatings)
    {
        if(a.FilePath == PhotoFilePath)
        {
            return true;
        }
    }
    return false;
}

int APhotoGrader::PhotoRatingCacheIndex(FString PhotoFilePath)
{
    for(int i = 0; i < CachedRatings.size(); i++)
    {
        if(CachedRatings[i].FilePath == PhotoFilePath)
        {
            return i;
        }
    }
    return -1;
}

FCacheEntry APhotoGrader::GetCachedRating(FString PhotoFilePath)
{
    // hopefully you check before running this if the entry exists
    return CachedRatings[PhotoRatingCacheIndex(PhotoFilePath)];
}
