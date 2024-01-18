#include "GP4_NodeAIComponent.h"

DECLARE_STATS_GROUP(TEXT("MaxFaAIComp"), STATGROUP_MaxFaAIComp, STATCAT_Advanced);
DECLARE_CYCLE_STAT(TEXT("ai avoidance"), STAT_intdist, STATGROUP_MaxFaAIComp);
DECLARE_CYCLE_STAT(TEXT("ai inode query"), STAT_aimove, STATGROUP_MaxFaAIComp);

TArray<UGP4_NodeAIComponent*> UGP4_NodeAIComponent::AIFlock;

UGP4_NodeAIComponent::UGP4_NodeAIComponent()
{
	PrimaryComponentTick.bCanEverTick = true;

	flowFieldVector = FVector::ZeroVector;
	AIVelocity = FVector::ZeroVector;
	unitSpeed = 50.0f;

}


// Called when the game starts
void UGP4_NodeAIComponent::BeginPlay()
{
	Super::BeginPlay();

	posZ = GetOwner()->GetActorLocation().Z;

	AIFlock.Add(this);
}

void UGP4_NodeAIComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	AIFlock.Remove(this);
	Super::EndPlay(EndPlayReason);
}



// Called every frame
void UGP4_NodeAIComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	UpdateFlowFieldVector();

	if (flowFieldVector != FVector::ZeroVector)
	{
		Movement(DeltaTime);
	}

}


void UGP4_NodeAIComponent::Movement(float DeltaTime)
{
	FVector Acceleration = FVector::ZeroVector;

	FVector finalLocation = GetOwner()->GetActorLocation() + (AIVelocity * DeltaTime);
	finalLocation.Z = posZ + 100.0f;

	GetOwner()->SetActorLocation(finalLocation);
	GetOwner()->SetActorRotation(FFVelocity.ToOrientationQuat());

	
	FVector sepVec = Separate(AIFlock);

	FVector ffVec = flowFieldVector * 100.0f;

	if (FVector::DotProduct(ffVec, sepVec) < 0.0f)
	{
		ffVec = ffVec / 2;
	}

	Acceleration += ffVec;

	Acceleration += sepVec;

	//update velocity
	AIVelocity += (Acceleration * DeltaTime);
	FFVelocity += (ffVec * DeltaTime);
	AIVelocity = AIVelocity.GetClampedToSize(0.0f, unitSpeed);
}


void UGP4_NodeAIComponent::SelectTarget(AActor* target, float stopWhenThisClose)
{
	if (target == NULL)
	{
		//target is null D:
		return;
	}
	this->stopRange = FMath::RoundToInt(pow(stopWhenThisClose, 2));
	targetReference = target->FindComponentByClass<UGP4_NodeAITargetComponent>();
	if (!targetReference.IsValid())
	{
		//select no target here
		return;
	}


	targetX = FMath::RoundToInt(target->GetActorLocation().X);
	targetY = FMath::RoundToInt(target->GetActorLocation().Y);

}

void UGP4_NodeAIComponent::StopMovement()
{
	targetReference = nullptr;
	this->flowFieldVector = FVector::ZeroVector;

}

void UGP4_NodeAIComponent::SetSpeed(float speed)
{
	this->unitSpeed = speed;
}

void UGP4_NodeAIComponent::UpdateFlowFieldVector()
{
	{
		SCOPE_CYCLE_COUNTER(STAT_aimove);
		if (!targetReference.IsValid())
		{
			return;
		}
		if (targetReference->LocalFlowField.empty())
		{
			return;
		}

		IFieldNode* testNode = AGP4_GridManager::GetGridManager()->GetClosestNodeFromWorldPos(GetOwner()->GetActorLocation(), targetReference->GetOwner()->GetActorLocation(), &targetReference->LocalFlowField, targetReference->radius);

		if (testNode != NULL)
		{
			this->flowFieldVector = testNode->vector;
			this->posX = testNode->localX;
			this->posY = testNode->localY;
			GridNode* node = AGP4_GridManager::GetGridManager()->GetNodePointer(testNode->gridNodeIndex);
			if (node != NULL)
			{
				if (node->type == NODE_FREE) this->posZ = node->z;
				if (AGP4_GridManager::CalcGridDistancei(node->x, node->y, this->targetX, this->targetY) < this->stopRange)
				{
					StopMovement();
					return;
				}
			}
		}
	}
}


FVector UGP4_NodeAIComponent::Separate(TArray<UGP4_NodeAIComponent*> Flock)
{
	{
		SCOPE_CYCLE_COUNTER(STAT_intdist);
		FVector Steering = FVector::ZeroVector;
		int32 FlockCount = 0;
		FVector SeparationDirection = FVector::ZeroVector;
		float ProximityFactor = 0.0f;

		//get separation steering force for each of the boid's flockmates
		for (int i = 0; i < Flock.Num() && FlockCount < 5; i++)
		{
			UGP4_NodeAIComponent* Flockmate = Flock[i];
			if (Flockmate != nullptr && Flockmate != this)
			{
				if (AGP4_GridManager::CalcGridDistancei(this->posX, this->posY, Flockmate->posX, Flockmate->posY) > 3)
				{
					continue;
				}

				if (FVector::DotProduct(GetOwner()->GetActorForwardVector(), (Flockmate->GetOwner()->GetActorLocation() - GetOwner()->GetActorLocation()).GetSafeNormal()) <= -0.5f)
				{
					continue;
				}

				//get normalized direction away from nearby boid

				FVector tVec = GetOwner()->GetActorLocation();
				tVec.Z = 0.0f;
				FVector fVec = Flockmate->GetOwner()->GetActorLocation();
				fVec.Z = 0.0f;


				SeparationDirection = tVec - fVec;
				SeparationDirection = SeparationDirection.GetSafeNormal();

				ProximityFactor = 1.0f - (SeparationDirection.Size() / 300.0f);

				if (ProximityFactor < 0.0f)
				{
					continue;
				}

				Steering += (ProximityFactor * SeparationDirection);
				FlockCount++;
			}
		}

		if (FlockCount > 0)
		{
			//get flock average separation steering force, apply separation steering strength factor and return force
			Steering /= FlockCount;
			Steering.GetSafeNormal() -= AIVelocity.GetSafeNormal();
			Steering *= 50.0f;
			return Steering;
		}
		else
		{
			return FVector::ZeroVector;
		}
	}
}
