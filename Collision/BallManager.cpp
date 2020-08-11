#include "BallManager.h"
#include "Ball.h"
#include "HeightMap.h"
#include "Collisions.h"
#include "Utils.h"

unsigned int BallManager::numOfBall = MinNumBalls;

BallManager::BallManager(HeightMap* mapData)
	:map(mapData)
{
	//create single mesh instance
	m_pMesh = CommonMesh::NewSphereMesh(Application::s_pApp, 1.f, 16, 16);

	//create debug ball
	DebugBall = new Ball(m_pMesh,0);
	//put it in the vector
	activeBalls.push_back(DebugBall);

	//create the mininum of balls
	for (int i = 1; i < MaxNumBalls; i++)
		activeBalls.push_back(new Ball(m_pMesh, i));



	//assign the first non debug ball as first available
	firstAvailable = activeBalls[1];
}


void BallManager::IncreaseNumOfBalls()
{
	//increment number of available balls providing it doesnt exceed maximum
	if (++numOfBall > MaxNumBalls)
		numOfBall = MaxNumBalls;
}

void BallManager::DecreaseNumOfBalls()
{
	//increment number of available balls providing it doesnt go below minimum
	if (--numOfBall < MinNumBalls)
		numOfBall = MinNumBalls;
}

BallManager::~BallManager()
{
	map = nullptr;	//null reference to height map
	for (unsigned int i = 0; i < activeBalls.size(); i++)
	{
		//deallocate pointer in array
		delete activeBalls[i];	
		activeBalls[i] = nullptr;
	}
	delete m_pMesh;	//deallocate sphere mesh
}

void BallManager::UpdateWorlds()
{
	//update all worlds for active balls
	for (unsigned int i = 0; i < activeBalls.size(); i++)
	{
		Ball* ball = GetById(i);
		if (ball->GetEnabled()) ball->SetWorld();
	}
}

void BallManager::InitBall(XMFLOAT3 iPos, XMFLOAT3 iVel, XMFLOAT3 iGrav)
{
	Ball* curr = firstAvailable;	//capture the first available in a temp var

	for (unsigned int i = 1; i < numOfBall; ++i)
	{
		Ball* next = GetById(i);
		//if activeball is the first available 
		if (next == firstAvailable) continue;	//continue
		
		//the ball isn't active
		if (!next->GetEnabled())
		{
			//make it the first available
			firstAvailable = next;
			//if found break out of loop
			break;
		}
	}
	
	//when it gets to the limit it begins to reuse the first ball in the available pool
	if (curr == firstAvailable) firstAvailable = GetById(0);

	//init the temp ball and then activate it 
	curr->SetPosition(iPos);
	curr->SetVelocity(iVel);
	curr->SetGravity(iGrav);
	curr->SetEnabled(true);
	curr->SetSphereCollided(false);
	curr->moving = true;
}

Ball* BallManager::GetById(int id)
{
	//return the ball by it's id in case of sorting putting things out of order
	for (unsigned int i = 0; i < activeBalls.size(); i++)
	{
		if (activeBalls[i]->GetID() == id)
			return activeBalls[i];
	}

	//or returns a nullptr
	return nullptr;
}

void BallManager::InitDebugBall(XMFLOAT3 iPos, XMFLOAT3 iVel, XMFLOAT3 iGrav)
{
	//init the debug ball and activate it
	DebugBall->SetPosition(iPos);
	DebugBall->SetVelocity(iVel);
	DebugBall->SetGravity(iGrav);
	DebugBall->SetEnabled(true);
	DebugBall->SetSphereCollided(false);
}

void BallManager::UpdateBalls(float dt)
{
	//Update Sphere
	XMVECTOR vSColPos, vSColNorm;	//variable to return the col position and collision normal
	float damping = 0.8f;

	int numOfTests = 0;	//num of tests performed
	int testnumofhits = 0;	//how many spheres to be tested

	//update the last frame's position and velocity data
	UpdatePrevious();
	
	for (unsigned int i = 0; i < activeBalls.size(); ++i)
	{
		//get ball by it's
		Ball* ball = GetById(i);
		//if it's enabled
		if (ball->GetEnabled())
		{
			//if it's moving
			if (ball->moving)
			{
				//increment the number of balls to be tested - primarily debugging purposes
				testnumofhits++;

				//if sphere hasn't collided
				if (!ball->GetSphereCollided())
				{
					//extract the information from the ball
					XMVECTOR vSPos = XMLoadFloat3(&ball->GetPosition());
					XMVECTOR vSVel = XMLoadFloat3(&ball->GetVelocity());
					XMVECTOR vSAcc = XMLoadFloat3(&ball->GetGravity());

					//update the values
					vSPos += vSVel * dt; //Really important that we add LAST FRAME'S velocity as this was how fast the collision is expecting the ball to move
					vSVel += vSAcc * dt; //The new velocity gets passed through to the collision so it can base its predictions on our speed NEXT FRAME

					//set the value ready to process collision data
					ball->SetVelocity(vSVel);
					ball->SetPosition(vSPos);
					ball->SetSpeed(XMVectorGetX(XMVector3Length(vSVel)));


					//check for collisions with the heightmap
					bool collided = map->RayCollision(ball, vSColPos, vSColNorm);

					//pass collided to the ball
					ball->SetSphereCollided(collided);

					//if ball has collided
					if (ball->GetSphereCollided())
					{
						//get  velocity from the ball
						const XMVECTOR V = XMLoadFloat3(&ball->GetVelocity());

						//V.N 
						float dotVN = XMVectorGetX(XMVector3Dot(V, vSColNorm));

						//R = V - 2N(V.N)
						XMVECTOR R = (V - ((2 * vSColNorm)*dotVN))*damping;

						//here for bouncy bouncy
						ball->SetVelocity(R);
						ball->SetSphereCollided(false);

						//this is my initial attempt to do natural rest
							//i felt the code without this in had a better look so i commented it out 
						/*if (ball->GetSpeed() < 0.05f)
						{
							ball->moving = false;
						}*/
					}
				}
			}
		}
	}

	//1-Dimensional Sweep and prune
	OneDimensionalSweepAndPrune(numOfTests);

	//outputs number of tests 
	char text[256];
	sprintf_s(text, "'num of tests =%i\n", numOfTests);
	OutputDebugStringA(text);

	//check the spheres aren't out of bounds
	CheckForOutOfBounds();
}

void BallManager::DrawBalls(Application* pApp)
{
	for (unsigned int i = 0; i < activeBalls.size(); ++i)
	{
		//if enabled
		if (activeBalls[i]->GetEnabled())
		{
			//set the world matrix
			pApp->SetWorldMatrix(activeBalls[i]->GetWorld());
			//draw the ball
			activeBalls[i]->Draw();
		}
	}
}

void BallManager::CheckForOutOfBounds()
{
	for (unsigned int i = 0; i < activeBalls.size(); ++i)
	{
		if (activeBalls[i]->GetEnabled())
		{
			//if below the height map
			if (activeBalls[i]->GetPosition().y <= -10.f)
			{
				//return to the pool
				activeBalls[i]->SetEnabled(false);
			}
		}
	}
}

void BallManager::ClearAll()
{
	//disactivate all of the balls
	for (unsigned int i = 0; i < activeBalls.size(); ++i)
	{
		activeBalls[i]->SetEnabled(false);
	}
}

Ball* BallManager::GetDebugBall()
{
	return DebugBall;
}

void BallManager::DropAll()
{
	//clear the pool
	ClearAll();

	//for all expect the debug ball
	for (unsigned int i = 1; i < numOfBall; ++i)
	{
		//randomly get a position
		XMFLOAT3 pos = XMFLOAT3((float)((rand() % 14 - 7.0f) - 0.5), (float)(rand() % 21 + 20), (float)((rand() % 14 - 7.0f) - 0.5));

		//check if initially overlapping, if so keep calling until
		while (OccupiedPosition(pos,i))
		{
			//a new unoccupied position is found
			pos = XMFLOAT3((float)((rand() % 14 - 7.0f) - 0.5), (float)(rand() % 21 + 20), (float)((rand() % 14 - 7.0f) - 0.5));
		}

		//initialise the ball with it's new found pos
		InitBall(pos, XMFLOAT3(0.0f, 0.2f, 0.0f), XMFLOAT3(0.0f, -0.05f, 0.0f));
	}
}

void BallManager::UpdatePrevious()
{
	//every frame capture the previous velocity and position 
	for (unsigned int i = 0; i < activeBalls.size(); i++)
	{
		activeBalls[i]->SetPrevPosition(activeBalls[i]->GetPosition());
		activeBalls[i]->SetPrevVelocity(activeBalls[i]->GetVelocity());
	}
}


void BallManager::OneDimensionalSweepAndPrune(int& numOfTests)
{
	//use insertion sort to reduce the overhead of sorting, and achieve temporal coherence
	insertionSort(activeBalls);

	for (unsigned int i = 0; i < activeBalls.size(); ++i)
	{
		if (!activeBalls[i]->GetEnabled()) continue;	//if enabled

		for (unsigned int j = i + 1; j < activeBalls.size(); ++j)
		{
			if (!activeBalls[j]->GetEnabled()) continue; //if enabled

			//if(ball1.xmax < ball2.xmin 
			if (activeBalls[i]->GetPosition().x + activeBalls[i]->GetRadius() < activeBalls[j]->GetPosition().x - activeBalls[j]->GetRadius())
				break;

			//check other axes
			//if ball1.maxY < balll2.minY  || ball2.maxY < balll1.minY
			if (activeBalls[i]->GetPosition().y + activeBalls[i]->GetRadius() < activeBalls[j]->GetPosition().y - activeBalls[j]->GetRadius() ||
				activeBalls[j]->GetPosition().y + activeBalls[j]->GetRadius() < activeBalls[i]->GetPosition().y - activeBalls[i]->GetRadius())
				continue;

			//if ball1.maxZ < balll2.minZ || ball2.maxZ < balll1.minZ
			if (activeBalls[i]->GetPosition().z + activeBalls[i]->GetRadius() < activeBalls[j]->GetPosition().z - activeBalls[j]->GetRadius() ||
				activeBalls[j]->GetPosition().z + activeBalls[j]->GetRadius() < activeBalls[i]->GetPosition().z - activeBalls[i]->GetRadius())
				continue;	
			
 			numOfTests++;	//if past all early out test has potential collision required increment debug counter
			float t = 0;	//initialie colDist
			
			if (Collisions::GetCol()->TestMovingSphereToSphere(activeBalls[i], activeBalls[j], t))
			{
				//again this is initial code for reactivating the ball's on rest but decided to remove it during to it looking worse that what is presented
				/*
				if (!activeBalls[i]->moving) activeBalls[i]->moving = true;

				if (!activeBalls[j]->moving) activeBalls[j]->moving = true;
				*/

				//resolve the intersection of the first ball
				Collisions::GetCol()->ResolveIntersection(activeBalls[i], t);

				//resolve the intersection of the second ball
				Collisions::GetCol()->ResolveIntersection(activeBalls[j], t);

				//resolve the collisions and assing new velocities to simulate sphere colliding accurately
				Collisions::GetCol()->ResolveSphereCollisions(activeBalls[i], activeBalls[j]);
			}
		}
	}
}

bool BallManager::OccupiedPosition(XMFLOAT3 pos, int index)
{
	for (int i = 1; i < activeBalls.size(); ++i)
	{
		//if the index matches then skip this test
		if (i == index)
			continue;

		//obtaine position for value
		XMFLOAT3 sC = activeBalls[i]->GetPosition();
		float r = activeBalls[i]->GetRadius();

		//test to see if the position is overlapping with the current sphere 
		if (pos.x >= sC.x - r && pos.x <= sC.x + r &&
			pos.y >= sC.y - r && pos.y <= sC.y + r &&
			pos.z >= sC.z - r && pos.z <= sC.z + r)
		{
			//if so tell the spawning
			return true;
		}
	}

	//if not accept current position for instantiation 
	return false;
}

