#include "GP4_AIDirector.h"
#include "GP4_WaveDirector.h"

AGP4_AIDirector::AGP4_AIDirector()
{
	PrimaryActorTick.bCanEverTick = true;
	TimeBetweenSpawns = 0.25f;
	waveIsActive = false;
	EnableWaveSpawns = true;
	MaxSpawnedEnemiesAtOnce = 250;
	spawnCounter = 0;
	SpawnRadius = 250.0f;
}

void AGP4_AIDirector::BeginPlay()
{
	Super::BeginPlay();

	enemyLists.Add(Level1_Enemies);
	enemyLists.Add(Level2_Enemies);
	enemyLists.Add(Level3_Enemies);
	enemyLists.Add(Level4_Enemies);
	enemyLists.Add(Level5_Enemies);

	rateLists.Add(Level1_Spawnrates);
	rateLists.Add(Level2_Spawnrates);
	rateLists.Add(Level3_Spawnrates);
	rateLists.Add(Level4_Spawnrates);
	rateLists.Add(Level5_Spawnrates);

	//GetWorld()->GetSubsystem<UGP4_WaveSubsystem>()->Spawners.Add(this);

	if (EnableWaveSpawns) AGP4_WaveDirector::RegisterSpawner(this);
}

void AGP4_AIDirector::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (EnableWaveSpawns) AGP4_WaveDirector::UnregisterSpawner(this);
	Super::EndPlay(EndPlayReason);
}

bool AGP4_AIDirector::SetupInitialTarget(float searchRange)
{
	/*initialTarget = ActorReference;
	intialTargetPos = ActorReference->GetActorLocation();
	RegisterTargetActor(intialTargetPos);*/

	//TArray<AActor*> searchScope;
	TArray<AGP4BaseBuilding*> buildingsVector = GetWorld()->GetSubsystem<UGP4BuildingsWorldSubsystem>()->Buildings;

	/*for (int i = 0; i < buildingsVector.Num(); i++)
	{
		AGP4BaseTower* towerRef = Cast<AGP4BaseTower>(buildingsVector[i]);
		if (towerRef)
		{
			AActor* actorRef = Cast<AActor>(buildingsVector[i]);
			searchScope.Add(actorRef);
		}
	}*/
	AGP4BaseBuilding* closestRef;
	if (GetClosestTargetBuilding(&closestRef, buildingsVector, this->GetActorLocation(), searchRange))
	{
		initialTarget = closestRef;
		intialTargetPos = closestRef->GetActorLocation();
		return true;
	}
	
	return false;
}

void AGP4_AIDirector::Tick(float DeltaTime)
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
				if (spawnCounter >= spawnList.Num())
				{
					waveIsActive = false;
					spawnCounter = 0;
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
			spawnLoc += FVector(FMath::RandRange(-SpawnRadius, SpawnRadius), FMath::RandRange(-SpawnRadius, SpawnRadius), 200.0f);
			spawnTransform.SetLocation(spawnLoc);
			if (spawnList.IsValidIndex(spawnCounter))
			{
				spawnedEnemy = GetWorld()->SpawnActor<AGP4_BaseEnemy>(spawnList[spawnCounter], spawnTransform, SpawnParams);
				spawnedEnemyWaiting = true;
			}
			else
			{
				GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Red, FString::Printf(TEXT("spawnlist empty at index")));
				waveIsActive = false;
				spawnCounter = 0;
			}
			spawnTimer = 0.0f;
		}
		else if (localUnits.Num() < MaxSpawnedEnemiesAtOnce) spawnTimer += DeltaTime;
	}

	/*if (unitsMightNeedSetup)
	{
		for (int i = 0; i < localUnits.Num(); i++)
		{
			if (localUnits[i]->HasTarget) continue;
			localUnits[i]->SetUnitTargetUnconditional(initialTarget.Get(), intialTargetPos);
		}
		unitsMightNeedSetup = false;
	}*/
}

void AGP4_AIDirector::RegisterUnit(AGP4_BaseEnemy* Unit)
{
	if (localUnits.Contains(Unit)) return;

	localUnits.Add(Unit);
	unitsMightNeedSetup = true;
}

void AGP4_AIDirector::UnRegisterUnit(AGP4_BaseEnemy* Unit)
{
	if (!this) return;
	if (!localUnits.Contains(Unit)) return;

	localUnits.Remove(Unit);
}

void AGP4_AIDirector::RequestTargetForUnit(AGP4_BaseEnemy* unit)
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

bool AGP4_AIDirector::RequestSpawnWave(int amount, float searchRadius)
{
	//if (!EnableWaveSpawns) return false;
	if (SetupInitialTarget(searchRadius))
	{
		int level = GetWorld()->GetSubsystem<UGP4WorldSubsystemResorce>()->FortAmount;
		level--;
		if (level < 0) level = 0;
		if (level > 4) level = 4;
		if (GenerateSpawnList(level, amount))
		{
			currentSpawnAmount = amount;
			waveIsActive = true;
		}
		return true;
	}
	return false;
}

bool AGP4_AIDirector::GetClosestTarget(AActor **ActorRef, TArray<AActor*> searchVector, FVector Location, float MaxDistance)
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

bool AGP4_AIDirector::GetClosestTargetBuilding(AGP4BaseBuilding** ActorRef, TArray<AGP4BaseBuilding*> searchVector, FVector Location, float MaxDistance)
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


bool AGP4_AIDirector::GenerateSpawnList(int level, int amount)
{

	if (rateLists[level].Num() != enemyLists[level].Num()) return false;

	TArray<TSubclassOf<AGP4_BaseEnemy>> interSpawnList;
	TArray<int> amountList;

	for (int i = 0; i < rateLists[level].Num(); i++)
	{
		int a = amount * rateLists[level][i];
		amountList.Add(a);
	}

	for (int i = 0; i < amountList.Num(); i++)
	{
		for (int j = 0; j < amountList[i]; j++)
		{
			interSpawnList.Add(enemyLists[level][i]);
		}
	}

	spawnList = interSpawnList;

	return true;

}

