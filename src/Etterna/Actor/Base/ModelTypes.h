/* ModelTypes - Types defined in msLib.h.  C arrays converted to use std::vector
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
	std::string sName;
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
	void Load(const std::string& sTexOrIniFile);
	void Unload();
	void Update(float fDelta);

	auto GetCurrentTexture() -> std::shared_ptr<RageTexture>;

	[[nodiscard]] auto GetNumStates() const -> int;
	void SetState(int iNewState);
	[[nodiscard]] auto GetAnimationLengthSeconds() const -> float;
	void SetSecondsIntoAnimation(float fSeconds);
	[[nodiscard]] auto GetSecondsIntoAnimation() const -> float;
	auto GetTextureTranslate() -> RageVector2;

	bool m_bSphereMapped;
	BlendMode m_BlendMode;

	[[nodiscard]] auto NeedsNormals() const -> bool { return m_bSphereMapped; }

  private:
	RageVector2 m_vTexOffset;
	RageVector2 m_vTexVelocity;

	int m_iCurState;
	float m_fSecsIntoFrame;
	struct AnimatedTextureState
	{
		AnimatedTextureState(std::shared_ptr<RageTexture> pTexture_,
							 float fDelaySecs_,
							 RageVector2 vTranslate_)
		  : pTexture(pTexture_)
		  , fDelaySecs(fDelaySecs_)
		  , vTranslate(vTranslate_)
		{
		}

		std::shared_ptr<RageTexture> pTexture;
		float fDelaySecs;
		RageVector2 vTranslate;
	};

	std::vector<AnimatedTextureState> vFrames;
};

struct msMaterial
{
	int nFlags;
	std::string sName;
	RageColor Ambient;
	RageColor Diffuse;
	RageColor Specular;
	RageColor Emissive;
	float fShininess;
	float fTransparency;

	AnimatedTexture diffuse;
	AnimatedTexture alpha;

	[[nodiscard]] auto NeedsNormals() const -> bool
	{
		return diffuse.NeedsNormals() || alpha.NeedsNormals();
	}
};

struct msPositionKey
{
	float fTime{};
	RageVector3 Position;
};

struct msRotationKey
{
	float fTime{};
	RageVector4 Rotation;
};

struct msBone
{
	int nFlags;
	std::string sName;
	std::string sParentName;
	RageVector3 Position;
	RageVector3 Rotation;

	std::vector<msPositionKey> PositionKeys;
	std::vector<msRotationKey> RotationKeys;
};

struct msAnimation
{
	[[nodiscard]] auto FindBoneByName(const std::string& sName) const -> int
	{
		for (unsigned i = 0; i < Bones.size(); i++) {
			if (Bones[i].sName == sName) {
				return i;
			}
		}
		return -1;
	}

	auto LoadMilkshapeAsciiBones(const std::string& sAniName, std::string sPath)
	  -> bool;

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
