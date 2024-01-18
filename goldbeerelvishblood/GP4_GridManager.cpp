#include "GP4_GridManager.h"

DECLARE_STATS_GROUP(TEXT("MaxFa"), STATGROUP_MaxFa, STATCAT_Advanced);
DECLARE_CYCLE_STAT(TEXT("calculate inode map"), STAT_inodemapgen, STATGROUP_MaxFa);
DECLARE_CYCLE_STAT(TEXT("calculate inode values"), STAT_inodecalcvalues, STATGROUP_MaxFa);
DECLARE_CYCLE_STAT(TEXT("calculate inode score"), STAT_inodecalcscore, STATGROUP_MaxFa);
DECLARE_CYCLE_STAT(TEXT("calculate inode vectors"), STAT_inodecalcvectors, STATGROUP_MaxFa);

TWeakObjectPtr<AGP4_GridManager> AGP4_GridManager::globalGridManagerReference;

// Sets default values
AGP4_GridManager::AGP4_GridManager()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
	originZ = 500.0f;

	GroundCollisionChannel = ECollisionChannel::ECC_Visibility;

	sizeX = 300000;
	sizeY = 300000;
	cellSize = 500;

	ready = false;

}

void AGP4_GridManager::BeginPlay()
{
	Super::BeginPlay();

	/*AsyncTask(ENamedThreads::AnyHiPriThreadNormalTask, [this]()
	{
			originX = (int)GetActorLocation().X;
			originY = (int)GetActorLocation().Y;
			originZ = GetActorLocation().Z;

			//first construct data array and save indicies for later
			for (int i = -sizeX + originX; i < sizeX + originX; i += cellSize)
			{
				std::vector<int> lineIndicies;
				for (int j = -sizeY + originY; j < sizeY + originY; j += cellSize)
				{
					int index = -1; // -1 means save this as NULL in the pointer array
					GridNode g;
					g.x = i;
					g.y = j;

					FVector start = FVector(i, j, originZ);
					FVector end = start;
					end.Z += -1.0f * 2000.0f;

					FCollisionQueryParams CollisionParams;
					FHitResult hit;
					bool rayCheck = GetWorld()->LineTraceSingleByChannel(hit, start, end, GroundCollisionChannel, CollisionParams);
					if (rayCheck)
					{
						rayCheck = hit.bBlockingHit;
						if (rayCheck)
						{
							g.z = hit.ImpactPoint.Z;
							GridNodes.push_back(g);
							index = GridNodes.size() - 1;
							stored++;
						}
					}
					lineIndicies.push_back(index);
				}
				GridNodeIndicies.push_back(lineIndicies);
			}

			// assign coords and neighbors to nodes
			for (int i = 0; i < GridNodeIndicies.size(); i++)
			{
				for (int j = 0; j < GridNodeIndicies[i].size(); j++)
				{
					if (GridNodeIndicies[i][j] != -1)
					{
						GridNodes[GridNodeIndicies[i][j]].gridY = j;
						GridNodes[GridNodeIndicies[i][j]].gridX = i;
						GridNodes[GridNodeIndicies[i][j]].links = GetNeighbors(i, j);
					}
				}
			}

			// data array is now finished so we can now get the pointers
			for (int i = 0; i < gridNodeIndicies.size(); i++)
			{
				std::vector<GridNode*> lineNodes;
				for (int j = 0; j < gridNodeIndicies[i].size(); j++)
				{
					pointerCount++;
					if (gridNodeIndicies[i][j] == -1)
					{
						GridNode* ptr = NULL;
						ptr = GetClosestNodeLinearSearchGridCoords(i, j);
						lineNodes.push_back(ptr);
						nullpointers++;
					}
					else
					{
						lineNodes.push_back(&GridNodes[gridNodeIndicies[i][j]]);
						validPointers++;
					}
					//UE_LOG(LogTemp, Warning, TEXT("generating nodemap: %d %d"), i, j);
				}
				GridNodePointers.push_back(lineNodes);
			}

			


			//DEBUG
			for (int i = 0; i < GridNodePointers.size(); i++)
			{
				for (int j = 0; j < GridNodePointers[i].size(); j++)
				{
					pointerCount++;
					if (GridNodePointers[i][j] == NULL) continue;
					validPointers++;
					if (j == 23 && i == 20)
					{
						DrawDebugSphere(GetWorld(), FVector(GridNodePointers[i][j]->x, GridNodePointers[i][j]->y, GridNodePointers[i][j]->z), 50, 6, FColor(0, 255, 0), false, 100.0f);
						for (int k = 0; k < GridNodePointers[i][j]->links.size(); k++)
						{
							DrawDebugSphere(GetWorld(), FVector(GridNodePointers[i][j]->links[k]->x, GridNodePointers[i][j]->links[k]->y, GridNodePointers[i][j]->links[k]->z), 40, 16, FColor(0, 0, 255), false, 100.0f);
						}
						continue;
					}
					DrawDebugSphere(GetWorld(), FVector(GridNodePointers[i][j]->x, GridNodePointers[i][j]->y, GridNodePointers[i][j]->z), 50, 6, FColor(255, 0, 0), false, 100.0f);
				}
			}

			OpenNodes.push_back(GridNodeIndicies[10][20]);

		AsyncTask(ENamedThreads::GameThread, [this]()
		{
			UE_LOG(LogTemp, Display, TEXT("async stuff is done"));

			// cancel if game is exiting
			if (GetWorld() == nullptr)
			{
				UE_LOG(LogTemp, Display, TEXT("Seems like the game is quitting, cancelling async"));
				return;
			}

			ready = true;

		});
	});

	*/

	originX = (int)GetActorLocation().X;
	originY = (int)GetActorLocation().Y;
	originZ = GetActorLocation().Z;

	// init to null
	for (int i = -sizeX + originX; i < sizeX + originX; i += cellSize)
	{
		std::vector<int> lineIndicies;
		for (int j = -sizeY + originY; j < sizeY + originY; j += cellSize)
		{
			int index = -1; // -1 means save this as NULL in the pointer array
			lineIndicies.push_back(index);
			nodemapcount++;
		}
		GridNodeIndicies.push_back(lineIndicies);
	}
	
}

// Called every frame
void AGP4_GridManager::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (!globalGridManagerReference.IsValid())
	{
		globalGridManagerReference = this;
	}

	/*if (!ready)
	{
		GEngine->AddOnScreenDebugMessage(-1, 0.0f, FColor::Green, FString::Printf(TEXT("generating nodes: %d // %d"), pointerCount, GridNodes.size()));
		return;
	}*/

	/*testTimer += DeltaTime;
	if (testTimer > 0.1f)
	{
		FloodFillTest();
		testTimer = 0.0f;
	}

	for (int j = 0; j < OpenNodes.size(); j++)
	{
		GridNode* node = &GridNodes[OpenNodes[j]];
		DrawDebugSphere(GetWorld(), FVector(node->x, node->y, node->z), 50, 4, FColor(0, 255, 0));
	}
	for (int j = 0; j < ClosedNodes.size(); j++)
	{
		GridNode* node = &GridNodes[ClosedNodes[j]];
		DrawDebugSphere(GetWorld(), FVector(node->x, node->y, node->z), 40, 4, FColor(0, 0, 255));
	}*/

	/*for (int i = 0; i < GridNodeIndicies.size(); i++)
	{
		for (int j = 0; j < GridNodeIndicies[i].size(); j++)
		{
			if (GridNodeIndicies[i][j] != -1)
			{
				GridNode* node = GetNodePointer(GridNodeIndicies[i][j]);
				DrawDebugSphere(GetWorld(), FVector(node->x, node->y, node->z), 50, 4, FColor(0, (unsigned char)(node->gridY % 200), (unsigned char)(node->gridX % 200)));
			}
		}
	}*/


	/*GridNode* closeGN = GetClosestNodeFromWorldPos(GetActorLocation());
	if (closeGN != NULL)
	{
		DrawDebugSphere(GetWorld(), FVector(closeGN->x, closeGN->y, closeGN->z), 40, 4, FColor(0, 0, 255));
	}*/
	
	

	//DEBUG
	GEngine->AddOnScreenDebugMessage(-1, 0.0f, FColor::Green, FString::Printf(TEXT("checked: %d"), checked));
	GEngine->AddOnScreenDebugMessage(-1, 0.0f, FColor::Green, FString::Printf(TEXT("stored: %d"), stored));
	GEngine->AddOnScreenDebugMessage(-1, 0.0f, FColor::Green, FString::Printf(TEXT("skipped: %d"), skipped));
	GEngine->AddOnScreenDebugMessage(-1, 0.0f, FColor::Green, FString::Printf(TEXT("map size: %d"), nodemapcount));
	
}

bool AGP4_GridManager::CheckIfValidIndex(std::vector<std::vector<int>> *map, int x, int y, bool skipNullCheck)
{
	bool result = true;

	if (x < 0 || x >= map->size()) result = false;
	else if (y < 0 || y >= (*map)[x].size()) result = false;
	else if ((*map)[x][y] == -1 && !skipNullCheck) result = false;

	return result;
}

bool AGP4_GridManager::CheckIfValidIndex(std::vector<std::vector<IFieldNode>>* map, int x, int y)
{
	bool result = true;

	if (x < 0 || x >= map->size()) result = false;
	else if (y < 0 || y >= (*map)[x].size()) result = false;

	return result;
}

std::vector<int> AGP4_GridManager::GetNeighbors(std::vector<std::vector<int>> *map, int x, int y, bool skipNull)
{
	std::vector<int> checkNodes;

	if (CheckIfValidIndex(map, x + 1, y, skipNull)) checkNodes.push_back((*map)[x + 1][y]);
	if (CheckIfValidIndex(map, x - 1, y, skipNull)) checkNodes.push_back((*map)[x - 1][y]);
	if (CheckIfValidIndex(map, x, y + 1, skipNull)) checkNodes.push_back((*map)[x][y + 1]);
	if (CheckIfValidIndex(map, x, y - 1, skipNull)) checkNodes.push_back((*map)[x][y - 1]);

	if (CheckIfValidIndex(map, x + 1, y + 1, skipNull)) checkNodes.push_back((*map)[x + 1][y + 1]);
	if (CheckIfValidIndex(map, x - 1, y - 1, skipNull)) checkNodes.push_back((*map)[x - 1][y - 1]);
	if (CheckIfValidIndex(map, x + 1, y - 1, skipNull)) checkNodes.push_back((*map)[x + 1][y - 1]);
	if (CheckIfValidIndex(map, x - 1, y + 1, skipNull)) checkNodes.push_back((*map)[x - 1][y + 1]);

	return checkNodes;
}

std::vector<int> AGP4_GridManager::GetCardinalNeighbors(std::vector<std::vector<int>> *map, int x, int y, bool skipNull)
{
	std::vector<int> checkNodes;

	if (CheckIfValidIndex(map, x + 1, y, skipNull)) checkNodes.push_back((*map)[x + 1][y]);
	if (CheckIfValidIndex(map, x - 1, y, skipNull)) checkNodes.push_back((*map)[x - 1][y]);
	if (CheckIfValidIndex(map, x, y + 1, skipNull)) checkNodes.push_back((*map)[x][y + 1]);
	if (CheckIfValidIndex(map, x, y - 1, skipNull)) checkNodes.push_back((*map)[x][y - 1]);

	return checkNodes;
}

GridNode* AGP4_GridManager::GetClosestNodeFromWorldPos(FVector worldPosition)
{

	int x, y; // world coordinates to pointermap coordinates
	x = FMath::RoundToInt((-originX + sizeX + worldPosition.X) / cellSize);
	y = FMath::RoundToInt((-originY + sizeY + worldPosition.Y) / cellSize);

	if (CheckIfValidIndex(&GridNodeIndicies, x, y))
	{
		return GetNodePointer(x, y);
	}

	// no direct match, try to find one in range
	int adjX = x;
	int adjY = y;
	if (adjX < 0) adjX = 0;
	if (adjY < 0) adjY = 0;
	if (adjX > ((sizeX * 2) / cellSize) - 1) adjX = ((sizeX * 2) / cellSize) - 1;
	if (adjY > ((sizeY * 2) / cellSize) - 1) adjY = ((sizeY * 2) / cellSize) - 1;

	if (CheckIfValidIndex(&GridNodeIndicies, adjX, adjY))
	{
		return GetNodePointer(adjX, adjY);
	}

	// last resort, we instead do a distance search
	GridNode* testPtr = NULL;
	testPtr = GetClosestNodeLinearSearchGridCoords(x, y);
	if (testPtr == NULL) testPtr = GetClosestNodeLinearSearchWorldCoords(worldPosition.X, worldPosition.Y); // if grid space search fails for some reason do a world space one instead
	if (testPtr != NULL) return testPtr;
	
	GEngine->AddOnScreenDebugMessage(-1, 10.0f, FColor::Red, FString::Printf(TEXT("NODEGRAPH: SEARCH FAILED, MORE DETAILS IN LOG, CONTACT MAX F")));
	UE_LOG(LogTemp, Error, TEXT("NODEGRAPH ERROR: GetClosestNodeFromWorldPos in GridManager FAILED, CONTACT MAX F, NODES: %d, WORLD POS: %f %f , NODESPACE POS: %d %d"), GridNodes.size(), worldPosition.X, worldPosition.Y, x, y);

	// we failed
	return NULL;
}

IFieldNode* AGP4_GridManager::GetClosestNodeFromWorldPos(FVector worldPosition, FVector mapOrigin, std::vector<std::vector<IFieldNode>>* map, int radius)
{

	int x, y; // world coordinates to pointermap coordinates
	x = FMath::RoundToInt((-mapOrigin.X + radius + worldPosition.X) / cellSize);
	y = FMath::RoundToInt((-mapOrigin.Y + radius + worldPosition.Y) / cellSize);

	if (CheckIfValidIndex(map, x, y))
	{
		return &(*map)[x][y];
	}

	// no direct match, try to find one in range
	int adjX = x;
	int adjY = y;
	if (adjX < 0) adjX = 0;
	if (adjY < 0) adjY = 0;
	if (adjX > map->size() - 1) adjX = map->size() - 1;
	if (adjY > (*map)[0].size() - 1) adjY = (*map)[0].size() - 1;

	if (CheckIfValidIndex(map, adjX, adjY))
	{
		return &(*map)[adjX][adjY];
	}

	GEngine->AddOnScreenDebugMessage(-1, 10.0f, FColor::Red, FString::Printf(TEXT("NODEGRAPH: FLOWFIELD SEARCH FAILED, MORE DETAILS IN LOG, CONTACT MAX F")));
	UE_LOG(LogTemp, Error, TEXT("NODEGRAPH ERROR: GetClosestNodeFromWorldPos in GridManager FAILED, CONTACT MAX F, NODES: %d, WORLD POS: %f %f , NODESPACE POS: %d %d"), GridNodes.size(), worldPosition.X, worldPosition.Y, x, y);

	// we failed
	return NULL;
}

bool AGP4_GridManager::GetClosestNodeFromWorldPos(float ix, float iy, int* ox, int* oy, bool skipNull)
{
	int x, y; // world coordinates to pointermap coordinates
	x = FMath::RoundToInt((-originX + sizeX + ix) / cellSize);
	y = FMath::RoundToInt((-originY + sizeY + iy) / cellSize);

	if (CheckIfValidIndex(&GridNodeIndicies, x, y, skipNull))
	{
		*ox = x;
		*oy = y;
		return true;
	}

	// no direct match, try to find one in range
	int adjX = x;
	int adjY = y;
	if (adjX < 0) adjX = 0;
	if (adjY < 0) adjY = 0;
	if (adjX > ((sizeX * 2) / cellSize) - 1) adjX = ((sizeX * 2) / cellSize) - 1;
	if (adjY > ((sizeY * 2) / cellSize) - 1) adjY = ((sizeY * 2) / cellSize) - 1;

	if (CheckIfValidIndex(&GridNodeIndicies, adjX, adjY, skipNull))
	{
		*ox = adjX;
		*oy = adjY;
		return true;
	}

	// last resort, we instead do a distance search
	GridNode* testPtr = NULL;
	testPtr = GetClosestNodeLinearSearchGridCoords(x, y);
	if (testPtr == NULL) testPtr = GetClosestNodeLinearSearchWorldCoords(ix, iy); // if grid space search fails for some reason do a world space one instead
	if (testPtr != NULL)
	{
		if (CheckIfValidIndex(&GridNodeIndicies, testPtr->gridX, testPtr->gridY))
		{
			*ox = testPtr->gridX;
			*oy = testPtr->gridY;
			return true;
		}
	}

	GEngine->AddOnScreenDebugMessage(-1, 10.0f, FColor::Red, FString::Printf(TEXT("NODEGRAPH: INTERNAL SEARCH FAILED, MORE DETAILS IN LOG, CONTACT MAX F")));
	UE_LOG(LogTemp, Error, TEXT("NODEGRAPH ERROR: INTERNAL GetClosestNodeFromWorldPos in GridManager FAILED, CONTACT MAX F, NODES: %d, WORLD POS: %f %f , NODESPACE POS: %d %d"), GridNodes.size(), ix, iy, x, y);

	// we failed
	return false;
}


GridNode* AGP4_GridManager::GetClosestNodeLinearSearchWorldCoords(float x, float y)
{
	GridNode* closestMatch = NULL;
	float closestDistance = FLT_MAX;
	for (int i = 0; i < GridNodes.size(); i++)
	{
		float thisDistance = CalcGridDistancef(x, y, GridNodes[i].x, GridNodes[i].y);
		if (thisDistance < closestDistance)
		{
			closestDistance = thisDistance;
			closestMatch = &GridNodes[i];
		}
	}
	return closestMatch;
}

GridNode* AGP4_GridManager::GetClosestNodeLinearSearchGridCoords(int x, int y)
{
	GridNode* closestMatch = NULL;
	float closestDistance = FLT_MAX;
	for (int i = 0; i < GridNodes.size(); i++)
	{
		float thisDistance = CalcGridDistancei(x, y, GridNodes[i].gridX, GridNodes[i].gridY);
		if (thisDistance < closestDistance)
		{
			closestDistance = thisDistance;
			closestMatch = &GridNodes[i];
		}
	}
	return closestMatch;
}

void AGP4_GridManager::FloodFillTest()
{
	/*std::vector<int> frameNodes = OpenNodes;
	OpenNodes.clear();
	for (int k = 0; k < frameNodes.size(); k++)
	{
		GridNode* node = &GridNodes[frameNodes[k]];
		
		ClosedNodes.push_back(frameNodes[k]);

		for (int i = 0; i < node->links.size(); i++)
		{
			int nlink = node->links[i];
			bool ok = true;
			for (int j = 0; j < OpenNodes.size(); j++)
			{
				if (OpenNodes[j] == nlink) ok = false;
			}
			for (int j = 0; j < ClosedNodes.size(); j++)
			{
				if (ClosedNodes[j] == nlink) ok = false;
			}
			for (int j = 0; j < frameNodes.size(); j++)
			{
				if (frameNodes[j] == nlink) ok = false;
			}

			if (ok)
			{
				OpenNodes.push_back(nlink);
			}
		}

	}*/
}

float AGP4_GridManager::CalcGridDistancef(float srcX, float srcY, float dstX, float dstY)
{
	float dx = srcX - dstX;
	float dy = srcY - dstY;

	return sqrtf(dx * dx + dy * dy);
}

int AGP4_GridManager::CalcGridDistancei(int srcX, int srcY, int dstX, int dstY)
{
	int dx = srcX - dstX;
	int dy = srcY - dstY;
	
	return (dx * dx) + (dy * dy);
}

std::vector<std::vector<int>> AGP4_GridManager::GenerateCloseNodes(FVector origin, int radius)
{
	std::vector<std::vector<int>> localIndexMap;
	bool ok = true;

	int ox, oy;
	ox = (int)origin.X;
	oy = (int)origin.Y;

	int rad = radius / cellSize;

	int x, y; //origin coords
	if (!GetClosestNodeFromWorldPos(ox, oy, &x, &y, true)) ok = false;

	int bx, by; //beginning coords
	if (!GetClosestNodeFromWorldPos(ox - radius, oy - radius, &bx, &by, true)) ok = false;

	int ex, ey; //end coords
	if (!GetClosestNodeFromWorldPos(ox + radius, oy + radius, &ex, &ey, true)) ok = false;

	if (!ok)
	{
		GEngine->AddOnScreenDebugMessage(-1, 100.0f, FColor::Red, FString::Printf(TEXT("NODEGRAPH: DYNAMIC GENERATION FAILED, CONTACT MAX F")));
		return localIndexMap;
	}

	UE_LOG(LogTemp, Display, TEXT("ORIGIN: %d %d  BEGIN: %d %d  END: %d %d"), x, y, bx, by, ex, ey);

	for (int i = bx; i < ex; i++)
	{
		std::vector<int> lineIndicies;
		for (int j = by; j < ey; j++)
		{
			int idx, idy; // world coordinates to pointermap coordinates
			idx = (-originX + sizeX + i) / cellSize;
			idy = (-originY + sizeY + j) / cellSize;
			if (GridNodeIndicies[i][j] != -1)
			{
				GridNode* g = GetNodePointer(GridNodeIndicies[i][j]);
				FVector start = FVector(g->x, g->y, originZ);
				FVector end = start;
				end.Z += -1.0f * 50000.0f;
				g->type = NODE_BLOCKED;
				FCollisionQueryParams CollisionParams;
				FHitResult hit;
				bool rayCheck = GetWorld()->LineTraceSingleByChannel(hit, start, end, GroundCollisionChannel, CollisionParams);
				if (rayCheck)
				{
					rayCheck = hit.bBlockingHit;
					if (rayCheck)
					{
						g->z = hit.ImpactPoint.Z;
						g->type = NODE_FREE;
						rayCheck = GetWorld()->LineTraceSingleByChannel(hit, start, end, ObstacleCollisionChannel, CollisionParams);
						if (rayCheck)
						{
							rayCheck = hit.bBlockingHit;
							if (rayCheck)
							{
								g->type = NODE_BLOCKED;
							}
						}
					}
				}

				lineIndicies.push_back(GridNodeIndicies[i][j]);
				skipped++;
			}
			else
			{
				int index = -1; // -1 means save this as NULL in the pointer array
				GridNode g;
				g.x = (originX - sizeX + (i * cellSize));
				g.y = (originY - sizeY + (j * cellSize));
				g.type = NODE_BLOCKED;
				FVector start = FVector(g.x, g.y, originZ);
				FVector end = start;
				end.Z += -1.0f * 50000.0f;

				FCollisionQueryParams CollisionParams;
				FHitResult hit;
				bool rayCheck = GetWorld()->LineTraceSingleByChannel(hit, start, end, GroundCollisionChannel, CollisionParams);
				if (rayCheck)
				{
					rayCheck = hit.bBlockingHit;
					if (rayCheck)
					{
						g.z = hit.ImpactPoint.Z;
						g.type = NODE_FREE;
						rayCheck = GetWorld()->LineTraceSingleByChannel(hit, start, end, ObstacleCollisionChannel, CollisionParams);
						if (rayCheck)
						{
							rayCheck = hit.bBlockingHit;
							if (rayCheck)
							{
								g.type = NODE_BLOCKED;
							}
						}
						GridNodes.push_back(g);
						index = GridNodes.size() - 1;
						stored++;
					}
				}
				lineIndicies.push_back(index);
				GridNodeIndicies[i][j] = index;
				checked++;
			}

		}
		localIndexMap.push_back(lineIndicies);
	}

	// assign coords and neighbors to nodes
	for (int i = 0; i < GridNodeIndicies.size(); i++)
	{
		for (int j = 0; j < GridNodeIndicies[i].size(); j++)
		{
			if (GridNodeIndicies[i][j] != -1)
			{
				GridNodes[GridNodeIndicies[i][j]].gridY = j;
				GridNodes[GridNodeIndicies[i][j]].gridX = i;
				GridNodes[GridNodeIndicies[i][j]].links = GetNeighbors(&GridNodeIndicies, i, j);
			}
		}
	}


	return localIndexMap;
}

GridNode* AGP4_GridManager::GetNodePointer(int x, int y)
{
	AGP4_GridManager* gm = GetGridManager();
	if (gm == NULL) return NULL;
	return &gm->GridNodes[gm->GridNodeIndicies[x][y]];
}

GridNode* AGP4_GridManager::GetNodePointer(int x, int y, std::vector<std::vector<int>>* map)
{
	AGP4_GridManager* gm = GetGridManager();
	if (gm == NULL) return NULL;
	return &gm->GridNodes[(*map)[x][y]];
}

GridNode* AGP4_GridManager::GetNodePointer(int index)
{
	AGP4_GridManager* gm = GetGridManager();
	if (gm == NULL) return NULL;
	if (index < 0 || index > gm->GridNodes.size()) return NULL;
	return &gm->GridNodes[index];
}

AGP4_GridManager* AGP4_GridManager::GetGridManager()
{
	AGP4_GridManager* ptr = NULL;
	if (globalGridManagerReference.IsValid())
	{
		ptr = globalGridManagerReference.Get();
	}
	return ptr;
}


std::vector<std::vector<IFieldNode>> AGP4_GridManager::GenerateFlowField(std::vector<std::vector<int>> localMap, int targetX, int targetY)
{

	std::vector<std::vector<IFieldNode>> LocalINodeMap;
	std::vector<std::vector<int>> LocalINodeIndexMap;
	std::vector<IFieldNode> LocalINodes;

	//std::vector<IFieldNode> LocalINodes;
	{
		SCOPE_CYCLE_COUNTER(STAT_inodemapgen);

		for (int i = 0; i < localMap.size(); i++)
		{
			std::vector<int> mapLine;
			for (int j = 0; j < localMap[i].size(); j++)
			{
				IFieldNode fnode;
				fnode.gridNodeIndex = localMap[i][j];
				fnode.nextNodeIndex = -1;
				fnode.localX = i;
				fnode.localY = j;
				fnode.cost = 255;
				fnode.bestCost = USHORT_MAX;

				if (localMap[i][j] != -1)
				{
					GridNode* gn = GetNodePointer(localMap[i][j]);
					if (gn != NULL)
					{
						fnode.gridNodeIndex = localMap[i][j];
						if (gn->type == NODE_FREE) fnode.cost = 1;
						else if (gn->type == NODE_BREAKABLE) fnode.cost = 10;
					}
				}
				//mapLine.push_back(fnode);
				LocalINodes.push_back(fnode);
				int index = LocalINodes.size() - 1;
				mapLine.push_back(index);
			}
			LocalINodeIndexMap.push_back(mapLine);
		}

	}

	std::vector<IFieldNode*> OpenNodes;
	std::vector<IFieldNode*> ClosedNodes;
	OpenNodes.push_back(&LocalINodes[LocalINodeIndexMap[targetX][targetY]]);
	LocalINodes[LocalINodeIndexMap[targetX][targetY]].cost = 0;
	LocalINodes[LocalINodeIndexMap[targetX][targetY]].bestCost = 0;
	
	{
		SCOPE_CYCLE_COUNTER(STAT_inodecalcvalues);

		while (OpenNodes.size() > 0)
		{
			IFieldNode* currentFnode = OpenNodes.back();
			ClosedNodes.push_back(OpenNodes.back());
			OpenNodes.pop_back();

			std::vector<int> localLinks;
			localLinks = GetCardinalNeighbors(&LocalINodeIndexMap, currentFnode->localX, currentFnode->localY, true);

			for (int i = 0; i < localLinks.size(); i++)
			{
				int nlink = localLinks[i];
				IFieldNode* currentLink = &LocalINodes[nlink];
				bool ok = true;
				int x = 0;
				int y = 0;

				/*for (int j = 0; j < localMap.size() && !ok; j++)
				{
					for (int k = 0; k < localMap[j].size() && !ok; k++)
					{
						if (LocalINodeMap[j][k].gridNodeIndex == nlink)
						{
							ok = true;
							x = j;
							y = k;
						}
					}
				}*/
				for (int j = 0; j < OpenNodes.size() && ok; j++)
				{
					if (OpenNodes[j]->gridNodeIndex == currentLink->gridNodeIndex) ok = false;
				}

				/*for (int j = 0; j < ClosedNodes.size() && init; j++)
				{
					if (ClosedNodes[j]->gridNodeIndex == nlink) init = false;
				}*/

				if (ok)
				{
					if (currentLink->cost + currentFnode->bestCost < currentLink->bestCost)
					{
						currentLink->bestCost = (unsigned short)(currentLink->cost + currentFnode->bestCost);
						OpenNodes.push_back(currentLink);
					}
				}

			}

		}

	}

	{
		SCOPE_CYCLE_COUNTER(STAT_inodecalcscore);

		while (ClosedNodes.size() > 0)
		{
			IFieldNode* currentFnode = ClosedNodes.back();
			GridNode* node = GetNodePointer(currentFnode->gridNodeIndex);
			ClosedNodes.pop_back();

			unsigned short bestCost = currentFnode->bestCost;
			std::vector<int> localLinks;
			localLinks = GetNeighbors(&LocalINodeIndexMap, currentFnode->localX, currentFnode->localY, true);

			if (node == NULL) continue;

			for (int i = 0; i < localLinks.size(); i++)
			{

				IFieldNode* currentNeighbor = &LocalINodes[localLinks[i]];

				if (currentNeighbor->bestCost < bestCost)
				{
					bestCost = currentNeighbor->bestCost;
					currentFnode->nextNodeIndex = currentNeighbor->gridNodeIndex;
				}
			}

		}

	}

	for (int i = 0; i < LocalINodeIndexMap.size(); i++)
	{
		std::vector<IFieldNode> lineNodes;
		for (int j = 0; j < LocalINodeIndexMap[i].size(); j++)
		{
			lineNodes.push_back(LocalINodes[LocalINodeIndexMap[i][j]]);
		}
		LocalINodeMap.push_back(lineNodes);
	}

	{
		SCOPE_CYCLE_COUNTER(STAT_inodecalcvectors);
		for (int i = 0; i < LocalINodeMap.size(); i++)
		{
			for (int j = 0; j < LocalINodeMap[i].size(); j++)
			{

				IFieldNode* fnode = &LocalINodeMap[i][j];
				if (fnode->gridNodeIndex != -1 && fnode->nextNodeIndex != -1)
				{
					GridNode* currentNode = GetNodePointer(fnode->gridNodeIndex);
					GridNode* nextNode = GetNodePointer(fnode->nextNodeIndex);

					FVector currentVector = FVector(currentNode->x, currentNode->y, currentNode->z);
					FVector nextVector = FVector(nextNode->x, nextNode->y, nextNode->z);

					FVector resultVector = nextVector - currentVector;

					resultVector.Normalize();

					fnode->vector = resultVector;
				}
			}
		}
	}


	//GetNodePointer(localMap[targetX][targetY])->iNodeMap = LocalINodeMap;
	return LocalINodeMap;
}