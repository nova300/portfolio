#include "PhotoSubjectComponent.h"

/*

objects that are inactive/not on screen run a simpler check to see if they have appeared on screen again
objects active on screen run the more advanced checks and doing calculations on the found collision point

distance variable is self adjusting
distance variable is self adjusting

*/


TArray<UPhotoSubjectComponent*> UPhotoSubjectComponent::onScreenObjects;
bool UPhotoSubjectComponent::runChecks = false;

UPhotoSubjectComponent::UPhotoSubjectComponent()
{
	PrimaryComponentTick.bCanEverTick = true;

	maxMultiplier = 1.0f;
	checkIfBlocked = true;

}

void UPhotoSubjectComponent::BeginDestroy()
{
	Super::BeginDestroy();

	resetArrays();
}

void UPhotoSubjectComponent::BeginPlay()
{
	Super::BeginPlay();
	isOnScreen = false;
	pc = GetWorld()->GetFirstPlayerController();
	camManager = GetWorld()->GetFirstPlayerController()->PlayerCameraManager;
	lastKnownLocation = GetOwner()->GetActorLocation();
	scoreMultiplier = 1.0f;
	if (doChecksOnMesh)
	{
		staticMesh = Cast<UPrimitiveComponent>(StaticMeshReference.GetComponent(GetOwner()));
		if (staticMesh == nullptr)
		{
			doChecksOnMesh = false;
			UE_LOG(LogTemp, Error, TEXT("[PhotoSubjectComponent] a collider object was provided but returned null on cast, reverting to centerpoint based detection"));
		} 
	}
	if (useActorBounds)
	{
		actorBox = GetOwner()->GetComponentsBoundingBox(true);
		doChecksOnMesh = true;
	}
}

void UPhotoSubjectComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

#if !UE_BUILD_SHIPPING
	if (useActorBounds)
	{
		DrawDebugBox(GetWorld(), actorBox.GetCenter(), actorBox.GetExtent(), FColor(255,0,0));
	}
#endif	

	if (!runChecks) return;

	if (viewDistance > 0.0f)
	{
		float dist = (camManager->GetCameraLocation() - GetOwner()->GetActorLocation()).Length();
		if (dist > viewDistance)
		{
			if (isOnScreen)
			{
				isOnScreen = false;
				SetOffscreen();
			}
			return;
		}
	}

	if (!isOnScreen)
	{
		isOnScreen = checkIfPosOnScreen(GetOwner()->GetActorLocation());
		if (!isOnScreen) isOnScreen = checkIfPosOnScreen(lastKnownLocation);
		if (isOnScreen) SetOnscreen();
		return;
	}

	FVector cast;
	FVector checkPoint = getClosestPoint(cast);
	bool check = checkIfPosOnScreen(checkPoint);

	if (!check)
	{
		isOnScreen = false;
		SetOffscreen();
		return;
	}
#if !UE_BUILD_SHIPPING
	DrawDebugSphere(GetWorld(), lastKnownLocation, 50, 6, FColor(255,0,0));
#endif
}

bool UPhotoSubjectComponent::checkIfPosOnScreen(FVector pos)
{
	bool ok;
	FVector2D screenPos;
	ok = pc->ProjectWorldLocationToScreen(pos, screenPos, true);

	pc->GetViewportSize(screenWidth, screenHeight);

	if (!ok) return false;
	if (screenPos.X > screenWidth || screenPos.X < 0.0f) return false;
	if (screenPos.Y > screenHeight || screenPos.Y < 0.0f) return false;

	return true;
}

FVector UPhotoSubjectComponent::getClosestPoint(FVector &castPoint)
{
	if (!isOnScreen)
	{
		return GetOwner()->GetActorLocation();
	}
	
	FVector camPos = camManager->GetCameraLocation();
	FVector camFowardVector = camManager->GetCameraRotation().Vector();

	if (!doChecksOnMesh)
	{
		FVector point;
		point = GetOwner()->GetActorLocation();
		FVector diff = point - camPos;
		diff.Normalize();
		float diffDot = FVector::DotProduct(camFowardVector, diff);
		scoreMultiplier = (diffDot - 0.7f) * 1.0f / (1.0f - 0.7f);
		return point;
	}

	if (lastDist < 0.0f)
	{
		newDist = (camPos - GetOwner()->GetActorLocation()).Length();
	}
	else
	{
		newDist = lastDist;
	}

	FVector castedPoint = (camFowardVector * newDist) + camPos;
	castPoint = castedPoint;
	FVector point;
	if (useActorBounds)
	{
		point = actorBox.GetClosestPointTo(castedPoint);
	}
	else
	{
		staticMesh->GetClosestPointOnCollision(castedPoint, point);
	}
	

	lastDist = (point - camPos).Length();

	FVector diff = point - camPos;
	diff.Normalize();
	float diffDot = FVector::DotProduct(camFowardVector, diff);
	float diffDist = (castedPoint - point).Length();

	lastDist += diffDist - (diffDist * diffDot); //adjust for big angles when casted point has big difference from closest

	scoreMultiplier = (diffDot - 0.7f) * 1.0f / (1.0f - 0.7f);

	lastKnownLocation = point;
	return point;
}

void UPhotoSubjectComponent::enableRunChecks()
{
	runChecks = true;
}
void UPhotoSubjectComponent::disableRunChecks()
{
	runChecks = false;
}

float UPhotoSubjectComponent::getscore()
{
	if (checkIfBlocked)
	{
		bool rayCheck;
		FCollisionQueryParams CollisionParams;
		CollisionParams.AddIgnoredActor(GetOwner());
		FHitResult hit;
		rayCheck = GetWorld()->LineTraceSingleByChannel(hit, camManager->GetCameraLocation(), lastKnownLocation, ECollisionChannel::ECC_Visibility, CollisionParams);
		if (rayCheck)
		{
			rayCheck = hit.bBlockingHit;
			if (rayCheck) return 1.0f;
		}
	}
	if (GetOwner()->IsHidden()) return 1.0f;
	return maxMultiplier * scoreMultiplier;
}

void UPhotoSubjectComponent::resetArrays()
{
	onScreenObjects.Empty();
	runChecks = false;
}

void UPhotoSubjectComponent::SetOnscreen()
{
	onScreenObjects.Add(this);
}

void UPhotoSubjectComponent::SetOffscreen()
{
	onScreenObjects.Remove(this);
}


float UPhotoSubjectComponent::GetOnscreenScore()
{
	float total = 0;
	int count = onScreenObjects.Num();
	for (int i = 0; i < count; i++)
	{
		float s = onScreenObjects[i]->getscore();
		s = s - 1.0f;
		if (s > 0.0f) total = total + s;
	}

	return total + 1.0f;
}

void UPhotoSubjectComponent::getOnscreenInfo(float& totalScore, int& ObjectsOnScreen)
{
	totalScore = GetOnscreenScore();
	ObjectsOnScreen = onScreenObjects.Num();
}

void UPhotoSubjectComponent::setStaticMeshComponent(UPrimitiveComponent* StaticMeshComponent)
{
	if (doChecksOnMesh) return;
	staticMesh = StaticMeshComponent;
	doChecksOnMesh = true;
}
