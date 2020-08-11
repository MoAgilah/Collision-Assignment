#ifndef BallManagerH
#define BallManagerH

#include "Application.h"
#include <vector>

#define MinNumBalls 10
#define MaxNumBalls 100

class Ball;
__declspec(align(16)) class BallManager
{
public:
	BallManager(HeightMap* mapData);
	~BallManager();
	//updating all active balls
	void UpdateBalls(float dt);
	//draw them balls
	void DrawBalls(Application* pApp);
	//get the ball by id
	Ball* GetById(int id);
	//initialisation of a new ball
	void InitBall(XMFLOAT3 iPos = XMFLOAT3(0, 0, 0), XMFLOAT3 iVel = XMFLOAT3(0, 0, 0), XMFLOAT3 iGrav = XMFLOAT3(0, 0, 0));
	//update world of all ball's
	void UpdateWorlds();
	//check for balls beyond screen space
	void CheckForOutOfBounds();
	//returns active ball for cycling through center and corners
	Ball* GetDebugBall();
	//update the debug ball
	void InitDebugBall(XMFLOAT3 iPos, XMFLOAT3 iVel = XMFLOAT3(0, 0, 0), XMFLOAT3 iGrav = XMFLOAT3(0, 0, 0));
	//double drop
	void DoubleDrop(XMFLOAT3 iPos, XMFLOAT3 iVel = XMFLOAT3(0, 0, 0), XMFLOAT3 iGrav = XMFLOAT3(0, 0, 0));
	//increase the number of balls in the pool
	void IncreaseNumOfBalls();
	//decrease the number of balls in the pool
	void DecreaseNumOfBalls();
	//drop all balls in the pool at random at the same time
	void DropAll();
	//clears all active balls
	void ClearAll();
	//update all spheres previous position
	void UpdatePrevious();
	//sweep and prune on the x dimension
	void OneDimensionalSweepAndPrune(int& numOfTests);
	//a check to see if the spheres chosen position if occupied
	bool OccupiedPosition(XMFLOAT3 pos, int index);
	int GetNumberOfBalls() { return numOfBall; }
private:
	static unsigned int numOfBall;		//a variable defining the number of ball's to be dropped
	CommonMesh *m_pMesh;				//shared mesh for all balls
	HeightMap* map;						//a pointer to the heightmap
	Ball* firstAvailable;				//currently the first available 
	Ball* DebugBall;					//the cyclic debug ball
	std::vector<Ball*> activeBalls;		//vector of active balls
public:
	void* operator new(size_t i)
	{
		return _mm_malloc(i, 16);
	}

	void operator delete(void* p)
	{
		_mm_free(p);
	}
};

#endif
