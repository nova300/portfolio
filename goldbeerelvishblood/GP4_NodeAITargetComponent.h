#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GP4_GridManager.h"
#include "GP4_NodeAITargetComponent.generated.h"


UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class UGP4_NodeAITargetComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	UGP4_NodeAITargetComponent();

	std::vector<std::vector<int>> LocalGridNodeMap; //array for indirect 2d adressing
	std::vector<int> LocalGridNodes;				//array for indirect linear adressing

	std::vector<std::vector<IFieldNode>> LocalFlowField;

	UPROPERTY(EditAnywhere)
		int radius;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	bool ready;
	bool inProgress;

	int ffx;
	int ffy;

	int radSqr;

	TWeakObjectPtr<AGP4_GridManager> gridManagerReference;

	static TArray<TWeakObjectPtr<UGP4_NodeAITargetComponent>> activeFlowFields;

	void UpdateFlowField();

public:	
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

		
};
