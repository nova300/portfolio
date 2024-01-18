#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GP4_NodeAITargetComponent.h"
#include "GP4_BaseEnemy.h"
#include "GP4_GridManager.h"
#include "GP4_NodeAIComponent.generated.h"


UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class UGP4_NodeAIComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	UGP4_NodeAIComponent();

	UPROPERTY(EditAnywhere)
	float unitSpeed;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

public:	
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	UFUNCTION(BlueprintCallable, Category = "GP4_AI_NodeAIComponent")
	void SelectTarget(AActor* target, float stopWhenThisClose);
	
	UFUNCTION(BlueprintCallable, Category = "GP4_AI_NodeAIComponent")
	void StopMovement();

	UFUNCTION(BlueprintCallable, Category = "GP4_AI_NodeAIComponent")
	void SetSpeed(float speed);

	int posX;
	int posY;
	float posZ;

private:
	FVector AIVelocity;
	FVector FFVelocity;

	void Movement(float DeltaTime); 
	
	void UpdateFlowFieldVector();

	FVector flowFieldVector;

	static TArray<UGP4_NodeAIComponent*> AIFlock;

	TWeakObjectPtr<UGP4_NodeAITargetComponent> targetReference;

	float stopRange;
	int targetX;
	int targetY;

	FVector Separate(TArray<UGP4_NodeAIComponent*> Flock);
		
};
