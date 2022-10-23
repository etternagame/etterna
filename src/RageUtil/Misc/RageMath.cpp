/*
 * Most of these prototypes match up with the D3DX math functions.  Take a
 * function name, replace "Rage" with "D3DX" and look it up in the D3D SDK
 * docs for details.
 */

#include "Etterna/Globals/global.h"
#include "RageMath.h"
#include "RageTypes.h"

#include <cfloat>
#include <algorithm>

#ifdef _WIN32

#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wnew-returns-null"
#pragma clang diagnostic ignored "-Wcomment"
#endif

#include <d3dx9math.h>

#if defined(__clang__)
#pragma clang diagnostic pop
#endif

#endif

void
RageVec2RotateFromOrigin(RageVector2* pOut, float degrees)
{
	const auto radians = degrees * PI / 180;
	const auto outx =
	  pOut->x * RageFastCos(radians) - pOut->y * RageFastSin(radians);
	const auto outy =
	  pOut->x * RageFastSin(radians) + pOut->y * RageFastCos(radians);
	pOut->x = outx;
	pOut->y = outy;
}

void
RageVec2RotateFromPoint(RageVector2* p1, RageVector2* p2, float degrees)
{
	const auto xdiff = p2->x - p1->x;
	const auto ydiff = p2->y - p1->y;
	RageVector2 p3(xdiff, ydiff);
	RageVec2RotateFromOrigin(&p3, degrees);
	p2->x = p1->x + p3.x;
	p2->y = p1->y + p3.y;
}

void
RageVec3ClearBounds(RageVector3& mins, RageVector3& maxs)
{
	mins = RageVector3(FLT_MAX, FLT_MAX, FLT_MAX);
	maxs = mins * -1;
}

void
RageVec3AddToBounds(const RageVector3& p, RageVector3& mins, RageVector3& maxs)
{
	mins.x = std::min(mins.x, p.x);
	mins.y = std::min(mins.y, p.y);
	mins.z = std::min(mins.z, p.z);
	maxs.x = std::max(maxs.x, p.x);
	maxs.y = std::max(maxs.y, p.y);
	maxs.z = std::max(maxs.z, p.z);
}

void
RageVec2Normalize(RageVector2* pOut, const RageVector2* pV)
{
#ifdef _WIN32
	D3DXVec2Normalize(reinterpret_cast<D3DXVECTOR2*>(pOut), (D3DXVECTOR2*)pV);
#else
	float scale = 1.0f / sqrtf(pV->x * pV->x + pV->y * pV->y);
	pOut->x = pV->x * scale;
	pOut->y = pV->y * scale;
#endif
}

void
RageVec3Normalize(RageVector3* pOut, const RageVector3* pV)
{
#ifdef _WIN32
	D3DXVec3Normalize(reinterpret_cast<D3DXVECTOR3*>(pOut), (D3DXVECTOR3*)pV);
#else
	float scale = 1.0f / sqrtf(pV->x * pV->x + pV->y * pV->y + pV->z * pV->z);
	pOut->x = pV->x * scale;
	pOut->y = pV->y * scale;
	pOut->z = pV->z * scale;
#endif
}

void
VectorFloatNormalize(std::vector<float>& v)
{
	ASSERT_M(v.size() == 3, "Can't normalize a non-3D std::vector.");
	const auto scale = 1.0f / sqrtf(v[0] * v[0] + v[1] * v[1] + v[2] * v[2]);
	v[0] *= scale;
	v[1] *= scale;
	v[2] *= scale;
}

void
RageVec3Cross(RageVector3* ret, const RageVector3* a, const RageVector3* b)
{
#ifdef _WIN32
	D3DXVec3Cross(
	  reinterpret_cast<D3DXVECTOR3*>(ret), (D3DXVECTOR3*)a, (D3DXVECTOR3*)b);
#else
	ret->x = (a->y * b->z) - (a->z * b->y);
	ret->y = ((a->x * b->z) - (a->z * b->x));
	ret->z = (a->x * b->y) - (a->y * b->x);
#endif
}

void
RageVec3TransformCoord(RageVector3* pOut,
					   const RageVector3* pV,
					   const RageMatrix* pM)
{
#ifdef _WIN32
	D3DXVec3TransformCoord(
	  reinterpret_cast<D3DXVECTOR3*>(pOut), (D3DXVECTOR3*)pV, (D3DXMATRIX*)pM);
#else
	RageVector4 temp(pV->x, pV->y, pV->z, 1.0f); // translate
	RageVec4TransformCoord(&temp, &temp, pM);
	*pOut = RageVector3(temp.x / temp.w, temp.y / temp.w, temp.z / temp.w);
#endif
}

void
RageVec3TransformNormal(RageVector3* pOut,
						const RageVector3* pV,
						const RageMatrix* pM)
{
#ifdef _WIN32
	D3DXVec3TransformNormal(
	  reinterpret_cast<D3DXVECTOR3*>(pOut), (D3DXVECTOR3*)pV, (D3DXMATRIX*)pM);
#else
	RageVector4 temp(pV->x, pV->y, pV->z, 0.0f); // don't translate
	RageVec4TransformCoord(&temp, &temp, pM);
	*pOut = RageVector3(temp.x, temp.y, temp.z);
#endif
}

#define m00 m[0][0]
#define m01 m[0][1]
#define m02 m[0][2]
#define m03 m[0][3]
#define m10 m[1][0]
#define m11 m[1][1]
#define m12 m[1][2]
#define m13 m[1][3]
#define m20 m[2][0]
#define m21 m[2][1]
#define m22 m[2][2]
#define m23 m[2][3]
#define m30 m[3][0]
#define m31 m[3][1]
#define m32 m[3][2]
#define m33 m[3][3]

void
RageVec4TransformCoord(RageVector4* pOut,
					   const RageVector4* pV,
					   const RageMatrix* pM)
{
	const auto& a = *pM;
	const auto& v = *pV;
	*pOut = RageVector4(a.m00 * v.x + a.m10 * v.y + a.m20 * v.z + a.m30 * v.w,
						a.m01 * v.x + a.m11 * v.y + a.m21 * v.z + a.m31 * v.w,
						a.m02 * v.x + a.m12 * v.y + a.m22 * v.z + a.m32 * v.w,
						a.m03 * v.x + a.m13 * v.y + a.m23 * v.z + a.m33 * v.w);
}

RageMatrix::RageMatrix(float v00,
					   float v01,
					   float v02,
					   float v03,
					   float v10,
					   float v11,
					   float v12,
					   float v13,
					   float v20,
					   float v21,
					   float v22,
					   float v23,
					   float v30,
					   float v31,
					   float v32,
					   float v33)
{
	m00 = v00;
	m01 = v01;
	m02 = v02;
	m03 = v03;
	m10 = v10;
	m11 = v11;
	m12 = v12;
	m13 = v13;
	m20 = v20;
	m21 = v21;
	m22 = v22;
	m23 = v23;
	m30 = v30;
	m31 = v31;
	m32 = v32;
	m33 = v33;
}

void
RageMatrixIdentity(RageMatrix* pOut)
{
#ifdef _WIN32
	D3DXMatrixIdentity(reinterpret_cast<D3DXMATRIX*>(pOut));
#else
	static float identity[16] = {
		1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1
	};
	memcpy(&pOut->m00, identity, sizeof(identity));
/*	*pOut = RageMatrix(
		1,0,0,0,
		0,1,0,0,
		0,0,1,0,
		0,0,0,1 );
*/
#endif
}

RageMatrix
RageMatrix::GetTranspose() const
{
	return RageMatrix(m00,
					  m10,
					  m20,
					  m30,
					  m01,
					  m11,
					  m21,
					  m31,
					  m02,
					  m12,
					  m22,
					  m32,
					  m03,
					  m13,
					  m23,
					  m33);
}

void
RageMatrixMultiply(RageMatrix* pOut, const RageMatrix* pA, const RageMatrix* pB)
{
#ifdef _WIN32
	D3DXMatrixMultiply(
	  reinterpret_cast<D3DXMATRIX*>(pOut), (D3DXMATRIX*)pB, (D3DXMATRIX*)pA);
#else
	const RageMatrix& a = *pA;
	const RageMatrix& b = *pB;

	*pOut =
	  RageMatrix(b.m00 * a.m00 + b.m01 * a.m10 + b.m02 * a.m20 + b.m03 * a.m30,
				 b.m00 * a.m01 + b.m01 * a.m11 + b.m02 * a.m21 + b.m03 * a.m31,
				 b.m00 * a.m02 + b.m01 * a.m12 + b.m02 * a.m22 + b.m03 * a.m32,
				 b.m00 * a.m03 + b.m01 * a.m13 + b.m02 * a.m23 + b.m03 * a.m33,
				 b.m10 * a.m00 + b.m11 * a.m10 + b.m12 * a.m20 + b.m13 * a.m30,
				 b.m10 * a.m01 + b.m11 * a.m11 + b.m12 * a.m21 + b.m13 * a.m31,
				 b.m10 * a.m02 + b.m11 * a.m12 + b.m12 * a.m22 + b.m13 * a.m32,
				 b.m10 * a.m03 + b.m11 * a.m13 + b.m12 * a.m23 + b.m13 * a.m33,
				 b.m20 * a.m00 + b.m21 * a.m10 + b.m22 * a.m20 + b.m23 * a.m30,
				 b.m20 * a.m01 + b.m21 * a.m11 + b.m22 * a.m21 + b.m23 * a.m31,
				 b.m20 * a.m02 + b.m21 * a.m12 + b.m22 * a.m22 + b.m23 * a.m32,
				 b.m20 * a.m03 + b.m21 * a.m13 + b.m22 * a.m23 + b.m23 * a.m33,
				 b.m30 * a.m00 + b.m31 * a.m10 + b.m32 * a.m20 + b.m33 * a.m30,
				 b.m30 * a.m01 + b.m31 * a.m11 + b.m32 * a.m21 + b.m33 * a.m31,
				 b.m30 * a.m02 + b.m31 * a.m12 + b.m32 * a.m22 + b.m33 * a.m32,
				 b.m30 * a.m03 + b.m31 * a.m13 + b.m32 * a.m23 + b.m33 * a.m33);
	// phew!
#endif
}

void
RageMatrixTranslation(RageMatrix* pOut, float x, float y, float z)
{
#ifdef _WIN32
	D3DXMatrixTranslation(reinterpret_cast<D3DXMATRIX*>(pOut), x, y, z);
#else
	RageMatrixIdentity(pOut);
	pOut->m[3][0] = x;
	pOut->m[3][1] = y;
	pOut->m[3][2] = z;
#endif
}

void
RageMatrixScaling(RageMatrix* pOut, float x, float y, float z)
{
#ifdef _WIN32
	D3DXMatrixScaling(reinterpret_cast<D3DXMATRIX*>(pOut), x, y, z);
#else
	RageMatrixIdentity(pOut);
	pOut->m[0][0] = x;
	pOut->m[1][1] = y;
	pOut->m[2][2] = z;
#endif
}

void
RageMatrixSkewX(RageMatrix* pOut, float fAmount)
{
	RageMatrixIdentity(pOut);
	pOut->m[1][0] = fAmount;
}

void
RageMatrixSkewY(RageMatrix* pOut, float fAmount)
{
	RageMatrixIdentity(pOut);
	pOut->m[0][1] = fAmount;
}

/*
 * Return:
 *
 * RageMatrix translate;
 * RageMatrixTranslation( &translate, fTransX, fTransY, fTransZ );
 * RageMatrix scale;
 * RageMatrixScaling( &scale, fScaleX, float fScaleY, float fScaleZ );
 * RageMatrixMultiply( pOut, &translate, &scale );
 */
void
RageMatrixTranslate(RageMatrix* pOut,
					float fTransX,
					float fTransY,
					float fTransZ)
{
#ifdef _WIN32
	D3DXMatrixTranslation(
	  reinterpret_cast<D3DXMATRIX*>(pOut), fTransX, fTransY, fTransZ);
#else
	pOut->m00 = 1;
	pOut->m01 = 0;
	pOut->m02 = 0;
	pOut->m03 = 0;

	pOut->m10 = 0;
	pOut->m11 = 1;
	pOut->m12 = 0;
	pOut->m13 = 0;

	pOut->m20 = 0;
	pOut->m21 = 0;
	pOut->m22 = 1;
	pOut->m23 = 0;

	pOut->m30 = fTransX;
	pOut->m31 = fTransY;
	pOut->m32 = fTransZ;
	pOut->m33 = 1;
#endif
}

void
RageMatrixScale(RageMatrix* pOut, float fScaleX, float fScaleY, float fScaleZ)
{
#ifdef _WIN32
	D3DXMatrixScaling(
	  reinterpret_cast<D3DXMATRIX*>(pOut), fScaleX, fScaleY, fScaleZ);
#else
	pOut->m00 = fScaleX;
	pOut->m01 = 0;
	pOut->m02 = 0;
	pOut->m03 = 0;

	pOut->m10 = 0;
	pOut->m11 = fScaleY;
	pOut->m12 = 0;
	pOut->m13 = 0;

	pOut->m20 = 0;
	pOut->m21 = 0;
	pOut->m22 = fScaleZ;
	pOut->m23 = 0;

	pOut->m30 = 0;
	pOut->m31 = 0;
	pOut->m32 = 0;
	pOut->m33 = 1;
#endif
}

void
RageMatrixRotationX(RageMatrix* pOut, float theta)
{
	// D3DXMatrixRotationX is slower
	theta *= PI / 180;

	RageMatrixIdentity(pOut);
	pOut->m[1][1] = RageFastCos(theta);
	pOut->m[2][2] = pOut->m[1][1];

	pOut->m[2][1] = RageFastSin(theta);
	pOut->m[1][2] = -pOut->m[2][1];
}

void
RageMatrixRotationY(RageMatrix* pOut, float theta)
{
	// D3DXMatrixRotationY is slower
	theta *= PI / 180;

	RageMatrixIdentity(pOut);
	pOut->m[0][0] = RageFastCos(theta);
	pOut->m[2][2] = pOut->m[0][0];

	pOut->m[0][2] = RageFastSin(theta);
	pOut->m[2][0] = -pOut->m[0][2];
}

void
RageMatrixRotationZ(RageMatrix* pOut, float theta)
{
	// D3DXMatrixRotationZ is slower
	theta *= PI / 180;

	RageMatrixIdentity(pOut);
	pOut->m[0][0] = RageFastCos(theta);
	pOut->m[1][1] = pOut->m[0][0];

	pOut->m[0][1] = RageFastSin(theta);
	pOut->m[1][0] = -pOut->m[0][1];
}

/* Return RageMatrixRotationX(rX) * RageMatrixRotationY(rY) *
 * RageMatrixRotationZ(rZ) quickly (without actually doing two complete matrix
 * multiplies), by removing the parts of the matrix multiplies that we know will
 * be 0. */
void
RageMatrixRotationXYZ(RageMatrix* pOut, float rX, float rY, float rZ)
{
	rX *= PI / 180;
	rY *= PI / 180;
	rZ *= PI / 180;

	const auto cX = RageFastCos(rX);
	const auto sX = RageFastSin(rX);
	const auto cY = RageFastCos(rY);
	const auto sY = RageFastSin(rY);
	const auto cZ = RageFastCos(rZ);
	const auto sZ = RageFastSin(rZ);

	/*
	 * X*Y:
	 * RageMatrix(
	 *	cY,  sY*sX, sY*cX, 0,
	 *	0,   cX,    -sX,   0,
	 *	-sY, cY*sX, cY*cX, 0,
	 *	0, 0, 0, 1
	 * );
	 *
	 * X*Y*Z:
	 *
	 * RageMatrix(
	 *	cZ*cY, cZ*sY*sX+sZ*cX, cZ*sY*cX+sZ*(-sX), 0,
	 *	(-sZ)*cY, (-sZ)*sY*sX+cZ*cX, (-sZ)*sY*cX+cZ*(-sX), 0,
	 *	-sY, cY*sX, cY*cX, 0,
	 *	0, 0, 0, 1
	 * );
	 */

	pOut->m00 = cZ * cY;
	pOut->m01 = cZ * sY * sX + sZ * cX;
	pOut->m02 = cZ * sY * cX + sZ * (-sX);
	pOut->m03 = 0;
	pOut->m10 = (-sZ) * cY;
	pOut->m11 = (-sZ) * sY * sX + cZ * cX;
	pOut->m12 = (-sZ) * sY * cX + cZ * (-sX);
	pOut->m13 = 0;
	pOut->m20 = -sY;
	pOut->m21 = cY * sX;
	pOut->m22 = cY * cX;
	pOut->m23 = 0;
	pOut->m30 = 0;
	pOut->m31 = 0;
	pOut->m32 = 0;
	pOut->m33 = 1;
}

void
RageAARotate(RageVector3* inret, RageVector3 const* axis, float angle)
{
	const auto ha = angle / 2.0f;
	const auto ca2 = RageFastCos(ha);
	const auto sa2 = RageFastSin(ha);
	const RageVector4 quat(axis->x * sa2, axis->y * sa2, axis->z * sa2, ca2);
	const RageVector4 quatc(-quat.x, -quat.y, -quat.z, ca2);
	RageVector4 point(inret->x, inret->y, inret->z, 0.0f);
	RageQuatMultiply(&point, quat, point);
	RageQuatMultiply(&point, point, quatc);
	inret->x = point.x;
	inret->y = point.y;
	inret->z = point.z;
}

void
RageQuatMultiply(RageVector4* pOut,
				 const RageVector4& pA,
				 const RageVector4& pB)
{
#ifdef _WIN32
	D3DXQuaternionMultiply(reinterpret_cast<D3DXQUATERNION*>(pOut),
						   (D3DXQUATERNION*)&pA,
						   (D3DXQUATERNION*)&pB);
#else
	RageVector4 out;
	out.x = pA.w * pB.x + pA.x * pB.w + pA.y * pB.z - pA.z * pB.y;
	out.y = pA.w * pB.y + pA.y * pB.w + pA.z * pB.x - pA.x * pB.z;
	out.z = pA.w * pB.z + pA.z * pB.w + pA.x * pB.y - pA.y * pB.x;
	out.w = pA.w * pB.w - pA.x * pB.x - pA.y * pB.y - pA.z * pB.z;

	float dist, square;

	square = out.x * out.x + out.y * out.y + out.z * out.z + out.w * out.w;

	if (square > 0.0)
		dist = 1.0f / sqrtf(square);
	else
		dist = 1;

	out.x *= dist;
	out.y *= dist;
	out.z *= dist;
	out.w *= dist;

	*pOut = out;
#endif
}

RageVector4
RageQuatFromH(float theta)
{
	theta *= PI / 180.0f;
	theta /= 2.0f;
	theta *= -1;
	const auto c = RageFastCos(theta);
	const auto s = RageFastSin(theta);

	return RageVector4(0, s, 0, c);
}

RageVector4
RageQuatFromP(float theta)
{
	theta *= PI / 180.0f;
	theta /= 2.0f;
	theta *= -1;
	const auto c = RageFastCos(theta);
	const auto s = RageFastSin(theta);

	return RageVector4(s, 0, 0, c);
}

RageVector4
RageQuatFromR(float theta)
{
	theta *= PI / 180.0f;
	theta /= 2.0f;
	theta *= -1;
	const auto c = RageFastCos(theta);
	const auto s = RageFastSin(theta);

	return RageVector4(0, 0, s, c);
}

/* Math from http://www.gamasutra.com/features/19980703/quaternions_01.htm . */

/* prh.xyz -> heading, pitch, roll */
void
RageQuatFromHPR(RageVector4* pOut, RageVector3 hpr)
{
	hpr *= PI;
	hpr /= 180.0f;
	hpr /= 2.0f;

	const auto sX = RageFastSin(hpr.x);
	const auto cX = RageFastCos(hpr.x);
	const auto sY = RageFastSin(hpr.y);
	const auto cY = RageFastCos(hpr.y);
	const auto sZ = RageFastSin(hpr.z);
	const auto cZ = RageFastCos(hpr.z);

	pOut->w = cX * cY * cZ + sX * sY * sZ;
	pOut->x = sX * cY * cZ - cX * sY * sZ;
	pOut->y = cX * sY * cZ + sX * cY * sZ;
	pOut->z = cX * cY * sZ - sX * sY * cZ;
}

/*
 * Screen orientatoin:  the "floor" is the XZ plane, and Y is height; in other
 * words, the screen is the XY plane and negative Z goes into it.
 */

/* prh.xyz -> pitch, roll, heading */
void
RageQuatFromPRH(RageVector4* pOut, RageVector3 prh)
{
	prh *= PI;
	prh /= 180.0f;
	prh /= 2.0f;

	/* Set cX to the cosine of the angle we want to rotate on the X axis,
	 * and so on.  Here, hpr.z (roll) rotates on the Z axis, hpr.x (heading)
	 * on Y, and hpr.y (pitch) on X. */
	const auto sX = RageFastSin(prh.y);
	const auto cX = RageFastCos(prh.y);
	const auto sY = RageFastSin(prh.x);
	const auto cY = RageFastCos(prh.x);
	const auto sZ = RageFastSin(prh.z);
	const auto cZ = RageFastCos(prh.z);

	pOut->w = cX * cY * cZ + sX * sY * sZ;
	pOut->x = sX * cY * cZ - cX * sY * sZ;
	pOut->y = cX * sY * cZ + sX * cY * sZ;
	pOut->z = cX * cY * sZ - sX * sY * cZ;
}

void
RageMatrixFromQuat(RageMatrix* pOut, const RageVector4& q)
{
	// D3DXMatrixRotationQuaternion is slower
	const auto xx = q.x * (q.x + q.x);
	const auto xy = q.x * (q.y + q.y);
	const auto xz = q.x * (q.z + q.z);

	const auto wx = q.w * (q.x + q.x);
	const auto wy = q.w * (q.y + q.y);
	const auto wz = q.w * (q.z + q.z);

	const auto yy = q.y * (q.y + q.y);
	const auto yz = q.y * (q.z + q.z);

	const auto zz = q.z * (q.z + q.z);
	// careful.  The param order is row-major, which is the
	// transpose of the order shown in the OpenGL docs.
	*pOut = RageMatrix(1 - (yy + zz),
					   xy + wz,
					   xz - wy,
					   0,
					   xy - wz,
					   1 - (xx + zz),
					   yz + wx,
					   0,
					   xz + wy,
					   yz - wx,
					   1 - (xx + yy),
					   0,
					   0,
					   0,
					   0,
					   1);
}

void
RageQuatSlerp(RageVector4* pOut,
			  const RageVector4& from,
			  const RageVector4& to,
			  float t)
{
#ifdef _WIN32
	D3DXQuaternionSlerp(reinterpret_cast<D3DXQUATERNION*>(pOut),
						(D3DXQUATERNION*)&from,
						(D3DXQUATERNION*)&to,
						t);
#else
	float to1[4];

	// calc cosine
	float cosom = from.x * to.x + from.y * to.y + from.z * to.z + from.w * to.w;

	// adjust signs (if necessary)
	if (cosom < 0) {
		cosom = -cosom;
		to1[0] = -to.x;
		to1[1] = -to.y;
		to1[2] = -to.z;
		to1[3] = -to.w;
	} else {
		to1[0] = to.x;
		to1[1] = to.y;
		to1[2] = to.z;
		to1[3] = to.w;
	}

	// calculate coefficients
	float scale0, scale1;
	if (cosom < 0.9999f) {
		// standard case (slerp)
		float omega = acosf(cosom);
		float sinom = RageFastSin(omega);
		scale0 = RageFastSin((1.0f - t) * omega) / sinom;
		scale1 = RageFastSin(t * omega) / sinom;
	} else {
		// "from" and "to" quaternions are very close
		//  ... so we can do a linear interpolation
		scale0 = 1.0f - t;
		scale1 = t;
	}
	// calculate final values
	pOut->x = scale0 * from.x + scale1 * to1[0];
	pOut->y = scale0 * from.y + scale1 * to1[1];
	pOut->z = scale0 * from.z + scale1 * to1[2];
	pOut->w = scale0 * from.w + scale1 * to1[3];
#endif
}

RageMatrix
RageLookAt(float eyex,
		   float eyey,
		   float eyez,
		   float centerx,
		   float centery,
		   float centerz,
		   float upx,
		   float upy,
		   float upz)
{
	// D3DXMatrixLookAtRH is slower here
	RageVector3 Z(eyex - centerx, eyey - centery, eyez - centerz);
	RageVec3Normalize(&Z, &Z);

	RageVector3 Y(upx, upy, upz);

	RageVector3 X(Y[1] * Z[2] - Y[2] * Z[1],
				  -Y[0] * Z[2] + Y[2] * Z[0],
				  Y[0] * Z[1] - Y[1] * Z[0]);

	Y = RageVector3(Z[1] * X[2] - Z[2] * X[1],
					-Z[0] * X[2] + Z[2] * X[0],
					Z[0] * X[1] - Z[1] * X[0]);

	RageVec3Normalize(&X, &X);
	RageVec3Normalize(&Y, &Y);

	RageMatrix mat(X[0],
				   Y[0],
				   Z[0],
				   0,
				   X[1],
				   Y[1],
				   Z[1],
				   0,
				   X[2],
				   Y[2],
				   Z[2],
				   0,
				   0,
				   0,
				   0,
				   1);

	RageMatrix mat2;
	RageMatrixTranslation(&mat2, -eyex, -eyey, -eyez);

	RageMatrix ret;
	RageMatrixMultiply(&ret, &mat, &mat2);

	return ret;
}

void
RageMatrixAngles(RageMatrix* pOut, const RageVector3& angles)
{
	const auto angles_radians(angles * 2 * PI / 360);

	const auto sy = RageFastSin(angles_radians[2]);
	const auto cy = RageFastCos(angles_radians[2]);
	const auto sp = RageFastSin(angles_radians[1]);
	const auto cp = RageFastCos(angles_radians[1]);
	const auto sr = RageFastSin(angles_radians[0]);
	const auto cr = RageFastCos(angles_radians[0]);

	RageMatrixIdentity(pOut);

	// matrix = (Z * Y) * X
	pOut->m[0][0] = cp * cy;
	pOut->m[0][1] = cp * sy;
	pOut->m[0][2] = -sp;
	pOut->m[1][0] = sr * sp * cy + cr * -sy;
	pOut->m[1][1] = sr * sp * sy + cr * cy;
	pOut->m[1][2] = sr * cp;
	pOut->m[2][0] = (cr * sp * cy + -sr * -sy);
	pOut->m[2][1] = (cr * sp * sy + -sr * cy);
	pOut->m[2][2] = cr * cp;
}

void
RageMatrixTranspose(RageMatrix* pOut, const RageMatrix* pIn)
{
#ifdef _WIN32
	D3DXMatrixTranspose(reinterpret_cast<D3DXMATRIX*>(pOut), (D3DXMATRIX*)pIn);
#else
	for (int i = 0; i < 4; i++)
		for (int j = 0; j < 4; j++)
			pOut->m[j][i] = pIn->m[i][j];
#endif
}

static const unsigned int sine_table_size = 1024;
static const unsigned int sine_index_mod = sine_table_size * 2;
static const double sine_table_index_mult =
  static_cast<double>(sine_index_mod) / (PI * 2);
static float sine_table[sine_table_size];

struct sine_initter
{
	sine_initter()
	{
		for (unsigned int i = 0; i < sine_table_size; ++i) {
			const auto angle = SCALE(i, 0, sine_table_size, 0.0f, PI);
			sine_table[i] = sinf(angle);
		}
	}
};

static sine_initter sinner;

const float
RageFastSin(float angle)
{
	if (angle == 0) {
		return 0;
	}
	const auto index = angle * static_cast<float>(sine_table_index_mult);
	auto first_index = static_cast<int>(index);
	const int second_index = (first_index + 1) % sine_index_mod;
	const auto remainder = index - first_index;
	first_index %= sine_index_mod;
	auto first = 0.0f;
	auto second = 0.0f;
#define SET_SAMPLE(sample)                                                     \
	if (sample##_index >= sine_table_size) {                                   \
		(sample) = -sine_table[sample##_index - sine_table_size];              \
	} else {                                                                   \
		(sample) = sine_table[sample##_index];                                 \
	}
	SET_SAMPLE(first);
	SET_SAMPLE(second);
#undef SET_SAMPLE
	const auto result = lerp(remainder, first, second);
	return result;
}

const float
RageFastCos(float x)
{
	return RageFastSin(x + 0.5f * PI);
}

float
RageQuadratic::Evaluate(float fT) const
{
	// optimized (m_fA * fT*fT*fT) + (m_fB * fT*fT) + (m_fC * fT) + m_fD;
	return ((m_fA * fT + m_fB) * fT + m_fC) * fT + m_fD;
}

void
RageQuadratic::SetFromBezier(float fX1, float fX2, float fX3, float fX4)
{
	m_fD = fX1;
	m_fC = 3.0f * (fX2 - fX1);
	m_fB = 3.0f * (fX3 - fX2) - m_fC;
	m_fA = fX4 - fX1 - m_fC - m_fB;
}

void
RageQuadratic::GetBezier(float& fX1, float& fX2, float& fX3, float& fX4) const
{
	fX1 = m_fD;
	fX2 = m_fD + m_fC / 3.0f;
	fX3 = m_fD + 2 * m_fC / 3.0f + m_fB / 3.0f;
	fX4 = m_fD + m_fC + m_fB + m_fA;
}

/* Cubic polynomial interpolation.  SetFromCubicPoly(-1, 0, 1, 2); Evaluate(p)
 * will interpolate between 0 and 1. */
void
RageQuadratic::SetFromCubic(float fX1, float fX2, float fX3, float fX4)
{
	m_fA = -1.0f / 6.0f * fX1 + +3.0f / 6.0f * fX2 + -3.0f / 6.0f * fX3 +
		   1.0f / 6.0f * fX4;
	m_fB = 3.0f / 6.0f * fX1 + -6.0f / 6.0f * fX2 + 3.0f / 6.0f * fX3;
	m_fC = -2.0f / 6.0f * fX1 + -3.0f / 6.0f * fX2 + fX3 + -1.0f / 6.0f * fX4;
	m_fD = fX2;
}

float
RageQuadratic::GetSlope(float fT) const
{
	return 3 * m_fA * fT * fT + 2 * m_fB * fT + m_fC;
}

void
RageBezier2D::Evaluate(float fT, float* pX, float* pY) const
{
	*pX = m_X.Evaluate(fT);
	*pY = m_Y.Evaluate(fT);
}

float
RageBezier2D::EvaluateYFromX(float fX) const
{
	/* Quickly approximate T using Newton-Raphelson successive optimization (see
	 * http://www.tinaja.com/text/bezmath.html).  This usually finds T within an
	 * acceptable error margin in a few steps. */
	auto fT = SCALE(fX, m_X.GetBezierStart(), m_X.GetBezierEnd(), 0, 1);
	// Don't try more than 100 times, the curve might be a bit nonsensical. -Kyz
	for (auto i = 0; i < 100; ++i) {
		const auto fGuessedX = m_X.Evaluate(fT);
		const auto fError = fX - fGuessedX;

		/* If our guess is good enough, evaluate the result Y and return. */
		if (unlikely(fabsf(fError) < 0.0001f))
			return m_Y.Evaluate(fT);

		const auto fSlope = m_X.GetSlope(fT);
		fT += fError / fSlope;
	}
	return m_Y.Evaluate(fT);
}

void
RageBezier2D::SetFromBezier(float fC1X,
							float fC1Y,
							float fC2X,
							float fC2Y,
							float fC3X,
							float fC3Y,
							float fC4X,
							float fC4Y)
{
	m_X.SetFromBezier(fC1X, fC2X, fC3X, fC4X);
	m_Y.SetFromBezier(fC1Y, fC2Y, fC3Y, fC4Y);
}

#include "Etterna/Models/Lua/LuaBinding.h"

struct LunaRageQuadratic : Luna<RageQuadratic>
{
	static int evaluate(T* p, lua_State* L)
	{
		lua_pushnumber(L, p->Evaluate(FArg(1)));
		return 1;
	}

	static int get_bezier(T* p, lua_State* L)
	{
		float a, b, c, d;
		p->GetBezier(a, b, c, d);
		lua_pushnumber(L, a);
		lua_pushnumber(L, b);
		lua_pushnumber(L, c);
		lua_pushnumber(L, d);
		return 4;
	}

	static int get_bezier_end(T* p, lua_State* L)
	{
		lua_pushnumber(L, p->GetBezierEnd());
		return 1;
	}

	static int get_bezier_start(T* p, lua_State* L)
	{
		lua_pushnumber(L, p->GetBezierStart());
		return 1;
	}

	static int get_slope(T* p, lua_State* L)
	{
		lua_pushnumber(L, p->GetSlope(FArg(1)));
		return 1;
	}

	static int set_from_bezier(T* p, lua_State* L)
	{
		p->SetFromBezier(FArg(1), FArg(2), FArg(3), FArg(4));
		COMMON_RETURN_SELF;
	}

	static int set_from_cubic(T* p, lua_State* L)
	{
		p->SetFromCubic(FArg(1), FArg(2), FArg(3), FArg(4));
		COMMON_RETURN_SELF;
	}

	LunaRageQuadratic()
	{
		ADD_METHOD(evaluate);
		ADD_METHOD(get_bezier);
		ADD_METHOD(get_bezier_end);
		ADD_METHOD(get_bezier_start);
		ADD_METHOD(get_slope);
		ADD_METHOD(set_from_bezier);
		ADD_METHOD(set_from_cubic);
	}
};

LUA_REGISTER_CLASS(RageQuadratic);

struct LunaRageBezier2D : Luna<RageBezier2D>
{
	static int evaluate(T* p, lua_State* L)
	{
		float x, y;
		p->Evaluate(FArg(1), &x, &y);
		lua_pushnumber(L, x);
		lua_pushnumber(L, y);
		return 2;
	}

	static int evaluate_y_from_x(T* p, lua_State* L)
	{
		lua_pushnumber(L, p->EvaluateYFromX(FArg(1)));
		return 1;
	}

	static int get_x(T* p, lua_State* L)
	{
		p->get_x().PushSelf(L);
		return 1;
	}

	static int get_y(T* p, lua_State* L)
	{
		p->get_y().PushSelf(L);
		return 1;
	}

	static int set_from_bezier(T* p, lua_State* L)
	{
		p->SetFromBezier(FArg(1),
						 FArg(2),
						 FArg(3),
						 FArg(4),
						 FArg(5),
						 FArg(6),
						 FArg(7),
						 FArg(8));
		COMMON_RETURN_SELF;
	}

	static int destroy(T* p, lua_State* L)
	{
		SAFE_DELETE(p);
		return 0;
	}

	LunaRageBezier2D()
	{
		ADD_METHOD(destroy);
		ADD_METHOD(evaluate);
		ADD_METHOD(evaluate_y_from_x);
		ADD_METHOD(get_x);
		ADD_METHOD(get_y);
		ADD_METHOD(set_from_bezier);
	}
};

LUA_REGISTER_CLASS(RageBezier2D);

int
LuaFunc_create_bezier(lua_State* L);

int
LuaFunc_create_bezier(lua_State* L)
{
	auto* bezier = new RageBezier2D;
	bezier->PushSelf(L);
	return 1;
}

LUAFUNC_REGISTER_COMMON(create_bezier);
