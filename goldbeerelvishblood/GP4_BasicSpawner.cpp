#include "GP4_BasicSpawner.h"

AGP4_BasicSpawner::AGP4_BasicSpawner()
{
	PrimaryActorTick.bCanEverTick = true;
	TimeBetweenSpawns = 0.25f;
	waveIsActive = false;
	waveCounter = 0;
	initialSpawnAmount = 10;
	spawnIncreaseEachWave = 10;
	TimeBetweenWaves = 5.0f;
	MaxSpawnedEnemiesAtOnce = 250;
	SearchRadius = 2000.0f;

	SpawnRadius = 250.0f;
}

void AGP4_BasicSpawner::BeginPlay()
{
	Super::BeginPlay();
}

void AGP4_BasicSpawner::BeginDestroy()
{
	Super::BeginDestroy();
}

bool AGP4_BasicSpawner::SetupInitialTarget(float searchRange)
{
	TArray<AGP4BaseBuilding*> buildingsVector = GetWorld()->GetSubsystem<UGP4BuildingsWorldSubsystem>()->Buildings;

	AGP4BaseBuilding* closestRef;
	if (GetClosestTargetBuilding(&closestRef, buildingsVector, this->GetActorLocation(), searchRange))
	{
		initialTarget = closestRef;
		intialTargetPos = closestRef->GetActorLocation();
		return true;
	}
	
	return false;
}

void AGP4_BasicSpawner::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (waveIsActive)
	{
		if (spawnedEnemyWaiting)
		{
			if (spawnedEnemy.IsValid())
			{
				spawnedEnemy->InitializeUnit(this);
				spawnedEnemyWaiting = false;
				spawnCounter++;
				if (spawnCounter > currentSpawnAmount)
				{
					waveIsActive = false;
					waveCounter++;
					waveTimer = 0.0f;
				}
			}
			else
			{
				// spawning failed, retry
				spawnedEnemyWaiting = false;
			}
		}
		else if (spawnTimer > TimeBetweenSpawns)
		{
			FActorSpawnParameters SpawnParams;
			FTransform spawnTransform = GetTransform();
			FVector spawnLoc = spawnTransform.GetLocation();
			spawnLoc += FVector(FMath::RandRange(-SpawnRadius, SpawnRadius), FMath::RandRange(-SpawnRadius, SpawnRadius), spawnLoc.Z + 150.0f);
			spawnTransform.SetLocation(spawnLoc);
			spawnedEnemy = GetWorld()->SpawnActor<AGP4_BaseEnemy>(EnemyBlueprint, spawnTransform, SpawnParams);
			spawnedEnemyWaiting = true;
			spawnTimer = 0.0f;
		}
		else if (localUnits.Num() < MaxSpawnedEnemiesAtOnce) spawnTimer += DeltaTime;
	}
	else
	{
		if (waveTimer > TimeBetweenWaves)
		{
			if (SetupInitialTarget(SearchRadius))
			{
				currentSpawnAmount = initialSpawnAmount + (spawnIncreaseEachWave * waveCounter);
				waveIsActive = true;
			}
		}
		waveTimer += DeltaTime;
	}
}

void AGP4_BasicSpawner::RegisterUnit(AGP4_BaseEnemy* Unit)
{
	if (localUnits.Contains(Unit)) return;

	localUnits.Add(Unit);
	unitsMightNeedSetup = true;
}

void AGP4_BasicSpawner::UnRegisterUnit(AGP4_BaseEnemy* Unit)
{
	if (!this) return;
	if (!localUnits.Contains(Unit)) return;

	localUnits.Remove(Unit);
}

void AGP4_BasicSpawner::RequestTargetForUnit(AGP4_BaseEnemy* unit)
{
	if (initialTarget.IsValid())
	{
		unit->SetUnitTargetUnconditional(initialTarget.Get(), intialTargetPos);
	}
	else
	{
		SetupInitialTarget(99999.0f);
		unit->SetUnitTargetUnconditional(initialTarget.Get(), intialTargetPos);
	}
}

bool AGP4_BasicSpawner::GetClosestTarget(AActor **ActorRef, TArray<AActor*> searchVector, FVector Location, float MaxDistance)
{
	float bestDist = MaxDistance;
	bool foundTarget = false;
	for (int i = 0; i < searchVector.Num(); i++)
	{
		FVector t = searchVector[i]->GetActorLocation();
		float disT = FVector::Distance(Location, t);
		if (disT < bestDist)
		{
			*ActorRef = searchVector[i];
			bestDist = disT;
			foundTarget = true;
		}
	}
	return foundTarget;
}

bool AGP4_BasicSpawner::GetClosestTargetBuilding(AGP4BaseBuilding** ActorRef, TArray<AGP4BaseBuilding*> searchVector, FVector Location, float MaxDistance)
{
	float bestDist = MaxDistance;
	bool foundTarget = false;
	for (int i = 0; i < searchVector.Num(); i++)
	{
		FVector t = searchVector[i]->GetActorLocation();
		float disT = FVector::Distance(Location, t);
		if (disT < bestDist)
		{
			*ActorRef = searchVector[i];
			bestDist = disT;
			foundTarget = true;
		}
	}
	return foundTarget;
}

