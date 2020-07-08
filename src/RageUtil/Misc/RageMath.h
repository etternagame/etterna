/* RageMath - vector/matrix math utilities. */

#ifndef RAGE_MATH_H
#define RAGE_MATH_H

#define PI (3.141592653589793f)
#define DegreeToRadian(degree) ((degree) * (PI / 180.0f))
#define RadianToDegree(radian) ((radian) * (180.0f / PI))

struct lua_State;
struct RageVector2;
struct RageVector3;
struct RageVector4;
struct RageMatrix;

void
RageVec2RotateFromOrigin(RageVector2* pOut, float degrees);

void
RageVec2RotateFromPoint(RageVector2* p1, RageVector2* p2, float degrees);

void
RageVec3ClearBounds(struct RageVector3& mins, RageVector3& maxs);
void
RageVec3AddToBounds(const struct RageVector3& p,
					RageVector3& mins,
					RageVector3& maxs);

void
RageVec2Normalize(struct RageVector2* pOut, const struct RageVector2* pV);
void
RageVec3Normalize(struct RageVector3* pOut, const struct RageVector3* pV);
void
VectorFloatNormalize(vector<float>& v);
void
RageVec3Cross(struct RageVector3* ret,
			  struct RageVector3 const* a,
			  struct RageVector3 const* b);
void
RageVec3TransformCoord(struct RageVector3* pOut,
					   const struct RageVector3* pV,
					   const struct RageMatrix* pM);
void
RageVec3TransformNormal(struct RageVector3* pOut,
						const struct RageVector3* pV,
						const struct RageMatrix* pM);
void
RageVec4TransformCoord(struct RageVector4* pOut,
					   const struct RageVector4* pV,
					   const struct RageMatrix* pM);
void
RageMatrixIdentity(struct RageMatrix* pOut);
void
RageMatrixMultiply(struct RageMatrix* pOut,
				   const struct RageMatrix* pA,
				   const struct RageMatrix* pB);
void
RageMatrixTranslation(struct RageMatrix* pOut, float x, float y, float z);
void
RageMatrixScaling(struct RageMatrix* pOut, float x, float y, float z);
void
RageMatrixSkewX(struct RageMatrix* pOut, float fAmount);
void
RageMatrixSkewY(struct RageMatrix* pOut, float fAmount);
void
RageMatrixTranslate(struct RageMatrix* pOut,
					float fTransX,
					float fTransY,
					float fTransZ);
void
RageMatrixScale(struct RageMatrix* pOut,
				float fScaleX,
				float fScaleY,
				float fScaleZ);
void
RageMatrixRotationX(struct RageMatrix* pOut, float fTheta);
void
RageMatrixRotationY(struct RageMatrix* pOut, float fTheta);
void
RageMatrixRotationZ(struct RageMatrix* pOut, float fTheta);
void
RageMatrixRotationXYZ(struct RageMatrix* pOut, float rX, float rY, float rZ);
void
RageAARotate(struct RageVector3* inret,
			 struct RageVector3 const* axis,
			 float angle);
void
RageQuatFromHPR(struct RageVector4* pOut, struct RageVector3 hpr);
void
RageQuatFromPRH(struct RageVector4* pOut, struct RageVector3 prh);
void
RageMatrixFromQuat(struct RageMatrix* pOut, const struct RageVector4& q);
void
RageQuatSlerp(struct RageVector4* pOut,
			  const struct RageVector4& from,
			  const RageVector4& to,
			  float t);
auto
RageQuatFromH(float theta) -> struct RageVector4;
auto
RageQuatFromP(float theta) -> struct RageVector4;
auto
RageQuatFromR(float theta) -> struct RageVector4;
void
RageQuatMultiply(struct RageVector4* pOut,
				 const struct RageVector4& pA,
				 const RageVector4& pB);
auto
RageLookAt(float eyex,
		   float eyey,
		   float eyez,
		   float centerx,
		   float centery,
		   float centerz,
		   float upx,
		   float upy,
		   float upz) -> struct RageMatrix;
void
RageMatrixAngles(struct RageMatrix* pOut, const struct RageVector3& angles);
void
RageMatrixTranspose(struct RageMatrix* pOut, const struct RageMatrix* pIn);

auto
RageFastSin(float x) -> float CONST_FUNCTION;
auto
RageFastCos(float x) -> float CONST_FUNCTION;

class RageQuadratic
{
  public:
	void SetFromBezier(float fC1, float fC2, float fC3, float fC4);
	void GetBezier(float& fC1, float& fC2, float& fC3, float& fC4) const;

	void SetFromCubic(float fX1, float fX2, float fX3, float fX4);

	[[nodiscard]] auto Evaluate(float fT) const -> float;
	[[nodiscard]] auto GetSlope(float fT) const -> float;

	/* Equivalent to Evaluate(0.0f) and Evaluate(1.0f), but faster: */
	[[nodiscard]] auto GetBezierStart() const -> float { return m_fD; }
	[[nodiscard]] auto GetBezierEnd() const -> float
	{
		return m_fA + m_fB + m_fC + m_fD;
	}

	void PushSelf(lua_State* L);

  private:
	float m_fA, m_fB, m_fC, m_fD;
};

class RageBezier2D
{
  public:
	void SetFromBezier(float fC1X,
					   float fC2X,
					   float fC3X,
					   float fC4X,
					   float fC1Y,
					   float fC2Y,
					   float fC3Y,
					   float fC4Y);

	void Evaluate(float fT, float* pX, float* pY) const;
	[[nodiscard]] auto EvaluateYFromX(float fX) const -> float;

	auto get_x() -> RageQuadratic& { return m_X; }
	auto get_y() -> RageQuadratic& { return m_Y; }
	void PushSelf(lua_State* L);

  private:
	RageQuadratic m_X;
	RageQuadratic m_Y;
};

#endif
