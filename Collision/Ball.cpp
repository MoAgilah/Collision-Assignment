#include "Ball.h"

//constructors
Ball::Ball(float r, XMFLOAT3 pos)
{
	//sphere with no mesh to allow robust poly intersection easier
	collided = enabled = false;

	mass = 1.0f;

	radius = r;
	mPosition = pos;

	mVelocity = XMFLOAT3(0.0f, 0.0f, 0.0f);
	mGravity = XMFLOAT3(0.0f, 0.0f, 0.0f);

	worldMtx = XMMatrixIdentity();
}

Ball::Ball(CommonMesh *sphere,int id, XMFLOAT3 pos)
{
	//all balls same sphere mesh given to them by ball mgr
	m_pMesh = sphere;

	collided = enabled = false;

	ID = id;

	radius = mass = 1.0f;

	mPosition = XMFLOAT3(pos);
	mVelocity = XMFLOAT3(0.0f, 0.0f, 0.0f);
	mGravity = XMFLOAT3(0.0f, 0.0f, 0.0f);

	worldMtx = XMMatrixIdentity();
}


Ball::~Ball()
{
	if (m_pMesh)
		m_pMesh = nullptr;
}
//----------------------------------------------------------------------------

void Ball::Update(float dt)
{
	SetWorld();
}

void Ball::Draw()
{
	if (enabled)
	{
		m_pMesh->Draw();
	}
	
}

//accessors and mutators
bool Ball::GetSphereCollided()
{
	return collided;
}

void Ball::SetSphereCollided(bool val)
{
	collided = val;
}

bool Ball::GetEnabled()
{
	return enabled;
}

void Ball::SetEnabled(bool val)
{
	enabled = val;
}

XMFLOAT3 Ball::GetPosition()
{
	return mPosition;
}

void Ball::SetPosition(XMFLOAT3 pos)
{
	mPosition = pos;
}

void Ball::SetPosition(XMVECTOR pos)
{
	XMFLOAT3 p;
	XMStoreFloat3(&p, pos);
	mPosition = p;
}

XMFLOAT3 Ball::GetPrevPosition()
{
	return mPrevPos;
}

void Ball::SetPrevPosition(XMFLOAT3 pos)
{
	mPrevPos = pos;
}

void Ball::SetPrevPosition(XMVECTOR pos)
{
	XMFLOAT3 p;
	XMStoreFloat3(&p, pos);
	mPrevPos = p;
}

XMFLOAT3 Ball::GetPrevVelocity()
{
	return mPrevVel;
}

void Ball::SetPrevVelocity(XMFLOAT3 vel)
{
	mPrevVel = vel;
}

void Ball::SetPrevVelocity(XMVECTOR vel)
{
	XMFLOAT3 v;
	XMStoreFloat3(&v, vel);
	mPrevVel = v;
}

XMFLOAT3 Ball::GetVelocity()
{
	return mVelocity;
}

void Ball::SetVelocity(XMFLOAT3 vel)
{
	mVelocity = vel;
}

void Ball::SetVelocity(XMVECTOR vel)
{
	XMFLOAT3 v;
	XMStoreFloat3(&v, vel);
	mVelocity = v;
}

float Ball::GetSpeed()
{
	return mSpeed;
}

void Ball::SetSpeed(float spd)
{
	mSpeed = spd;
}

XMFLOAT3 Ball::GetGravity()
{
	return mGravity;
}

void Ball::SetGravity(XMFLOAT3 grav)
{
	mGravity = grav;
}

void Ball::SetGravity(XMVECTOR vel)
{
	XMFLOAT3 g;
	XMStoreFloat3(&g, vel);
	mGravity = g;
}

XMMATRIX Ball::GetWorld()
{
	return worldMtx;
}

void Ball::SetWorld()
{
	worldMtx = XMMatrixTranslation(mPosition.x, mPosition.y, mPosition.z);
}

float Ball::GetRadius()
{
	return radius;
}

void Ball::SetRadius(float r)
{
	radius = r;
}

float Ball::GetMass()
{
	return mass;
}

void Ball::SetMass(float m)
{
	mass = m;
}

int Ball::GetID()
{
	return ID;
}
//-------------------------------------------------------

