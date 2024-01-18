#pragma once

#include "CoreMinimal.h"
#include "Components/SceneComponent.h"
#include "Components/PrimitiveComponent.h"
#include "Math/Box.h"
#include "Components/StaticMeshComponent.h"
#if !UE_BUILD_SHIPPING
#include "DrawDebugHelpers.h"
#endif 
#include "PhotoSubjectComponent.generated.h"

UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class UPhotoSubjectComponent : public USceneComponent
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere)
	bool checkIfBlocked;

	UPROPERTY(EditAnywhere)
	bool useActorBounds;

	UPROPERTY(EditAnywhere)
	bool doChecksOnMesh;

	UPROPERTY(EditAnywhere, meta=(UseComponentPicker,AllowedClasses="ShapeComponent, StaticMeshComponent",DisallowCreateNew))
	FComponentReference StaticMeshReference;

	UPROPERTY(EditAnywhere)
	float maxMultiplier;

	UPROPERTY(EditAnywhere)
	float viewDistance;
	
public:	
	UPhotoSubjectComponent();

	UFUNCTION(BlueprintCallable)
	static void enableRunChecks();

	UFUNCTION(BlueprintCallable)
	static void disableRunChecks();

	UFUNCTION(BlueprintCallable)
	static void resetArrays();

	UFUNCTION(BlueprintCallable)
	static void getOnscreenInfo(float &totalScore, int &ObjectsOnScreen);

	UFUNCTION(BlueprintCallable)
	void setStaticMeshComponent(UPrimitiveComponent *StaticMeshComponent);

	bool checkIfPosOnScreen(FVector pos);

	UFUNCTION(BlueprintCallable)
	FVector getClosestPoint(FVector &castPoint);

	static bool runChecks;

	static TArray<UPhotoSubjectComponent*> onScreenObjects;

	UFUNCTION(BlueprintCallable)
	static float GetOnscreenScore();

	void SetOnscreen();
	void SetOffscreen();
	float getscore();

protected:
	virtual void BeginPlay() override;
	virtual void BeginDestroy() override;

public:
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;


private:
	bool isOnScreen;
	float lastDist;
	float newDist;
	float scoreMultiplier;
	FVector lastKnownLocation;
	int screenWidth;
	int screenHeight;
	APlayerController* pc;
	APlayerCameraManager* camManager;
	UPrimitiveComponent* staticMesh;
	FBox actorBox;



};
