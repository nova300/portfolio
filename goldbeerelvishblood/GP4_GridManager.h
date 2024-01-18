#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GP4_GridManager.generated.h"

struct GridNode;

enum NodeType
{
	NODE_FREE,			//default walkable node
	NODE_BLOCKED,		//blocked by enviroment, will never be walkable
	NODE_BREAKABLE		//blocked by dynamic obj, can be walkable in the future
};

struct IFieldNode	//integration field node
{
	int gridNodeIndex;
	int nextNodeIndex;	//used to check if node is still linked to base node
	int localX;
	int localY;
	unsigned char cost;
	unsigned short bestCost;
	FVector vector;
};

struct GridNode		//base node
{
	int x;
	int y;
	unsigned short gridX;
	unsigned short gridY;
	float z;
	float pressure;
	NodeType type;
	std::vector<int> links;
	std::vector<std::vector<IFieldNode>> iNodeMap;
};

UCLASS()
class AGP4_GridManager : public AActor
{
	GENERATED_BODY()

	float originZ;
	int originX;
	int originY;
	
public:	
	AGP4_GridManager();

	UPROPERTY(EditAnywhere)
		int sizeX;

	UPROPERTY(EditAnywhere)
		int sizeY;

	UPROPERTY(EditAnywhere)
		int cellSize;	
	
	UPROPERTY(EditAnywhere)
		TEnumAsByte<ECollisionChannel> GroundCollisionChannel;

	UPROPERTY(EditAnywhere)
		TEnumAsByte<ECollisionChannel> ObstacleCollisionChannel;

protected:
	virtual void BeginPlay() override;

public:	
	virtual void Tick(float DeltaTime) override;

	GridNode* GetClosestNodeFromWorldPos(FVector worldPosition);
	IFieldNode* GetClosestNodeFromWorldPos(FVector worldPosition, FVector mapOrigin, std::vector<std::vector<IFieldNode>>* map, int radius);
	bool GetClosestNodeFromWorldPos(float ix, float iy, int *ox, int* oy, bool skipNull = false);

	std::vector<std::vector<int>> GenerateCloseNodes(FVector origin, int radius);
	std::vector<std::vector<IFieldNode>> GenerateFlowField(std::vector<std::vector<int>> localMap, int targetX, int targetY);

	static GridNode* GetNodePointer(int x, int y);
	static GridNode* GetNodePointer(int x, int y, std::vector<std::vector<int>>* map);
	static GridNode* GetNodePointer(int index);

	GridNode* GetNodeChecked(int index);

	static float CalcGridDistancef(float srcX, float srcY, float dstX, float dstY);
	static int CalcGridDistancei(int srcX, int srcY, int dstX, int dstY);

	static AGP4_GridManager* GetGridManager();

private:

	std::vector<GridNode> GridNodes;
	std::vector<std::vector<int>> GridNodeIndicies;

	static TWeakObjectPtr<AGP4_GridManager> globalGridManagerReference;

	bool ready;

	//DEBUG
	int stored;
	int skipped;
	int nodemapcount;
	int checked;
	float testTimer;

	std::vector<int> GetNeighbors(std::vector<std::vector<int>> *map, int x, int y, bool skipNull = false);
	std::vector<int> GetCardinalNeighbors(std::vector<std::vector<int>> *map, int x, int y, bool skipNull = false);
	bool CheckIfValidIndex(std::vector<std::vector<int>> *map, int x, int y, bool skipNullCheck = false);
	bool CheckIfValidIndex(std::vector<std::vector<IFieldNode>>* map, int x, int y);

	GridNode* GetClosestNodeLinearSearchGridCoords(int x, int y);
	GridNode* GetClosestNodeLinearSearchWorldCoords(float x, float y);


	void FloodFillTest();

};
