#pragma once

#include "PreOpenCVHeaders.h"
#include "opencv2/opencv.hpp"
#include "opencv2/core.hpp"
#include "opencv2/calib3d.hpp"
#include "opencv2/imgproc.hpp"
#include "opencv2/core/mat.hpp"
#include <opencv2/features2d/features2d.hpp>
#include <opencv2/highgui/highgui.hpp>
#include "PostOpenCVHeaders.h"
#include <iostream>
#include <fstream>
#include <vector>
#include <cstring>


#include "Misc/Paths.h" 

#include <sys/stat.h>
#include <io.h>

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Kismet/KismetRenderingLibrary.h"
#include "Engine/TextureRenderTarget2D.h"
#include "PhotoGradingLibrary.generated.h"

struct Metadata_datablock
{
	int score;
	int day;
	float time;
	int type;
};

struct Metadata 
{
	unsigned int chunkLen;
	char blockName[4];
	Metadata_datablock datablock;
	unsigned int crc;
};

struct Metadata_cacheEntry
{
	char filename[128];
	Metadata_datablock datablock;
};

struct imageCacheEntry
{
	FString filepath;
	UTexture2D* texture;
};


UCLASS()
class UNREALPROJECT_GP3T6_API UPhotoGradingLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:

	
	UFUNCTION(BlueprintCallable)
	static float GetSharpness(const TArray<FColor>& InPixels);




	static cv::Mat BuildImageFromPixels(const TArray<FColor>& InPixels, int32 Width);

	static cv::Mat BuildImageBasedOnCameraUsed(const TArray<FColor>& InPixels, int32 InWidth, int32 CameraUsed);

	UFUNCTION(BlueprintCallable)
	static void SaveImageToFile(const FString FilePath, const TArray<FColor>& InPixels, const int32 Width, const int32 CameraUsed, bool writeMeta = false, int score = 0, int day = 0, float time = 0.0);

	UFUNCTION(BlueprintPure)
	static UTexture2D* ImageFileToTexture(const FString FilePath);

	UFUNCTION(BlueprintPure)
	static UTexture2D* PixelArrayToTexture(const TArray<FColor>& InPixels, int32 Width);

	/* Returns list of files in a folder. If sort by modification date is enabled, the newer files be first (newest file == index 0) */
	UFUNCTION(BlueprintCallable)
	static void FilesInDirectory(const bool SortByModificationDate, const FString& DirPath, TArray<FString>& FileNames);
	
	static cv::Mat CVMatFromImageFile(const FString FilePath);
	
	static double CalculateMSE(const cv::Mat image1, const cv::Mat image2);

	static double CalculateSSIM(const cv::Mat image1, const cv::Mat image2);

	static double CalculateFeatureMatching(const cv::Mat image1, const cv::Mat image2, bool outputDebugImg = false);
	static double CalculateFMExperimental(const cv::Mat image1, const cv::Mat image2, bool outputDebugImg = false);

	static void DownscaleComparison(const cv::Mat in1, const cv::Mat in2, cv::Mat& out1, cv::Mat& out2);

	UFUNCTION(BlueprintCallable)
	static void GetMetadataFromFile(const FString FilePath, int& score, int& day, float& time, int& type);

	/* Returns the points part from a comparison photo filename (eg. photo-100.png will make this return 100) */
    // UFUNCTION(BlueprintPure, Category="Photo Grading")
    // static int GetPointsFromFilename(const FString FileName);

	static unsigned int calculateCRC32(const char* data, int length);

	static unsigned int invertByteOrderInt(unsigned int i);

	static std::vector<Metadata_cacheEntry> metadataCache;
	static std::vector<imageCacheEntry> imageCache;

	static void CachePixelArrayAsTexture(const FString FilePath, const TArray<FColor>& InPixels, int32 Width);
	
};


