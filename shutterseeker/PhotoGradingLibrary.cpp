#include "PhotoGradingLibrary.h"

#include <filesystem>


std::vector<Metadata_cacheEntry> UPhotoGradingLibrary::metadataCache;
std::vector<imageCacheEntry> UPhotoGradingLibrary::imageCache;



float UPhotoGradingLibrary::GetSharpness(const TArray<FColor>& InPixels)
{

	
	auto Image = BuildImageFromPixels(InPixels, 256);

	// convert to grayscale
	cv::Mat GrayImage;
	cv::cvtColor(Image, GrayImage, cv::COLOR_RGBA2GRAY);
	
	cv::Mat LaplacianImage;
	cv::Laplacian(GrayImage, LaplacianImage, CV_64F);
	
	cv::Scalar Mean, StdDev;
	cv::meanStdDev(LaplacianImage, Mean,StdDev);
	double Variance = StdDev.val[0] * StdDev.val[0];
	
	return Variance;
}



cv::Mat UPhotoGradingLibrary::BuildImageFromPixels(const TArray<FColor>& InPixels, int32 Width)
{
	int Height = InPixels.Num() / Width;
	UE_LOG(LogTemp, Display, TEXT("BuildImageFromPixels: input size is %i %i"), Width, Height);
	cv::Mat Image(Height, Width, CV_8UC3);
	uint8* Data = Image.data;
	for (int i = 0; i < InPixels.Num(); i++)
	{
		*Data++ = InPixels[i].B; // OpenCV uses BGR
		*Data++ = InPixels[i].G;
		*Data++ = InPixels[i].R;
	}
	
	UE_LOG(LogTemp, Display, TEXT("BuildImageFromPixels: constructed image is %i %i"), Image.cols, Image.rows);
	return Image;
}

cv::Mat UPhotoGradingLibrary::BuildImageBasedOnCameraUsed(const TArray<FColor>& InPixels, int32 InWidth, int32 CameraUsed)
{
	int InHeight = InPixels.Num() / InWidth;
	float InAspect = InWidth / (InHeight * 1.0); // * 1.0 to get a proper float that isn't rounded (yes, very cursed)
	
	UE_LOG(LogTemp, Display, TEXT("[BuildImageBasedOnCameraUsed] input is %ix%i (AR: %f),  camera is %i"), InWidth, InHeight, InAspect, CameraUsed);
	cv::Mat Raw = BuildImageFromPixels(InPixels, InWidth);
	
	float CalcWidth = InWidth;
	float CalcHeight = InHeight;

	if( InAspect < (16.0 / 9.0) ) // 4:3, 5:4, 16:10 etc
	{
		if(CameraUsed == 1 && InAspect == (4.0/3.0) ) // for the one person using a 4:3 resolution in 2023
		{
			// dont need to do anything
		}
		else if (CameraUsed == 1)
		{
			// theres probably a smarter way to do this
			if(InAspect < (4.0/3.0)) // 5:4 for instance (like 1280x1024 => 1280x960)
			{
				CalcHeight = InWidth / (4.0 / 3.0);
			} else // 16:10 for instance (like 2560x1600 => 2133x1600)
			{
				CalcWidth = (4.0 / 3.0) * InHeight;
			}
		}
		else
		{
			CalcHeight = InWidth / (16.0 / 9.0);
		}
	}
	else // everything else ==> we might need to crop left and right
	{
		if (CameraUsed == 1)	
		{
			CalcWidth = InHeight * (4.0 / 3.0);
		}
		else if (InAspect > (16.0 / 9.0) ) // ultrawides, 21:9, etc.
		{
			CalcWidth = InHeight * (16.0 / 9.0);
		}
	}

	// assert we never get larger than input (should 100% prevent crashes)
	CalcHeight = FMath::Min(InHeight, CalcHeight);
	CalcWidth = FMath::Min(InWidth, CalcWidth);
	
	int TargetWidth = floor(CalcWidth);
	int TargetHeight = floor(CalcHeight);
	UE_LOG(LogTemp, Display, TEXT("[BuildImageBasedOnCameraUsed] Crop: crop should be %ix%i"), TargetWidth, TargetHeight);
	
	cv::Mat OriginalNoBlackBars;
	if(InWidth != TargetWidth || InHeight != TargetHeight)
	{
		int CropLeft = floor((InWidth - TargetWidth) / 2.0);
		int CropTop = floor((InHeight - TargetHeight) / 2.0);
		cv::Rect Crop(CropLeft, CropTop, TargetWidth, TargetHeight);
		OriginalNoBlackBars = Raw(Crop);
		UE_LOG(LogTemp, Display, TEXT("[BuildImageBasedOnCameraUsed] Crop: Cropped to %ix%i"), OriginalNoBlackBars.cols, OriginalNoBlackBars.rows);
	} else {
		UE_LOG(LogTemp, Display, TEXT("[BuildImageBasedOnCameraUsed] Crop: didnt need to crop!"));
		OriginalNoBlackBars = Raw;
	}

	if(CameraUsed==1)
	{
		// camera1 is 480x360 and 4:3 (it should already be 4:3 when we get here so dont need to worry about that here)
		if(OriginalNoBlackBars.rows <= 360)
		{
			return OriginalNoBlackBars;
		}
		cv::Mat resized;
		cv::resize(OriginalNoBlackBars, resized, cv::Size(480,360));
		UE_LOG(LogTemp, Display, TEXT("[BuildImageBasedOnCameraUsed] Returning %i x %i"), resized.cols, resized.rows);
		return resized;
	}
	else if (CameraUsed==2)
	{
		//camera2 is 16:9 and max 1280x720
		if (OriginalNoBlackBars.rows <= 720)
		{
			return OriginalNoBlackBars;
		}
		cv::Mat resized;
		cv::resize(OriginalNoBlackBars, resized, cv::Size(1280,720));
		UE_LOG(LogTemp, Display, TEXT("[BuildImageBasedOnCameraUsed] Returning %i x %i"), resized.cols, resized.rows);
		return resized;
	}
	else
	{
		//camera3 is 16:9 and max 1920x1080
		if(OriginalNoBlackBars.rows <= 1080) 
		{
			return OriginalNoBlackBars;
		}
		cv::Mat resized;
		cv::resize(OriginalNoBlackBars, resized, cv::Size(1920,1080));
		UE_LOG(LogTemp, Display, TEXT("[BuildImageBasedOnCameraUsed] Returning %i x %i"), resized.cols, resized.rows);
		return resized;
	}
}

void UPhotoGradingLibrary::SaveImageToFile(const FString FilePath, const TArray<FColor>& InPixels, const int32 Width, const int32 CameraUsed, bool writeMeta , int score, int day, float time)
{
	std::string fp = std::string(TCHAR_TO_UTF8(*FilePath));

	
	
	if (!writeMeta)
	{
		cv::Mat img = BuildImageBasedOnCameraUsed(InPixels, Width, CameraUsed);
		cv::imwrite(fp, img);

		cv::cvtColor(img, img, cv::COLOR_BGR2RGB);
		int size = img.rows * img.cols;
		TArray<FColor> pixels;
		pixels.Init(FColor(0, 0, 0), size);
		for (int y = 0; y < img.rows; ++y)
		{
			for (int x = 0; x < img.cols; ++x)
			{
				cv::Vec3b pixel = img.at<cv::Vec3b>(y, x);
				pixels[y * img.cols + x] = FColor(pixel[0], pixel[1], pixel[2]);
			}
		}

		int outWidth = img.cols;

		AsyncTask(ENamedThreads::GameThread, [pixels, FilePath, outWidth]()
        {
            UPhotoGradingLibrary::CachePixelArrayAsTexture(FilePath, pixels, outWidth);
        });

		return;
	}


	std::vector<uchar> membuf;

	std::string ext (".png");
	cv::imencode(ext, BuildImageBasedOnCameraUsed(InPixels, Width, CameraUsed), membuf);

	if (writeMeta)
	{
		Metadata meta{};
		// our png mem chunk definition, currently set to non-critical private and unsafe to copy ,see https://www.w3.org/TR/2003/REC-PNG-20031110/#5Chunk-naming-conventions
		const char* nameDef = "auRa";	
		for (int i = 0; i < 4; i++)
		{
			meta.blockName[i] = nameDef[i];
		}
		meta.datablock.score = score;
		meta.datablock.day = day;
		meta.datablock.time = time;
		meta.datablock.type = CameraUsed;

		meta.chunkLen = sizeof(Metadata_datablock);
		meta.chunkLen = invertByteOrderInt(meta.chunkLen); //png expects big endian byte order :/

		std::vector<char> data(reinterpret_cast<const char*>(&meta.datablock), reinterpret_cast<const char*>(&meta.datablock) + sizeof(Metadata_datablock));
		meta.crc = calculateCRC32(data.data(), sizeof(Metadata_datablock));
		meta.crc = invertByteOrderInt(meta.crc);

		std::vector<char> blockData(reinterpret_cast<const char*>(&meta), reinterpret_cast<const char*>(&meta) + sizeof(Metadata));
		membuf.insert(membuf.end() - 12, blockData.begin(), blockData.end()); // insert our chunk at the end of the file minus 12 bytes to be before IEND
	}

	std::ofstream outputFile(fp, std::ios::binary | std::ios::trunc);

	if (!outputFile) {
		UE_LOG(LogTemp, Error, TEXT("could not save to %s"), *FilePath);
		return;
	}

	outputFile.write((char*)membuf.data(), membuf.size());

	outputFile.close();

	_chmod(fp.c_str(), _S_IREAD);

	UE_LOG(LogTemp, Log, TEXT("Saved to %s"), *FilePath);

}

UTexture2D* UPhotoGradingLibrary::ImageFileToTexture(const FString FilePath)
{

	for (int i = 0; i < imageCache.size(); i++)
	{
		if (imageCache[i].filepath == FilePath)
		{
			UE_LOG(LogTemp, Log, TEXT("ImageFileToTexture: image was in cache, returning early"));
			return imageCache[i].texture;
		}
	}

	std::string fp = std::string(TCHAR_TO_UTF8(*FilePath));
	cv::Mat InImage = cv::Mat::zeros(cv::Size(100,100),CV_8UC1);
	
	if(std::filesystem::exists(fp)) // check if file exists before trying to read it 
	{
		InImage = cv::imread(fp, cv::IMREAD_UNCHANGED);
		cv::cvtColor(InImage, InImage, cv::COLOR_BGR2RGB); // convert BGR to RGB
		UE_LOG(LogTemp, Log, TEXT("ImageFileToTexture: read image of size %i %i"), InImage.cols, InImage.rows);
	}
	else
	{
		// file doesnt exist, return a blank
		UE_LOG(LogTemp, Error, TEXT("ImageFileToTexture: image didnt exist, returning blank"));
		TArray<FColor> pixels;
		pixels.Init(FColor(0,0,0), 8*8);
		return PixelArrayToTexture(pixels,8);
	}

	int size = InImage.rows * InImage.cols;

	TArray<FColor> pixels;
	pixels.Init(FColor(0,0,0), size);
	for(int y = 0; y < InImage.rows; ++y)
	{
		for(int x = 0; x < InImage.cols; ++x)
		{
			cv::Vec3b pixel = InImage.at<cv::Vec3b>(y,x);
			pixels[y*InImage.cols + x] = FColor(pixel[0], pixel[1], pixel[2]);
		}
	}

	return PixelArrayToTexture(pixels, InImage.cols);
	
}

UTexture2D* UPhotoGradingLibrary::PixelArrayToTexture(const TArray<FColor>& InPixels, int32 Width)
{
	int Height = InPixels.Num() / Width;
	UTexture2D* texture = UTexture2D::CreateTransient(Width, Height, PF_B8G8R8A8);

	// https://stackoverflow.com/a/69896620
	void* TextureData = texture->PlatformData->Mips[0].BulkData.Lock(LOCK_READ_WRITE);
	const int32 TextureDataSize = InPixels.Num() * 4;
	FMemory::Memcpy(TextureData, InPixels.GetData(), TextureDataSize);
	texture->PlatformData->Mips[0].BulkData.Unlock();
	texture->UpdateResource();
	
	return texture;
}

void UPhotoGradingLibrary::CachePixelArrayAsTexture(const FString FilePath, const TArray<FColor>& InPixels, int32 Width)
{
	imageCacheEntry cacheEntry;
	cacheEntry.filepath = FilePath;
	cacheEntry.texture = PixelArrayToTexture(InPixels, Width);
	imageCache.push_back(cacheEntry);

	if (imageCache.size() > 10) imageCache.erase(imageCache.begin());
}

void UPhotoGradingLibrary::FilesInDirectory(const bool SortByModificationDate, const FString& DirPath, TArray<FString>& FileNames)
{
	FPlatformFileManager::Get().GetPlatformFile().IterateDirectory(*DirPath, [&FileNames, SortByModificationDate](const TCHAR* FilenameOrDirectory, bool bIsDirectory) -> bool
	{
		if (SortByModificationDate)
		{
			time_t this_mtime = 0;
			time_t comp_mtime = 0;

			if (!bIsDirectory)
			{
				struct stat this_stat;
				if (stat(TCHAR_TO_UTF8(FilenameOrDirectory), &this_stat) == 0) // stat returns 0 if successful
				{
					this_mtime = this_stat.st_mtime;
				}

				int idx = FileNames.Num();
				for (int i = 0; i < FileNames.Num(); i++)
				{
					struct stat comp_stat;
					if (stat(TCHAR_TO_UTF8(*FileNames[i]), &comp_stat) == 0)
					{
						comp_mtime = comp_stat.st_mtime;
					}

					if (this_mtime > comp_mtime)
					{
						idx = i;
						break;
					}
				}

				FileNames.Insert(FilenameOrDirectory, idx);
			}
		}
		else
		{
			if (!bIsDirectory)
			{
				FileNames.Add(FilenameOrDirectory);
			}
		}
		
		return true;
	});
}

cv::Mat UPhotoGradingLibrary::CVMatFromImageFile(const FString FilePath)
{
	std::string fp = std::string(TCHAR_TO_UTF8(*FilePath));
	// TODO verify file exists and make sure it was read
	cv::Mat InImage = cv::imread(fp, cv::IMREAD_ANYCOLOR);
	//UE_LOG(LogTemp, Display, TEXT("ImageFileToTexture: read image of size %i %i"), InImage.cols, InImage.rows);

	return InImage;
}

double UPhotoGradingLibrary::CalculateMSE(const cv::Mat image1, const cv::Mat image2)
{	
	cv::Mat diff;
    cv::absdiff(image1, image2, diff);
    diff.convertTo(diff, CV_32F);
    diff = diff.mul(diff);
    double mse = cv::sum(diff)[0] / (double)(image1.total() * image1.channels());
	UE_LOG(LogTemp, Log, TEXT("[CalculateMSE] score: %f"), mse);
    return mse;
}

double UPhotoGradingLibrary::CalculateSSIM(const cv::Mat image1, const cv::Mat image2)
{
	cv::Mat src1Gray, src2Gray;
    cv::cvtColor(image1, src1Gray, cv::COLOR_BGR2GRAY);
    cv::cvtColor(image2, src2Gray, cv::COLOR_BGR2GRAY);

    const double C1 = 6.5025, C2 = 58.5225;
    const int d = CV_32F;

    cv::Mat src1f, src2f;
    src1Gray.convertTo(src1f, d);
    src2Gray.convertTo(src2f, d);

    cv::Mat src1fSq, src2fSq, src1fSrc2f;
    cv::multiply(src1f, src1f, src1fSq);
    cv::multiply(src2f, src2f, src2fSq);
    cv::multiply(src1f, src2f, src1fSrc2f);

    cv::Mat mu1, mu2;
    cv::GaussianBlur(src1f, mu1, cv::Size(11, 11), 1.5);
    cv::GaussianBlur(src2f, mu2, cv::Size(11, 11), 1.5);

    cv::Mat mu1Sq, mu2Sq, mu1mu2;
    cv::multiply(mu1, mu1, mu1Sq);
    cv::multiply(mu2, mu2, mu2Sq);
    cv::multiply(mu1, mu2, mu1mu2);

    cv::Mat sigma1Sq, sigma2Sq, sigma12;
    cv::GaussianBlur(src1fSq, sigma1Sq, cv::Size(11, 11), 1.5);
    cv::addWeighted(sigma1Sq, 1.0, mu1Sq, -1.0, 0.0, sigma1Sq);
    cv::GaussianBlur(src2fSq, sigma2Sq, cv::Size(11, 11), 1.5);
    cv::addWeighted(sigma2Sq, 1.0, mu2Sq, -1.0, 0.0, sigma2Sq);
    cv::GaussianBlur(src1fSrc2f, sigma12, cv::Size(11, 11), 1.5);
    cv::addWeighted(sigma12, 1.0, mu1mu2, -1.0, 0.0, sigma12);

    cv::Mat ssimMap;
    cv::divide((2 * mu1mu2 + C1) * (2 * sigma12 + C2), (mu1Sq + mu2Sq + C1) * (sigma1Sq + sigma2Sq + C2), ssimMap);

    cv::Scalar ssimScalar = cv::mean(ssimMap);
    double ssim = ssimScalar[0];

    return ssim;
}

void UPhotoGradingLibrary::DownscaleComparison(const cv::Mat in1, const cv::Mat in2, cv::Mat& out1, cv::Mat& out2)
{
	//UE_LOG(LogTemp, Display, TEXT("[PhotoGradingRescaler] image1: %d x %d"), in1.cols, in1.rows);
	//UE_LOG(LogTemp, Display, TEXT("[PhotoGradingRescaler] image2: %d x %d"), in2.cols, in2.rows);

	float aspect1 = (float)in1.cols / (float)in1.rows;
	float aspect2 = (float)in2.cols / (float)in2.rows;

	//UE_LOG(LogTemp, Display, TEXT("[PhotoGradingRescaler] Aspect ratios: %f %f"), aspect1, aspect2);
	/*if (aspect1 != aspect2)
	{
		UE_LOG(LogTemp, Error, TEXT("[PhotoGradingRescaler] Aspect ratios are different! This might be a bad comparison."));
	}*/

	// resize to same size if needed
	// we do ignore aspect ratio here but it should be fine...
	if (in1.rows != 360)
	{
		cv::resize(in1, out1, cv::Size(640, 360));
	}
	else
	{
		out1 = in1;
	}
	if (in2.rows != 360)
	{
		cv::resize(in2, out2, cv::Size(640, 360));
	}
	else
	{
		out2 = in2;
	}

	//UE_LOG(LogTemp, Display, TEXT("[PhotoGradingRescaler] Photo sizes used for comparison: %ix%i and %ix%i"), out1.cols, out1.rows, out2.cols, out2.rows);
}

double UPhotoGradingLibrary::CalculateFeatureMatching(const cv::Mat image1, const cv::Mat image2, bool outputDebugImg)
{
	cv::Mat img1;
	cv::Mat img2;
	cv::cvtColor(image1, img1, cv::COLOR_BGR2GRAY);
	cv::cvtColor(image2, img2, cv::COLOR_BGR2GRAY);
	int samples = 1000;
	cv::Ptr<cv::Feature2D> detector = cv::AKAZE::create(cv::AKAZE::DESCRIPTOR_MLDB, 64, 2);
	cv::Ptr<cv::DescriptorMatcher> matcher = cv::BFMatcher::create(cv::NORM_HAMMING, true);

    std::vector<cv::KeyPoint> keypoints1, keypoints2;
    cv::Mat descriptors1, descriptors2;

    detector->detectAndCompute(img1, cv::Mat(), keypoints1, descriptors1);
    detector->detectAndCompute(img2, cv::Mat(), keypoints2, descriptors2);

	std::vector<std::vector<cv::DMatch>> knnMatches;
	std::vector<cv::DMatch> goodMatches;


    matcher->match(descriptors1, descriptors2, goodMatches);

	double dist = 0;

	for (int i = 0; i < goodMatches.size(); i++)
	{
		dist += goodMatches[i].distance;
	}

	double lenght = 1;
	double sum = 9999;
	if (goodMatches.size() > 2) lenght = (double)goodMatches.size();
	if (dist > 1) sum = dist;
    double matches = dist / lenght;
	UE_LOG(LogTemp, Log, TEXT("[CalculateFM] score: %f inf: %d outf: %d"), matches, keypoints1.size(), keypoints2.size());
	if (outputDebugImg)
	{
		cv::Mat output;
		cv::drawMatches(img1, keypoints1, img2, keypoints2, goodMatches, output, cv::Scalar::all(-1), cv::Scalar::all(-1), std::vector<char>(), cv::DrawMatchesFlags::DRAW_RICH_KEYPOINTS | cv::DrawMatchesFlags::NOT_DRAW_SINGLE_POINTS);
		FString dir = FPaths::ProjectSavedDir();
		std::string fn = std::string(TCHAR_TO_UTF8(*dir));
		int num = rand();
		fn.append("DEBUG_");
		fn.append(std::to_string(num));
		fn.append(".png");
		cv::imwrite(fn, output);
	}
    return matches;// / 100;
}

double UPhotoGradingLibrary::CalculateFMExperimental(const cv::Mat image1, const cv::Mat image2, bool outputDebugImg)
{
	cv::Mat img1;
	cv::Mat img2;
	cv::cvtColor(image1, img1, cv::COLOR_BGR2GRAY);
	cv::cvtColor(image2, img2, cv::COLOR_BGR2GRAY);
	int samples = 1000;
	cv::Ptr<cv::Feature2D> detector = cv::AKAZE::create();
	cv::Ptr<cv::DescriptorMatcher> matcher = cv::BFMatcher::create(cv::NORM_HAMMING);

	std::vector<cv::KeyPoint> keypoints1, keypoints2;
	cv::Mat descriptors1, descriptors2;

	detector->detectAndCompute(img1, cv::Mat(), keypoints1, descriptors1);
	detector->detectAndCompute(img2, cv::Mat(), keypoints2, descriptors2);

	std::vector<std::vector<cv::DMatch>> knnMatches;
	std::vector<cv::DMatch> goodMatches;
	std::vector<cv::KeyPoint> matched1, matched2;
	std::vector<cv::Point2f> matchpoints1, matchpoints2;

	matcher->knnMatch(descriptors1, descriptors2, knnMatches, 2);

	double dist = 0;

	/*for (int i = 0; i < goodMatches.size(); i++)
	{
		dist += goodMatches[i].distance;
	}*/

	const float ratioThresh = 0.75f;
	for (size_t i = 0; i < knnMatches.size(); i++)
	{
		cv::DMatch first = knnMatches[i][0];
		float dist1 = knnMatches[i][0].distance;
		float dist2 = knnMatches[i][1].distance;

		if (dist1 < ratioThresh * dist2)
		{
			goodMatches.push_back(knnMatches[i][0]);
			matched1.push_back(keypoints1[first.queryIdx]);
			matched2.push_back(keypoints2[first.trainIdx]);
			matchpoints1.push_back(keypoints1[first.queryIdx].pt);
			matchpoints2.push_back(keypoints2[first.trainIdx].pt);
		}
	}

	cv::Mat H;
	cv::Mat mask;
	if (goodMatches.size() > 10) H = cv::findHomography(matchpoints1, matchpoints2, mask, cv::RANSAC, 3);

	std::vector<cv::DMatch> inlineMatches;
	for (int i = 0; i < mask.rows; i++)
	{
		uchar* inliner = mask.ptr<uchar>(i);
		if (inliner[0] == 1)
		{
			inlineMatches.push_back(goodMatches[i]);
		}
	}

	double lenght = 1;
	double sum = 9999;
	if (goodMatches.size() > 2) lenght = (double)goodMatches.size();
	if (dist > 1) sum = dist;
	double matches = dist / lenght;
	UE_LOG(LogTemp, Log, TEXT("[CalculateFM] similarity: %d  matches: %f  in features: %d  compare feature: %d"), goodMatches.size(), matches, keypoints1.size(), keypoints2.size());
	if (outputDebugImg && inlineMatches.size() > 2)
	{
		cv::Mat output;
		cv::drawMatches(img1, keypoints1, img2, keypoints2, inlineMatches, output, cv::Scalar::all(-1), cv::Scalar::all(-1), std::vector<char>(), cv::DrawMatchesFlags::DRAW_RICH_KEYPOINTS | cv::DrawMatchesFlags::NOT_DRAW_SINGLE_POINTS);
		std::string dbg = std::string("MATCHES: ");
		dbg.append(std::to_string(goodMatches.size()));
		dbg.append(" INLINES: ");
		dbg.append(std::to_string(inlineMatches.size()));
		dbg.append(" SCORE: ");
		dbg.append(std::to_string(matches));
		cv::putText(output, dbg, cv::Point(6, output.rows / 8), cv::FONT_HERSHEY_DUPLEX, 1.0, CV_RGB(255, 0, 0), 2);

		FString dir = FPaths::ProjectSavedDir();
		std::string fn = std::string(TCHAR_TO_UTF8(*dir));
		int num = rand();
		fn.append("DEBUG_");
		fn.append(std::to_string(num));
		fn.append(".png");
		cv::imwrite(fn, output);
	}
	return matches / 100;
}

void UPhotoGradingLibrary::GetMetadataFromFile(const FString FilePath, int& score, int& day, float& time, int& type)
{
	std::ifstream fileStream;
	std::string filename = std::string(TCHAR_TO_UTF8(*FilePath));

	int count = metadataCache.size();
	for (int i = 0; i < count; i++)
	{
		int check = strcmp(metadataCache[i].filename, filename.c_str());
		if (check)
		{
			Metadata_datablock cache = metadataCache[i].datablock;
			score = cache.score;
			day = cache.day;
			time = cache.time;
			type = cache.type;
			UE_LOG(LogTemp, Log, TEXT("metadata read success from cache on file %s"), *FilePath);
			return;
		}
	}

	fileStream.open(filename, std::ios::binary);

	if (!fileStream) {
		UE_LOG(LogTemp, Error, TEXT("could not open %s to extract metadata"), *FilePath);
		return;
	}

	fileStream.seekg(0, std::ios::end);
	std::streampos fileSize = fileStream.tellg();

	if (fileSize < sizeof(Metadata))
	{
		UE_LOG(LogTemp, Error, TEXT("%s is too small, could not possibly contain metadata"), *FilePath);
		return;
	}

	// function is stupid and only looks for the data chunk in the place where the other function would put it, end of file minus size of chunk minus 12 more bytes for IEND 
	int offset = sizeof(Metadata) - 12;

	fileStream.seekg(-offset, std::ios::end);

	std::vector<char> membuf(sizeof(Metadata));
	fileStream.read(membuf.data(), sizeof(Metadata));
	fileStream.close();

	Metadata meta{};
	char* dataPtr = reinterpret_cast<char*>(&meta);
	std::copy(membuf.data(), membuf.data() + sizeof(Metadata), dataPtr);
	
	// check if chunk header is correct, otherwise our chunk is probably not there
	const char* nameDef = "auRa";
	for (int i = 0; i < 4; i++)
	{
		if (meta.blockName[i] != nameDef[i])
		{
			UE_LOG(LogTemp, Error, TEXT("could not find metadata in %s"), *FilePath);
			return;
		}
	}

	score = meta.datablock.score;
	day = meta.datablock.day;
	time = meta.datablock.time;
	type = meta.datablock.type;

	Metadata_cacheEntry cacheEntry;
	strcpy(cacheEntry.filename, filename.c_str());
	cacheEntry.datablock = meta.datablock;
	metadataCache.push_back(cacheEntry);

	UE_LOG(LogTemp, Log, TEXT("metadata read success on file %s"), *FilePath);
	return;

}

// int UPhotoGradingLibrary::GetPointsFromFilename(const FString FileName)
// {
// 	if (FileName == "null")
//     {
//         UE_LOG(LogTemp, Error, TEXT("compare file name is null!!"));
//         return 0;
//     }
//     std::string path = std::string(TCHAR_TO_UTF8(*FileName));
//     std::string base_filename = path.substr(path.find_last_of("/\\")+1);
//     std::string no_ext = base_filename.substr(0, base_filename.size()-4); // remove extension
//
//     std::string points_str = no_ext.substr(no_ext.find_last_of("-") + 1);
//
//     int result = std::stoi(points_str);
//     UE_LOG(LogTemp, Warning, TEXT("Returning %i from %s"), result, *FileName);
//     return result;
// }

unsigned int UPhotoGradingLibrary::calculateCRC32(const char* data, int length)
{
	unsigned int crc = 0xFFFFFFFF;

	for (int i = 0; i < length; ++i)
	{
		crc ^= static_cast<unsigned int>(data[i]);

		for (int j = 0; j < 8; ++j)
		{
			if (crc & 1)
				crc = (crc >> 1) ^ 0xEDB88320;
			else
				crc >>= 1;
		}
	}

	return crc ^ 0xFFFFFFFF;
}

unsigned int UPhotoGradingLibrary::invertByteOrderInt(unsigned int x)
{
	unsigned char* s = (unsigned char*)&x;
	return (unsigned int)(s[0] << 24 | s[1] << 16 | s[2] << 8 | s[3]);
}

