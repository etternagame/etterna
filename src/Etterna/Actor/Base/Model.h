/* Model - A 3D model. */

#ifndef MODEL_H
#define MODEL_H

#include "Actor.h"
#include "ModelTypes.h"
#include <map>
#include <vector>
#include <optional>

class RageModelGeometry;
class RageCompiledGeometry;

class Model : public Actor
{
  public:
	Model();
	~Model() override;
	[[nodiscard]] Model* Copy() const override;

	void Clear();
	void Load(const std::string& sFile);

	void LoadFromNode(const XNode* pNode) override;

	void Update(float fDelta) override;
	[[nodiscard]] bool EarlyAbortDraw() const override;
	void DrawPrimitives() override;

	[[nodiscard]] int GetNumStates() const override;
	void SetState(int iNewState) override;

	[[nodiscard]] float GetAnimationLengthSeconds() const override
	{
		return m_animation_length_seconds;
	}
	void SetSecondsIntoAnimation(float fSeconds) override;

	// Lua
	void PushSelf(lua_State* L) override;
	void PlayAnimation(const std::string& sAniName, float fPlayRate = 1);
	void SetRate(float fRate) { m_fCurAnimationRate = fRate; }
	void SetLoop(bool b) { m_bLoop = b; }
	void SetPosition(float fSeconds);
	virtual void RecalcAnimationLengthSeconds();
	[[nodiscard]] std::string GetDefaultAnimation() const
	{
		return m_sDefaultAnimation;
	};
	void SetDefaultAnimation(const std::string& sAnimation,
							 float fPlayRate = 1);
  private:
	void LoadGLTF(const std::string& path);
	void DrawGLTFModel();
	bool IsGLTFLoaded();

	void LoadPieces(const std::string& sMeshesPath,
					const std::string& sMaterialsPath,
					const std::string& sBomesPath);
	void LoadMilkshapeAscii(const std::string& sFile);
	void LoadMaterialsFromMilkshapeAscii(const std::string& sPath);
	bool LoadMilkshapeAsciiBones(const std::string& sAniName,
								 const std::string& sPath);

	void DrawCelShaded();
	void SetCelShading(bool bShading) { m_bDrawCelShaded = bShading; }
	[[nodiscard]] bool MaterialsNeedNormals() const;

	RageModelGeometry* m_pGeometry;

	float m_animation_length_seconds;
	std::vector<msMaterial> m_Materials;
	std::map<std::string, msAnimation> m_mapNameToAnimation;
	const msAnimation* m_pCurAnimation;

	static void SetBones(const msAnimation* pAnimation,
						 float fFrame,
						 std::vector<myBone_t>& vpBones);
	std::vector<myBone_t> m_vpBones;

	// If any vertex has a bone weight, then then render from m_pTempGeometry.
	// Otherwise, render directly from m_pGeometry.
	RageCompiledGeometry* m_pTempGeometry;
	void UpdateTempGeometry();

	/* Keep a copy of the mesh data only if m_pTempGeometry is in use.  The
	 * normal and position data will be changed; the rest is static and kept
	 * only to prevent making a complete copy. */
	std::vector<msMesh> m_vTempMeshes;

	void DrawMesh(int i) const;
	void AdvanceFrame(float fDeltaTime);

	float m_fCurFrame;
	std::string m_sDefaultAnimation;
	float m_fDefaultAnimationRate;
	float m_fCurAnimationRate;
	bool m_bLoop;
	bool m_bDrawCelShaded; // for Lua models

	Model& operator=(const Model& rhs);
};

#endif
