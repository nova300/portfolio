#include "GP4_WaveDirector.h"
#include "GP4_AIDirector.h"


TArray<AGP4_AIDirector*> AGP4_WaveDirector::Spawners;
int AGP4_WaveDirector::currentActiveRaids = 0;

AGP4_WaveDirector::AGP4_WaveDirector()
{
	PrimaryActorTick.bCanEverTick = true;
	currentActiveRaids = 0;

}

void AGP4_WaveDirector::Tick(float DeltaTime)
{
	if (printDebugCountdown)
	{
		GEngine->AddOnScreenDebugMessage(-1, 0.0f, FColor::Red, FString::Printf(TEXT("Raid Countdown: %f"), timeBetweenRaids - raidTimer));
	}
	bool raidsActive = false;
	for (int i = 0; i < Spawners.Num(); i++)
	{
		if (Spawners[i]->waveIsActive) raidsActive = true;
	}
	if (!raidsActive)
	{
		if (raidTimer > timeBetweenRaids)
		{
			int currentSpawnAmount = initialSpawnAmount + (increaseAmountEachRaid * raidCount);
			for (int i = 0; i < Spawners.Num(); i++)
			{
				if (Spawners[i] == nullptr) continue;
				Spawners[i]->RequestSpawnWave(currentSpawnAmount, raidRange);
			}
			
			raidCount++;
			raidTimer = 0.0f;

		}
		else
		{
			raidTimer += DeltaTime;
		}
	}

}


void AGP4_WaveDirector::RegisterSpawner(AGP4_AIDirector* spawner)
{
	if (!Spawners.Contains(spawner))
	{
		Spawners.Add(spawner);
	}
}

void AGP4_WaveDirector::UnregisterSpawner(AGP4_AIDirector* spawner)
{
	if (Spawners.Contains(spawner))
	{
		Spawners.Remove(spawner);
	}
}