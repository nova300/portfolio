// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GP4_WaveDirector.generated.h"

class AGP4_AIDirector; //fwd decl of director class bc cannot include here

UCLASS()
class GOLDBEERELVISHBLOOD_API AGP4_WaveDirector : public AActor
{
	GENERATED_BODY()

public:

	AGP4_WaveDirector();

	virtual void Tick(float DeltaTime) override;

	static TArray<AGP4_AIDirector*> Spawners;

	static int currentActiveRaids;

	int raidCount;

	float raidTimer;

	UPROPERTY(EditAnywhere)
	bool printDebugCountdown;

	UPROPERTY(EditAnywhere)
	float difficulty;

	UPROPERTY(EditAnywhere)
	float timeBetweenRaids;

	UPROPERTY(EditAnywhere)
	float raidRange;

	UPROPERTY(EditAnywhere)
	int initialSpawnAmount;

	UPROPERTY(EditAnywhere)
	int increaseAmountEachRaid;


	static void RegisterSpawner(AGP4_AIDirector* spawner);
	static void UnregisterSpawner(AGP4_AIDirector* spawner);



	
};
