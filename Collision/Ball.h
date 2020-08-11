#ifndef BallH
#define BallH

#include "Application.h"
#include "CommonMesh.h"

__declspec(align(16)) class Ball
{
public:
	bool moving;
	Ball(float radius, XMFLOAT3 pos);
	Ball(CommonMesh *sphere,int id,XMFLOAT3 pos = XMFLOAT3(0, 0, 0));
	~Ball();
	void Update(float deltaTime);
	void Draw();

	//accessor for the individual balls sphere's collided variable
	bool GetSphereCollided();
	void SetSphereCollided(bool val);

	bool GetEnabled();
	void SetEnabled(bool val);

	//accessor for the individual sphere's position
	XMFLOAT3 GetPosition();
	void SetPosition(XMFLOAT3 pos);
	void SetPosition(XMVECTOR pos);

	//accessor for the individual sphere's previous position
	XMFLOAT3 GetPrevPosition();
	void SetPrevPosition(XMFLOAT3 pos);
	void SetPrevPosition(XMVECTOR pos);

	//accessor for the individual sphere's velocity
	XMFLOAT3 GetVelocity();
	void SetVelocity(XMFLOAT3 vel);
	void SetVelocity(XMVECTOR vel);

	//accessor for the individual sphere's previous position
	XMFLOAT3 GetPrevVelocity();
	void SetPrevVelocity(XMFLOAT3 pos);
	void SetPrevVelocity(XMVECTOR pos);

	//accessor for the individual sphere's gravity
	XMFLOAT3 GetGravity();
	void SetGravity(XMFLOAT3 grav);
	void SetGravity(XMVECTOR grav);
	
	//accessor for the individual ball
	XMMATRIX GetWorld();
	void SetWorld();

	//accessor for the individual balls speed
	float GetSpeed();
	void SetSpeed(float speed);

	//accessor for the individual balls radius
	float GetRadius();
	void SetRadius(float r);

	//accessor for the individual balls mass
	float GetMass();
	void SetMass(float m);

	//accessor for the id no mutator
	int GetID();
private:
	bool collided;
	bool enabled;

	int ID;

	float mSpeed;
	float radius;
	float mass;

	XMFLOAT3 mPosition;	
	XMFLOAT3 mPrevPos;	
	XMFLOAT3 mVelocity;	
	XMFLOAT3 mPrevVel;
	XMFLOAT3 mGravity;

	XMMATRIX worldMtx;

	CommonMesh *m_pMesh;
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
 