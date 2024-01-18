// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"

#include "PreOpenCVHeaders.h"
#include "opencv2/core/mat.hpp"
#include "PostOpenCVHeaders.h"

#include "PhotoGrader.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FPhotoGradingQueued);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FPhotoGradingComplete);

struct FComparisonPhoto
{
	
    cv::Mat MatData;
    FString FileName;
	FString FullPath;
	int CameraUsed;
	float Rating;
	TArray<FString> Categories;
    FComparisonPhoto() {}

    FComparisonPhoto(cv::Mat InMat, FString InFileName, FString InFullPath, int InCameraUsed, float InRating, TArray<FString> InCategories)
    {
        FileName = InFileName;
    	FullPath = InFullPath;
        MatData = InMat.clone();
    	CameraUsed = InCameraUsed;
    	Rating = InRating;
    	Categories = InCategories;
    }
};

struct FCacheEntry
{
	FString FilePath;
	int IndexMSE;
	float ValueMSE;
	int IndexFM;
	int ValueFM;
	// can add more ratings here
	// float RatingSSIM;

	FCacheEntry(){}

	FCacheEntry(FString InFilePath, int InIndexMSE, float InRatingMSE, int InIndexFM, float InRatingFM)
	{
		FilePath = InFilePath;
		IndexMSE = InIndexMSE;
		ValueMSE = InRatingMSE;
		IndexFM = InIndexFM;
		ValueFM = InRatingFM;
	}
};

UCLASS()
class UNREALPROJECT_GP3T6_API APhotoGrader : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	APhotoGrader();

	UPROPERTY(BlueprintAssignable)
	FPhotoGradingComplete GradingComplete;

	UPROPERTY(BlueprintAssignable)
	FPhotoGradingQueued GradingQueued;

	std::vector<FComparisonPhoto> ComparisonPhotosVector; // std::vector because TArray didnt like custom structs
	std::vector<FCacheEntry> CachedRatings;
	
	/* Finds best match using MSE (lower score = more similar) */
    UFUNCTION(BlueprintCallable, Category="Photo Grading")
    void FindBestMatchMSE(FString PhotoFilePath, int Width, bool& Success, double& MSEValue, int CameraUsed, int& ComparisonIndexMSE, double& FMValue, int& ComparisonIndexFM);

	/* Parses the tsv file that contains photo rating info */
	UFUNCTION(BlueprintCallable, Category="Photo Grading")
	void ParseRatingsExcel(const FString FilePathIn);

	/* Get how many entries are in comparison database */
	UFUNCTION(BlueprintPure, Category="Photo Grading")
	int HowManyInComparisonVector();

	/* Get info from comparison vector by index */
	UFUNCTION(BlueprintPure, Category="Photo Grading")
	void GetInfoAboutComparison(int index, FString& FullPath, FString& FileName, float& Rating, TArray<FString>& Categories);

	/* Returns true if photo is cached */
	UFUNCTION(BlueprintPure, Category="Photo Grading")
	bool PhotoRatingIsCached(FString PhotoFilePath);
	
	/* Returns index of cached photo rating. Returns -1 if it isnt cached. */
	UFUNCTION(BlueprintPure, Category="Photo Grading")
	int PhotoRatingCacheIndex(FString PhotoFilePath);

	/* Gets cached rating entry. It assumes the entry exists so you should probably check yourself beforehand. */
	FCacheEntry GetCachedRating(FString PhotoFilePath);

	

	/* Adds a photo to the grading queue (async, wow) */
	UFUNCTION(BlueprintCallable, Category="Photo Grading")
	void AddGradingJob(FString InPhotoFilePath);

	/* Returns how many photos are in the queue of being graded */
	UFUNCTION(BlueprintPure, Category="Photo Grading")
	int GradingJobsQueued();

	/* Returns true if this photo is currently being graded */
	UFUNCTION(BlueprintPure, Category="Photo Grading")
	bool PhotoIsBeingGraded(FString PhotoFilePath);

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;
	
	TArray<FString> GradingQueue;
	bool GradingInProgress = false;
};
