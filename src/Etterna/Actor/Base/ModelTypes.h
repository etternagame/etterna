﻿/* ModelTypes - Types defined in msLib.h.  C arrays converted to use std::vector
 */

#ifndef MODEL_TYPES_H
#define MODEL_TYPES_H

#include "RageUtil/Misc/RageTypes.h"

struct msTriangle
{
	uint16_t nVertexIndices[3];
};

struct msMesh
{
	RString sName;
	char nMaterialIndex;

	std::vector<RageModelVertex> Vertices;

	// OPTIMIZATION: If all verts in a mesh are transformed by the same bone,
	// then send the transform to the graphics card for the whole mesh instead
	// of transforming each vertex on the CPU;
	char m_iBoneIndex; // -1 = no bone

	std::vector<msTriangle> Triangles;
};

class RageTexture;

// merge this into Sprite?
class AnimatedTexture
{
  public:
	AnimatedTexture();
	~AnimatedTexture();

	void LoadBlank();
	void Load(const RString& sTexOrIniFile);
	void Unload();
	void Update(float fDelta);

	RageTexture* GetCurrentTexture();

	int GetNumStates() const;
	void SetState(int iNewState);
	float GetAnimationLengthSeconds() const;
	void SetSecondsIntoAnimation(float fSeconds);
	float GetSecondsIntoAnimation() const;
	RageVector2 GetTextureTranslate();

	bool m_bSphereMapped;
	BlendMode m_BlendMode;

	bool NeedsNormals() const { return m_bSphereMapped; }

  private:
	RageVector2 m_vTexOffset;
	RageVector2 m_vTexVelocity;

	int m_iCurState;
	float m_fSecsIntoFrame;
	struct AnimatedTextureState
	{
		AnimatedTextureState(RageTexture* pTexture_,
							 float fDelaySecs_,
							 RageVector2 vTranslate_)
		  : pTexture(pTexture_)
		  , fDelaySecs(fDelaySecs_)
		  , vTranslate(vTranslate_)
		{
		}

		RageTexture* pTexture;
		float fDelaySecs;
		RageVector2 vTranslate;
	};
	std::vector<AnimatedTextureState> vFrames;
};

struct msMaterial
{
	int nFlags;
	RString sName;
	RageColor Ambient;
	RageColor Diffuse;
	RageColor Specular;
	RageColor Emissive;
	float fShininess;
	float fTransparency;

	AnimatedTexture diffuse;
	AnimatedTexture alpha;

	bool NeedsNormals() const
	{
		return diffuse.NeedsNormals() || alpha.NeedsNormals();
	}
};

struct msPositionKey
{
	float fTime;
	RageVector3 Position;
};

struct msRotationKey
{
	float fTime;
	RageVector4 Rotation;
};

struct msBone
{
	int nFlags;
	RString sName;
	RString sParentName;
	RageVector3 Position;
	RageVector3 Rotation;

	std::vector<msPositionKey> PositionKeys;
	std::vector<msRotationKey> RotationKeys;
};

struct msAnimation
{
	int FindBoneByName(const RString& sName) const
	{
		for (unsigned i = 0; i < Bones.size(); i++)
			if (Bones[i].sName == sName)
				return i;
		return -1;
	}

	bool LoadMilkshapeAsciiBones(const RString& sAniName, RString sPath);

	std::vector<msBone> Bones;
	int nTotalFrames;
};

struct myBone_t
{
	RageMatrix m_Relative;
	RageMatrix m_Absolute;
	RageMatrix m_Final;
};

#endif
