#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GP4_BaseEnemy.h"
#include "Math/UnrealMathUtility.h"
#include "Kismet/GameplayStatics.h" 
#include "../Buildings/GP4BuildingsWorldSubsystem.h"
#include "../Buildings/GP4BaseTower.h"
#include "GP4_BasicSpawner.generated.h"

UCLASS()
class AGP4_BasicSpawner : public AActor
{
	GENERATED_BODY()
	
public:	
	AGP4_BasicSpawner();

	UPROPERTY(EditAnywhere, Category = "AGP4_BasicSpawner")
		TSubclassOf<AGP4_BaseEnemy> EnemyBlueprint;

	//might be unstable if less than 0.2
	UPROPERTY(EditAnywhere, Category = "AGP4_BasicSpawner")
		float TimeBetweenSpawns;

	UPROPERTY(EditAnywhere, Category = "AGP4_BasicSpawner")
		float TimeBetweenWaves;

	UPROPERTY(EditAnywhere, Category = "AGP4_BasicSpawner")
		int initialSpawnAmount;

	UPROPERTY(EditAnywhere, Category = "AGP4_BasicSpawner")
		int spawnIncreaseEachWave;

	UPROPERTY(EditAnywhere, Category = "AGP4_BasicSpawner")
		int MaxSpawnedEnemiesAtOnce;

	UPROPERTY(EditAnywhere, Category = "AGP4_BasicSpawner")
		float SpawnRadius;

	UPROPERTY(EditAnywhere, Category = "AGP4_BasicSpawner")
		float SearchRadius;

	UFUNCTION(BlueprintCallable, Category = "AGP4_BasicSpawner")
	void RegisterUnit(AGP4_BaseEnemy *Unit);

	UFUNCTION(BlueprintCallable, Category = "AGP4_BasicSpawner")
	void UnRegisterUnit(AGP4_BaseEnemy *Unit);

	static bool GetClosestTarget(AActor** ActorRef, TArray<AActor*> searchVector, FVector Location, float MaxDistance = 200.0f);

	static bool GetClosestTargetBuilding(AGP4BaseBuilding** ActorRef, TArray<AGP4BaseBuilding*> searchVector, FVector Location, float MaxDistance = 200.0f);

	void RequestTargetForUnit(AGP4_BaseEnemy *unit);

	UFUNCTION(BlueprintCallable, Category = "AGP4_BasicSpawner")
	bool SetupInitialTarget(float searchRange);

	UPROPERTY()
	TArray<AGP4_BaseEnemy*> localUnits;

	bool unitsMightNeedSetup;

	TWeakObjectPtr<AActor> initialTarget;
	//AActor* initialTarget;

	FVector intialTargetPos;

	bool waveIsActive;

protected:
	virtual void BeginPlay() override;
	virtual void BeginDestroy() override;

	float spawnTimer;
	float waveTimer;
	int spawnCounter;
	int waveCounter;
	int currentSpawnAmount;

	bool spawnedEnemyWaiting;
	TWeakObjectPtr<AGP4_BaseEnemy> spawnedEnemy;



public:	
	virtual void Tick(float DeltaTime) override;

};
