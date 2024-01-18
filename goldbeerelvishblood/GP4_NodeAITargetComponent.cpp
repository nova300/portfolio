#include "GP4_NodeAITargetComponent.h"

TArray<TWeakObjectPtr<UGP4_NodeAITargetComponent>> UGP4_NodeAITargetComponent::activeFlowFields;

UGP4_NodeAITargetComponent::UGP4_NodeAITargetComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	ready = false;
	radius = 50000;
	inProgress = false;
}


void UGP4_NodeAITargetComponent::BeginPlay()
{
	Super::BeginPlay();

	ffx = FMath::RoundToInt(GetOwner()->GetActorLocation().X);
	ffy = FMath::RoundToInt(GetOwner()->GetActorLocation().Y);

	radSqr = FMath::RoundToInt(pow(radius, 2));

	for (int i = 0; i < activeFlowFields.Num(); i++)
	{
		if (!activeFlowFields[i].IsValid()) continue;
		if (AGP4_GridManager::CalcGridDistancei(activeFlowFields[i]->ffx, activeFlowFields[i]->ffy, ffx, ffy) < radSqr)
		{
			activeFlowFields[i]->UpdateFlowField();
		}
	}

	activeFlowFields.Add(this);
}

void UGP4_NodeAITargetComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	
	if (activeFlowFields.Contains(this)) activeFlowFields.Remove(this);
	Super::EndPlay(EndPlayReason);

}

void UGP4_NodeAITargetComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	/*if (ready && !inProgress)
	{
		for (int i = 0; i < LocalFlowField.size(); i++)
		{
			for (int j = 0; j < LocalFlowField[i].size(); j++)
			{
				if (LocalFlowField[i][j].gridNodeIndex != -1)
				{
					GridNode* node = AGP4_GridManager::GetGridManager()->GetNodePointer(LocalFlowField[i][j].gridNodeIndex);
					//DrawDebugSphere(GetWorld(), FVector(node->x, node->y, node->z), 50, 4, FColor((unsigned int)(LocalFlowField[i][j].bestCost)));
					DrawDebugDirectionalArrow(GetWorld(), FVector(node->x, node->y, node->z), FVector(node->x, node->y, node->z) + (LocalFlowField[i][j].vector * 100.0f), 200.0f, FColor(255, 0, 0));
				}
			}
		}
	}*/

	if (!ready && !inProgress && AGP4_GridManager::GetGridManager() != NULL)
	{
		LocalGridNodeMap = AGP4_GridManager::GetGridManager()->GenerateCloseNodes(GetOwner()->GetActorLocation(), radius);

		for (int i = 0; i < LocalGridNodeMap.size(); i++)
		{
			for (int j = 0; j < LocalGridNodeMap[i].size(); j++)
			{
				if (LocalGridNodeMap[i][j] != -1)
				{
					LocalGridNodes.push_back(LocalGridNodeMap[i][j]);
				}
			}
		}

		ready = true;

		UpdateFlowField();

		//int x = LocalGridNodeMap.size() / 2;
		//int y = LocalGridNodeMap[0].size() / 2;
		//LocalFlowField = AGP4_GridManager::GetGridManager()->GenerateFlowField(LocalGridNodeMap, x, y);
	}
}

void UGP4_NodeAITargetComponent::UpdateFlowField()
{
	if (!ready) return;
	if (inProgress) return;
	inProgress = true;
	TWeakObjectPtr<UGP4_NodeAITargetComponent> callerPtr;
	std::vector<std::vector<int>> asyncMap;
	asyncMap = LocalGridNodeMap;
	callerPtr = this;

	AsyncTask(ENamedThreads::AnyHiPriThreadNormalTask, [callerPtr, asyncMap]()
		{
			int x = asyncMap.size() / 2;
			int y = asyncMap[0].size() / 2;
			std::vector<std::vector<IFieldNode>> newFlowField = AGP4_GridManager::GetGridManager()->GenerateFlowField(asyncMap, x, y);

			AsyncTask(ENamedThreads::GameThread, [callerPtr, newFlowField]()
				{
					UE_LOG(LogTemp, Display, TEXT("async stuff is done"));

					// cancel if game is exiting
					if (!callerPtr.IsValid())
					{
						UE_LOG(LogTemp, Display, TEXT("Seems like the game is quitting, cancelling async"));
						return;
					}

					callerPtr->LocalFlowField = newFlowField;
					callerPtr->inProgress = false;

				});
		});

}

