#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GP4_BaseEnemy.h"
#include "GP4_WaveDirector.h"
#include "Math/UnrealMathUtility.h"
#include "Kismet/GameplayStatics.h" 
#include "../Buildings/GP4BuildingsWorldSubsystem.h"
#include "../Buildings/GP4BaseTower.h"
#include "../GP4WorldSubsystemResorce.h"
#include "GP4_AIDirector.generated.h"

UCLASS()
class AGP4_AIDirector : public AActor
{
	GENERATED_BODY()
	
public:	
	AGP4_AIDirector();

	UPROPERTY(EditAnywhere, Category = "GP4_AIDirector")
		TArray<TSubclassOf<AGP4_BaseEnemy>> Level1_Enemies;

	UPROPERTY(EditAnywhere, Category = "GP4_AIDirector")
		TArray<float> Level1_Spawnrates;

	UPROPERTY(EditAnywhere, Category = "GP4_AIDirector")
		TArray<TSubclassOf<AGP4_BaseEnemy>> Level2_Enemies;

	UPROPERTY(EditAnywhere, Category = "GP4_AIDirector")
		TArray<float> Level2_Spawnrates;

	UPROPERTY(EditAnywhere, Category = "GP4_AIDirector")
		TArray<TSubclassOf<AGP4_BaseEnemy>> Level3_Enemies;

	UPROPERTY(EditAnywhere, Category = "GP4_AIDirector")
		TArray<float> Level3_Spawnrates;

	UPROPERTY(EditAnywhere, Category = "GP4_AIDirector")
		TArray<TSubclassOf<AGP4_BaseEnemy>> Level4_Enemies;

	UPROPERTY(EditAnywhere, Category = "GP4_AIDirector")
		TArray<float> Level4_Spawnrates;

	UPROPERTY(EditAnywhere, Category = "GP4_AIDirector")
		TArray<TSubclassOf<AGP4_BaseEnemy>> Level5_Enemies;

	UPROPERTY(EditAnywhere, Category = "GP4_AIDirector")
		TArray<float> Level5_Spawnrates;

	//should this spawner respect calls from the wave subsystem?
	UPROPERTY(EditAnywhere, Category = "GP4_AIDirector")
		bool EnableWaveSpawns;

	//might be unstable if less than 0.2
	UPROPERTY(EditAnywhere, Category = "GP4_AIDirector")
		float TimeBetweenSpawns;

	UPROPERTY(EditAnywhere, Category = "GP4_AIDirector")
		int MaxSpawnedEnemiesAtOnce;

	UPROPERTY(EditAnywhere, Category = "GP4_AIDirector")
		float SpawnRadius;

	UFUNCTION(BlueprintCallable, Category = "GP4_AIDirector")
	void RegisterUnit(AGP4_BaseEnemy *Unit);

	UFUNCTION(BlueprintCallable, Category = "GP4_AIDirector")
	void UnRegisterUnit(AGP4_BaseEnemy *Unit);

	static bool GetClosestTarget(AActor** ActorRef, TArray<AActor*> searchVector, FVector Location, float MaxDistance = 200.0f);

	static bool GetClosestTargetBuilding(AGP4BaseBuilding** ActorRef, TArray<AGP4BaseBuilding*> searchVector, FVector Location, float MaxDistance = 200.0f);

	UFUNCTION(BlueprintImplementableEvent, Category = "GP4_AIDirector")
	void RequestSetupInitialTarget();

	void RequestTargetForUnit(AGP4_BaseEnemy *unit);

	UFUNCTION(BlueprintCallable, Category = "GP4_AIDirector")
	bool SetupInitialTarget(float searchRange);

	UFUNCTION(BlueprintCallable, Category = "GP4_AIDirector")
	bool RequestSpawnWave(int amount, float searchRadius);

	UPROPERTY()
	TArray<AGP4_BaseEnemy*> localUnits;

	bool unitsMightNeedSetup;

	TWeakObjectPtr<AActor> initialTarget;
	//AActor* initialTarget;

	FVector intialTargetPos;

	bool waveIsActive;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	float spawnTimer;
	float waveTimer;
	int spawnCounter;
	int waveCounter;
	int currentSpawnAmount;

	bool spawnedEnemyWaiting;
	TWeakObjectPtr<AGP4_BaseEnemy> spawnedEnemy;

	TArray<TSubclassOf<AGP4_BaseEnemy>> spawnList;

	TArray<TArray<TSubclassOf<AGP4_BaseEnemy>>> enemyLists;
	TArray<TArray<float>> rateLists;

	bool GenerateSpawnList(int level, int amount);



public:	
	virtual void Tick(float DeltaTime) override;

};
