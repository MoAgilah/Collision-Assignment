#include "Collisions.h"
#include "Ball.h"
#include "Utils.h"

Collisions* Collisions::self = nullptr;

Collisions::Collisions()
{
	//only allow one instance to be made
	if (self == nullptr)
	{
		self = this;
	}
}

Collisions::~Collisions()
{
	self = nullptr;
}

Collisions* Collisions::GetCol()
{
	return self;
}

// Intersect sphere s0 moving in direction d over time interval t0 <= t <= t1, against
// a stationary sphere s1. If found intersecting, return time(t) of collision
bool Collisions::TestMovingSphereToSphere(Ball* b0, Ball* b1, float &colDist)
{
	//make a copy to create expanded sphere for collision
	Ball onstack(*b1);

	//expand sphere b1 by the radius of b0
	onstack.SetRadius(b0->GetRadius() + b1->GetRadius());

	XMVECTOR vel0, vel1, V;
	vel0 = XMLoadFloat3(&b0->GetVelocity());
	vel1 = XMLoadFloat3(&b1->GetVelocity());

	//subtract the velocity of s1 from both s0 and s1, making s1 stationary
	V = vel0 - vel1;
	float vLen = XMVectorGetX(XMVector3Length(V));

	//can now test directed segment s = s0.c + tv, v = (v0-v1)/||v0-v1|| against the expanded sphere for intersection
	XMVECTOR colPos = XMVECTOR();	//rayPoint

	if (IntersectRaySphere(b0->GetPosition(), V / vLen, &onstack, colPos, colDist))
	{
		return colDist <= vLen;
	}

	return false;
}
// Intersects ray r = p + td, |d| = 1, with sphere s and, if intersecting,
// returns t value of intersection and intersection point q
bool Collisions::IntersectRaySphere(XMFLOAT3 rayPos, XMVECTOR dir, Ball* s, XMVECTOR& colPos,float &colDist)
{
	XMVECTOR point(XMLoadFloat3(&rayPos)), sCenter(XMLoadFloat3(&s->GetPosition()));

	// Calculate ray start's offset from the sphere center i.e. point - sphereCenter
	XMVECTOR sphereToPoint = point - sCenter;

	float b = XMVectorGetX(XMVector3Dot(sphereToPoint, dir));

	float c = XMVectorGetX(XMVector3Dot(sphereToPoint, sphereToPoint)) - (s->GetRadius() * s->GetRadius());

	//exit if ray's origin is outside s(c > 0) and ray is pointing away from s(b > 0)
	if (c > 0.0f && b > 0.0f) return false;

	float discR = (b * b) - c;

	//a negative discriminant means the ray missed the sphere
	if (discR < 0.0f) return false;

	//ray now found to intersect the sphere, compute the smallest t value of intersection
	colDist = -b - sqrtf(discR);

	//if colDist is negative 
	if (colDist < 0.0f) colDist = 0.0f;	//ray started inside sphere so clamp colDist to zero

	//calculate the col position 
	colPos = point + colDist * dir;

	return true;
}

void Collisions::ResolveIntersection(Ball* b, float lerp)
{
	//acquire the current and previous position
	XMVECTOR pos(XMLoadFloat3(&b->GetPosition())), pPos(XMLoadFloat3(&b->GetPrevPosition()));
	//lerp the position back to the point just as the collision takes place defined by t or col dist
	XMVECTOR resPoint = XMVectorLerp(pPos, pos, lerp);
	//set the correct position
	b->SetPosition(resPoint);

	//acquire the velocity and previous velocity
	XMVECTOR vel(XMLoadFloat3(&b->GetVelocity())), pVel(XMLoadFloat3(&b->GetPrevVelocity()));
	//lerp the velocity back to the point just as the collision takes place by t or col dist
	XMVECTOR resPoint2 = XMVectorLerp(pVel, vel, lerp);
	//set the correct velocity
 	b->SetVelocity(resPoint2);
}

void Collisions::ResolveSphereCollisions(Ball* b0, Ball* b1)
{
	//extract the position and store in a vector for execution
	XMVECTOR sCenter0 = XMLoadFloat3(&b0->GetPosition());
	XMVECTOR sCenter1 = XMLoadFloat3(&b1->GetPosition());

	float damping = 0.8f;

	//first find the normalised vector n  
	//from the center of sphere1 to the center of sphere2
	XMVECTOR N = XMVector3Normalize(sCenter0 - sCenter1);

	//extract the velocity vector and store in a vector for execution
	XMVECTOR sVel0 = XMLoadFloat3(&b0->GetVelocity());
	XMVECTOR sVel1 = XMLoadFloat3(&b1->GetVelocity());

	//find the length of the component of each of the velocity vectors along n
	float a1 = XMVectorGetX(XMVector3Dot(sVel0, N));
	float a2 = XMVectorGetX(XMVector3Dot(sVel1, N));

	//calculate the optimised momentum of any particle with mass
	float opP = (2.0f *(a1 - a2)) / (b0->GetMass() + b1->GetMass());

	//calculate the new velocity vector for sphere 1
	XMVECTOR newVel0 = (sVel0 - opP * b1->GetMass() * N)*damping;
	//calculate the new velocity vector for sphere 2
	XMVECTOR newVel1 = (sVel1 + opP * b0->GetMass() * N)*damping;

	//return the values into a float 3 to be applied to the balls
	XMFLOAT3 vel0, vel1;
	XMStoreFloat3(&vel0, newVel0);
	XMStoreFloat3(&vel1, newVel1);
	b0->SetVelocity(vel0);
	b1->SetVelocity(vel1);
}

bool Collisions::IntersectRayCyclinder(Ball* b0, XMFLOAT3 startP, XMFLOAT3 endQ, float& ColDist)
{
	XMFLOAT3 center = b0->GetPosition();	//center of the sphere
	float r = b0->GetRadius();				//radius of the sphere

	XMVECTOR minP = XMLoadFloat3(&XMFLOAT3(center.x - r, center.y - r, center.z - r));	//min value of the sphere bounding box
	XMVECTOR maxP = XMLoadFloat3(&XMFLOAT3(center.x + r, center.y + r, center.z + r));  //max value of the sphere bounding box
	XMVECTOR dir = XMLoadFloat3(&endQ) - XMLoadFloat3(&startP);
	XMVECTOR m = minP - XMLoadFloat3(&startP);
	XMVECTOR n = maxP - minP;

	float md = XMVectorGetX(XMVector3Dot(m, dir));
	float nd = XMVectorGetX(XMVector3Dot(n, dir));
	float dd = XMVectorGetX(XMVector3Dot(dir, dir));

	// Test if segment fully outside either endcap of cylinder
	if (md < 0.0f && md + nd < 0.0f) return false; // Segment outside ’startP’ side of cylinder
	if (md > dd && md + nd > dd) return false; // Segment outside ’endP’ side of cylinder

	float nn = XMVectorGetX(XMVector3Dot(n, n));
	float mn = XMVectorGetX(XMVector3Dot(m, n));
	float a = dd * nn - nd * nd;
	float k = XMVectorGetX(XMVector3Dot(m, m)) - r * r;
	float c = dd * k - md * md;

	if (fabs(a) < std::numeric_limits<float>::epsilon()) {
		// Segment runs parallel to cylinder axis
		if (c > 0.0f) return false; // ’a’ and thus the segment lie outside cylinder
		// Now known that segment intersects cylinder; figure out how it intersects
		if (md < 0.0f) ColDist = -mn / nn; // Intersect segment against ’p’ endcap
		else if (md > dd) ColDist = (nd - mn) / nn; // Intersect segment against ’q’ endcap
		else ColDist = 0.0f; // ’a’ lies inside cylinder
		return true;
	}

	float b = dd * mn - nd * md;
	float discr = b * b - a * c;
	if (discr < 0.0f) return false; // No real roots; no intersection

	ColDist = (-b - sqrt(discr)) / a;

	if (ColDist < 0.0f || ColDist > 1.0f) return 0; // Intersection lies outside segment
	if (md + ColDist * nd < 0.0f) {
		// Intersection outside cylinder on ’p’ side
		if (nd <= 0.0f) return false; // Segment pointing away from endcap
		ColDist = -md / nd;
		// Keep intersection if Dot(S(t) - p, S(t) - p) <= r∧2
		return k + 2 * ColDist * (mn + ColDist * nn) <= 0.0f;
	}
	else if (md + ColDist * nd > dd) {
		// Intersection outside cylinder on ’q’ side
		if (nd >= 0.0f) return false; // Segment pointing away from endcap
		ColDist = (dd - md) / nd;
		// Keep intersection if Dot(S(t) - q, S(t) - q) <= r∧2
		return k + dd - 2 * md + ColDist * (2 * (mn - nd) + ColDist * nn) <= 0.0f;
	}

	// Segment intersects cylinder between the endcaps; t is correct
	return true;
}


bool Collisions::TestSpherePolyIntersection(Ball* b, FaceCollisionData fcd, float& t)
{
	//extract radius of tested with sphere 
	float exR = b->GetRadius();
	//acquire velocity of the ball and the lack of velocity for the corner's spheres
	XMVECTOR V, vel0(XMLoadFloat3(&b->GetVelocity())), vel1(XMLoadFloat3(&XMFLOAT3(0, 0, 0)));
	
	XMVECTOR colPos, colNorm;

	//raise the triangle by its normal to rest the triangle evenly on spheres and cyclinder if triangle checks fail
	FaceCollisionData upTri, downTri;
	upTri = downTri = fcd;
	upTri.m_v0.x += upTri.m_vNormal.x;
	upTri.m_v0.y += upTri.m_vNormal.y;
	upTri.m_v0.z += upTri.m_vNormal.z;
	upTri.m_v1.x += upTri.m_vNormal.x;
	upTri.m_v1.y += upTri.m_vNormal.y;
	upTri.m_v1.z += upTri.m_vNormal.z;
	upTri.m_v2.x += upTri.m_vNormal.x;
	upTri.m_v2.y += upTri.m_vNormal.y;
	upTri.m_v2.z += upTri.m_vNormal.z;
	
	//test ray triangle intesection 
	if (IntersectRayTriangle(upTri, b, colPos, colNorm, t)){
		if (t <= b->GetSpeed() && t >= 0.0f)
		{
			b->SetPosition(colPos);
			return true;
		}
	}

	//turn the corners into sphere
	Ball cor1(exR, fcd.m_v0), cor2(exR, fcd.m_v1), cor3(exR, fcd.m_v2);
	V = vel0 - vel1;
	float vLen = XMVectorGetX(XMVector3Length(V));

	//check for corners collision
	if (IntersectRaySphere(b->GetPosition(), V / vLen, &cor1, colPos, t)){
		if (t <= b->GetSpeed() && t >= 0.0f)
		{
			b->SetPosition(colPos);
			return true;
		}
	}
	
	if (IntersectRaySphere(b->GetPosition(), V / vLen, &cor2, colPos, t)){
		if (t <= b->GetSpeed() && t >= 0.0f)
		{
			b->SetPosition(colPos);
			return true;
		}
	}
	
	if (IntersectRaySphere(b->GetPosition(), V / vLen, &cor3, colPos, t)){
		if (t <= b->GetSpeed() && t >= 0.0f)
		{
			b->SetPosition(colPos);
			return true;
		}
	}

	//turn the edges into cylinder
	//check for edges
	if (IntersectRayCyclinder(b, fcd.m_v0, fcd.m_v1, t)){
		if (t <= b->GetSpeed() && t >= 0.0f)
		{
			colPos = XMLoadFloat3(&b->GetPosition()) + XMLoadFloat3(&b->GetVelocity()) * t;
			b->SetPosition(colPos);
			return true;
		}
	}

	if (IntersectRayCyclinder(b, fcd.m_v1, fcd.m_v2, t)){
		if (t <= b->GetSpeed() && t >= 0.0f)
		{
			colPos = XMLoadFloat3(&b->GetPosition()) + XMLoadFloat3(&b->GetVelocity()) * t;
			b->SetPosition(colPos);
			return true;
		}
	}

	if (IntersectRayCyclinder(b, fcd.m_v2, fcd.m_v0, t)){
		if (t <= b->GetSpeed() && t >= 0.0f)
		{
			colPos = XMLoadFloat3(&b->GetPosition()) + XMLoadFloat3(&b->GetVelocity()) * t;
			b->SetPosition(colPos);
			return true;
		}
	}

	//if it fails all the above there is no collision
	return false;
}

// Function:	rayTriangle
// Description: Tests a ray for intersection with a triangle
// Parameters:
//				vert0		First vertex of triangle 
//				vert1		Second vertex of triangle
//				vert3		Third vertex of triangle
//				rayPos		Start position of ray
//				rayDir		Direction of ray
//				colPos		Position of collision (returned)
//				colNormN	The normalised Normal to triangle (returned)
//				colDist		Distance from rayPos to collision (returned)
// Returns: 	true if the intersection point lies within the bounds of the triangle.
// Notes: 		Not for the faint-hearted :) you weren't kidding :P
bool Collisions::IntersectRayTriangle(FaceCollisionData fcd, Ball* b, XMVECTOR& colPos, XMVECTOR& colNormN, float& colDist)
{
	XMVECTOR vert0 = XMLoadFloat3(&fcd.m_v0);
	XMVECTOR vert1 = XMLoadFloat3(&fcd.m_v1);
	XMVECTOR vert2 = XMLoadFloat3(&fcd.m_v2);

	XMVECTOR rayPos(XMLoadFloat3(&b->GetPosition()));

	XMVECTOR rayDir(XMLoadFloat3(&b->GetVelocity()));

	// Step 1: Calculate |COLNORM|  //[A,B,C] = Normalised normal of the plane
	colNormN = XMVector3Normalize(XMVector3Cross(vert1 - vert0, vert2 - vert1));
	XMVECTOR NRayDir = XMVector3Normalize(rayDir);
	// ...

	//so[A, B, C] .[x, y, z] = -D
	//[ x, y, z ] = Pc  (any point on the plane) i.e. vert0, vert1, vert2
	// Step 2: Use |COLNORM| and any vertex on the triangle to calculate D
	float D;
	XMStoreFloat(&D, -XMVector3Dot(colNormN, vert0));

	//|N|.|V|
	// Step 3: Calculate the demoninator of the COLDIST equation: (|COLNORM| dot |RAYDIR|) and "early out" (return false) if it is 0
	float Denom;
	XMStoreFloat(&Denom, XMVector3Dot(colNormN, NRayDir));
	if (Denom == 0) return false;
	// ...

	//d= -(D+(|N|.Po)
	// Step 4: Calculate the numerator of the COLDIST equation: -(D+(|COLNORM| dot RAYPOS))
	float Numer;
	Numer = -(D + XMVectorGetY(XMVector3Dot(colNormN, rayPos)));
	// ...
	//

	//-(D + (| N | .Po)/| N | . | V |
	// Step 5: Calculate COLDIST and "early out" again if COLDIST is behind RAYDIR
	colDist = Numer / Denom;
	if (colDist < 0) return false;
	// ...

	//Pc = Po + dV
	// Step 6: Use COLDIST to calculate COLPOS
	colPos = rayPos + (colDist * NRayDir);
	// ...

	// Part 2: Work out if the intersection point falls within the triangle
	// If the point is inside the triangle then it will be contained by the three new planes defined by:
	// 1) RAYPOS, VERT0, VERT1		2) RAYPOS, VERT1, VERT2		3) RAYPOS, VERT2, VERT0

	// Move the ray backwards by a tiny bit (one unit) in case the ray is already on the plane
	XMVECTOR NRayPos = rayPos - NRayDir;

	if (PointPlane(NRayPos, vert1, vert0, colPos))
		return false;

	if (PointPlane(NRayPos, vert2, vert1, colPos))
		return false;

	if (PointPlane(NRayPos, vert0, vert2, colPos))
		return false;

	// Step 4: Return true! (on triangle)
	return true;
}

// Function:	pointPlane
// Description: Tests a point to see if it is in front of a plane
// Parameters:
//				vert0		First point on plane 
//				vert1		Second point on plane 
//				vert3		Third point on plane 
//				pointPos	Point to test
// Returns: 	true if the point is in front of the plane
bool Collisions::PointPlane(const XMVECTOR& rayPos, const XMVECTOR& vert1, const XMVECTOR& vert2, const XMVECTOR& pointPos)
{
	// For any point on the plane [x,y,z] Ax + By + Cz + D = 0
	// So if Ax + By + Cz + D < 0 then the point is behind the plane
	// --> [ A,B,C ] dot [ x,y,z ] + D < 0
	// --> |PNORM| dot POINTPOS + D < 0
	// but D = -(|PNORM| dot VERT0 )
	// --> (|PNORM| dot POINTPOS) - (|PNORM| dot VERT0) < 0
	XMVECTOR sNormN;
	float sD, sNumer;

	// Step 1: Calculate PNORM
	sNormN = XMVector3Cross(vert1 - rayPos, vert2 - rayPos);

	//  Step 1: Test against plane 1 and return false if behind plane
	// Step 2: Calculate D
	sD = -XMVectorGetY(XMVector3Dot(sNormN, rayPos));

	// Step 3: Calculate full equation
	if (XMVectorGetY(XMVector3Dot(sNormN, pointPos)) + sD < 0)
		return true;	// Step 4: Return false if < 0 (behind plane)

	// Step 5: Return true! (in front of plane)
	return false;
}