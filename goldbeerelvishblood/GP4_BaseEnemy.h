#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
//#include "GameFramework/CharacterMovementComponent.h"
#include "Components/SphereComponent.h"
#include "Components/CapsuleComponent.h"
#include "GP4_BaseEnemy.generated.h"

class AGP4_AIDirector; //fwd decl of director class bc cannot include here
class AGP4_BasicSpawner;

UCLASS()
class AGP4_BaseEnemy : public APawn
{
	GENERATED_BODY()

public:
	AGP4_BaseEnemy();

	UPROPERTY(EditAnywhere)
	float AttackDistance;

	UPROPERTY(EditAnywhere)
	float AvoidanceDistance;

	UPROPERTY(EditAnywhere)
	float InterruptDistance;

	UPROPERTY(EditAnywhere)
	float Health;

	UPROPERTY(EditAnywhere)
	float Damage;

	UPROPERTY(EditAnywhere)
	float DamageCooldown;

	UPROPERTY(EditAnywhere)
	float UnitSpeed;

	UPROPERTY(EditAnywhere)
	float BloodGain;

	UFUNCTION(BlueprintImplementableEvent, Category = "GP4_AI_BaseEnemy")
	void SetAIControllerMoveTo(FVector targetLocation, AActor* targetActor);

	UFUNCTION(BlueprintImplementableEvent, Category = "GP4_AI_BaseEnemy")
	void OnUnitTakeDamage();

	UFUNCTION(BlueprintImplementableEvent, Category = "GP4_AI_BaseEnemy")
	void OnUnitDeath();

	UFUNCTION(BlueprintImplementableEvent, Category = "GP4_AI_BaseEnemy")
	void OnUnitAttack();

	UFUNCTION(BlueprintImplementableEvent, Category = "GP4_AI_BaseEnemy")
	void SetMovementSpeed(float Speed);

	UFUNCTION(BlueprintCallable, Category = "GP4_AI_BaseEnemy")
	void SetUnitHasCompletedPath();

	UFUNCTION(BlueprintCallable, Category = "GP4_AI_BaseEnemy")
	void UnitTakeDamage(float amount);

	UFUNCTION(BlueprintCallable, Category = "GP4_AI_BaseEnemy")
	void AttackBuilding(AGP4BaseBuilding* targetBuilding);

	UFUNCTION(BlueprintCallable, Category = "GP4_AI_BaseEnemy")
	void InitializeUnit(AGP4_AIDirector* director);
	void InitializeUnit(AGP4_BasicSpawner* director);

	UFUNCTION(BlueprintCallable, Category = "GP4_AI_BaseEnemy")
	void UnitSlowdownEffect(float SlowdownAmount, float Duration);


	TWeakObjectPtr<AActor> targetActorReference;

	FVector targetActorLocation;

	TWeakObjectPtr<AActor> savedActorReference;

	FVector savedActorLocation;

	bool savedHasBeenSet;

	TWeakObjectPtr<AGP4_AIDirector> directorReference;

	TWeakObjectPtr<AGP4_BasicSpawner> spawnerReference;

	bool HasTarget;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UPROPERTY(EditAnywhere)
	UStaticMeshComponent* Mesh;

	UPROPERTY(EditAnywhere)
	UCapsuleComponent* Collider;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	USphereComponent* RangeBubble;


	float damageTimer;
	bool slowdownEffect;
	float slowdownTimer;

public:
	virtual void Tick(float DeltaTime) override;

	void SetUnitTargetUnconditional(AActor* targetActor, FVector targetLocation);
};
