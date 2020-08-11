#ifndef CollisionsH
#define CollisionsH

#include "Application.h"
#include <algorithm>
#include <numeric>

class Ball;
struct FaceCollisionData;
__declspec(align(16)) class Collisions
{
public:
	Collisions();
	~Collisions();
	//return collision singletion
	static Collisions* GetCol(); 
	//get if the point is behind or in front of the plane
	bool PointPlane(const XMVECTOR& rayPos, const XMVECTOR& vert1, const XMVECTOR& vert2, const XMVECTOR& pointPos);
	//dynamic to dynamic sphere col test to dynamic to static sphere col
	bool TestMovingSphereToSphere(Ball* b0, Ball* b1, float &t);
	//resolve intersection by lerpfactor that defines time of collision
	void ResolveIntersection(Ball* b, float lerpFactor);
	//if there is a collision between two ball's resolve it
	void ResolveSphereCollisions(Ball* b0, Ball* b1);
	//intersect ray to sphere collision check
	bool IntersectRaySphere(XMFLOAT3 p, XMVECTOR d, Ball* s, XMVECTOR& colPos, float &t);
	//intersect ray to cyclinder collision check
	bool IntersectRayCyclinder(Ball* b0, XMFLOAT3 startP, XMFLOAT3 endP, float& t);
	//intersect ray to triangle collision check
	bool IntersectRayTriangle(FaceCollisionData fcd, Ball* b, XMVECTOR& colPos, XMVECTOR& colNormN, float& t);
	//creates test primitives for individual faces of the heightmap
	bool TestSpherePolyIntersection(Ball* b,FaceCollisionData fcd,float& t);
private:
	static Collisions* self;
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

