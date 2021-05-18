#include "Etterna/Globals/global.h"
#include "ActorUtil.h"
#include "Etterna/Models/Lua/LuaBinding.h"
#include "Model.h"
#include "ModelManager.h"
#include "ModelTypes.h"
#include "Etterna/Singletons/PrefsManager.h"
#include "RageUtil/Graphics/Display/RageDisplay.h"
#include "RageUtil/File/RageFile.h"
#include "RageUtil/Misc/RageMath.h"
#include "RageUtil/Graphics/RageTextureManager.h"
#include "RageUtil/Utils/RageUtil.h"
#include "Etterna/FileTypes/XmlFile.h"

#include <cstring>
#include <algorithm>

REGISTER_ACTOR_CLASS(Model);

static const float FRAMES_PER_SECOND = 30;
static const std::string DEFAULT_ANIMATION_NAME = "default";

Model::Model()
{
	m_bTextureWrapping = true;
	SetUseZBuffer(true);
	SetCullMode(CULL_BACK);
	m_pGeometry = nullptr;
	m_pCurAnimation = nullptr;
	m_fDefaultAnimationRate = 1;
	m_fCurAnimationRate = 1;
	m_bLoop = true;
	m_bDrawCelShaded = false;
	m_pTempGeometry = nullptr;
	m_animation_length_seconds = 0.f;
	m_fCurFrame = 0.f;
}

Model::~Model()
{
	Clear();
}

void
Model::Clear()
{
	if (m_pGeometry != nullptr) {
		MODELMAN->UnloadModel(m_pGeometry);
		m_pGeometry = nullptr;
	}
	m_vpBones.clear();
	m_Materials.clear();
	m_mapNameToAnimation.clear();
	m_pCurAnimation = nullptr;
	RecalcAnimationLengthSeconds();

	if (m_pTempGeometry != nullptr)
		DISPLAY->DeleteCompiledGeometry(m_pTempGeometry);
}

void
Model::Load(const std::string& sFile)
{
	if (sFile.empty())
		return;

	const auto sExt = make_lower(GetExtension(sFile));
	if (sExt == "txt")
		LoadMilkshapeAscii(sFile);
	RecalcAnimationLengthSeconds();
}

#define THROW                                                                  \
	RageException::Throw("Parse error in \"%s\" at line %d: \"%s\".",          \
						 sPath.c_str(),                                        \
						 iLineNum,                                             \
						 sLine.c_str())

// TODO: Move MS3D loading into its own class. - Colby
void
Model::LoadMilkshapeAscii(const std::string& sPath)
{
	LoadPieces(sPath, sPath, sPath);
}

void
Model::LoadPieces(const std::string& sMeshesPath,
				  const std::string& sMaterialsPath,
				  const std::string& sBonesPath)
{
	Clear();

	// TRICKY: Load materials before geometry so we can figure out whether the
	// materials require normals.
	LoadMaterialsFromMilkshapeAscii(sMaterialsPath);

	ASSERT(m_pGeometry == nullptr);
	m_pGeometry =
	  MODELMAN->LoadMilkshapeAscii(sMeshesPath, this->MaterialsNeedNormals());

	// Validate material indices.
	for (auto& m_Meshe : m_pGeometry->m_Meshes) {
		const msMesh* pMesh = &m_Meshe;

		if (pMesh->nMaterialIndex >= static_cast<int>(m_Materials.size()))
			RageException::Throw("Model \"%s\" mesh \"%s\" references material "
								 "index %i, but there are only %i materials.",
								 sMeshesPath.c_str(),
								 pMesh->sName.c_str(),
								 pMesh->nMaterialIndex,
								 static_cast<int>(m_Materials.size()));
	}

	if (LoadMilkshapeAsciiBones(DEFAULT_ANIMATION_NAME, sBonesPath))
		PlayAnimation(DEFAULT_ANIMATION_NAME);

	// Setup temp vertices (if necessary)
	if (m_pGeometry->HasAnyPerVertexBones()) {
		m_vTempMeshes = m_pGeometry->m_Meshes;
		m_pTempGeometry = DISPLAY->CreateCompiledGeometry();
		m_pTempGeometry->Set(m_vTempMeshes, this->MaterialsNeedNormals());
	}
	RecalcAnimationLengthSeconds();
}

void
Model::LoadFromNode(const XNode* pNode)
{
	std::string s1, s2, s3;
	ActorUtil::GetAttrPath(pNode, "Meshes", s1);
	ActorUtil::GetAttrPath(pNode, "Materials", s2);
	ActorUtil::GetAttrPath(pNode, "Bones", s3);
	if (!s1.empty() || !s2.empty() || !s3.empty()) {
		ASSERT(!s1.empty() && !s2.empty() && !s3.empty());
		LoadPieces(s1, s2, s3);
	}

	Actor::LoadFromNode(pNode);
	RecalcAnimationLengthSeconds();
}

void
Model::LoadMaterialsFromMilkshapeAscii(const std::string& _sPath)
{
	auto sPath = _sPath;

	FixSlashesInPlace(sPath);
	const auto sDir = Dirname(sPath);

	RageFile f;
	if (!f.Open(sPath))
		RageException::Throw(
		  "Model::LoadMilkshapeAscii Could not open \"%s\": %s",
		  sPath.c_str(),
		  f.GetError().c_str());

	std::string sLine;
	auto iLineNum = 0;

	while (f.GetLine(sLine) > 0) {
		iLineNum++;

		if (!strncmp(sLine.c_str(), "//", 2))
			continue;

		int nFrame;
		if (sscanf(sLine.c_str(), "Frames: %d", &nFrame) == 1) {
			// ignore
			// m_pModel->nTotalFrames = nFrame;
		}
		if (sscanf(sLine.c_str(), "Frame: %d", &nFrame) == 1) {
			// ignore
			// m_pModel->nFrame = nFrame;
		}

		// materials
		auto nNumMaterials = 0;
		if (sscanf(sLine.c_str(), "Materials: %d", &nNumMaterials) == 1) {
			m_Materials.resize(nNumMaterials);

			char szName[256];

			for (auto i = 0; i < nNumMaterials; i++) {
				auto& Material = m_Materials[i];

				// name
				if (f.GetLine(sLine) <= 0)
					THROW;
				if (sscanf(sLine.c_str(), "\"%255[^\"]\"", szName) != 1)
					THROW;
				Material.sName = szName;

				// ambient
				if (f.GetLine(sLine) <= 0)
					THROW;
				RageVector4 Ambient;
				if (sscanf(sLine.c_str(),
						   "%f %f %f %f",
						   &Ambient[0],
						   &Ambient[1],
						   &Ambient[2],
						   &Ambient[3]) != 4)
					THROW;
				memcpy(&Material.Ambient, &Ambient, sizeof(Material.Ambient));

				// diffuse
				if (f.GetLine(sLine) <= 0)
					THROW;
				RageVector4 Diffuse;
				if (sscanf(sLine.c_str(),
						   "%f %f %f %f",
						   &Diffuse[0],
						   &Diffuse[1],
						   &Diffuse[2],
						   &Diffuse[3]) != 4)
					THROW;
				memcpy(&Material.Diffuse, &Diffuse, sizeof(Material.Diffuse));

				// specular
				if (f.GetLine(sLine) <= 0)
					THROW;
				RageVector4 Specular;
				if (sscanf(sLine.c_str(),
						   "%f %f %f %f",
						   &Specular[0],
						   &Specular[1],
						   &Specular[2],
						   &Specular[3]) != 4)
					THROW;
				memcpy(
				  &Material.Specular, &Specular, sizeof(Material.Specular));

				// emissive
				if (f.GetLine(sLine) <= 0)
					THROW;
				RageVector4 Emissive;
				if (sscanf(sLine.c_str(),
						   "%f %f %f %f",
						   &Emissive[0],
						   &Emissive[1],
						   &Emissive[2],
						   &Emissive[3]) != 4)
					THROW;
				memcpy(
				  &Material.Emissive, &Emissive, sizeof(Material.Emissive));

				// shininess
				if (f.GetLine(sLine) <= 0)
					THROW;
				float fShininess;
				if (!StringConversion::FromString(sLine, fShininess))
					THROW;
				Material.fShininess = fShininess;

				// transparency
				if (f.GetLine(sLine) <= 0)
					THROW;
				float fTransparency;
				if (!StringConversion::FromString(sLine, fTransparency))
					THROW;
				Material.fTransparency = fTransparency;

				// diffuse texture
				if (f.GetLine(sLine) <= 0)
					THROW;
				strcpy(szName, "");
				sscanf(sLine.c_str(), "\"%255[^\"]\"", szName);
				std::string sDiffuseTexture = szName;

				if (sDiffuseTexture.empty()) {
					Material.diffuse.LoadBlank();
				} else {
					auto sTexturePath = sDir + sDiffuseTexture;
					FixSlashesInPlace(sTexturePath);
					CollapsePath(sTexturePath);
					if (!IsAFile(sTexturePath))
						RageException::Throw("\"%s\" references a texture "
											 "\"%s\" that does not exist.",
											 sPath.c_str(),
											 sTexturePath.c_str());

					Material.diffuse.Load(sTexturePath);
				}

				// alpha texture
				if (f.GetLine(sLine) <= 0)
					THROW;
				strcpy(szName, "");
				sscanf(sLine.c_str(), "\"%255[^\"]\"", szName);
				std::string sAlphaTexture = szName;

				if (sAlphaTexture.empty()) {
					Material.alpha.LoadBlank();
				} else {
					auto sTexturePath = sDir + sAlphaTexture;
					FixSlashesInPlace(sTexturePath);
					CollapsePath(sTexturePath);
					if (!IsAFile(sTexturePath))
						RageException::Throw("\"%s\" references a texture "
											 "\"%s\" that does not exist.",
											 sPath.c_str(),
											 sTexturePath.c_str());

					Material.alpha.Load(sTexturePath);
				}
			}
		}
	}
}

bool
Model::LoadMilkshapeAsciiBones(const std::string& sAniName,
							   const std::string& sPath)
{
	m_mapNameToAnimation[sAniName] = msAnimation();
	auto& Animation = m_mapNameToAnimation[sAniName];

	if (Animation.LoadMilkshapeAsciiBones(sAniName, sPath)) {
		m_mapNameToAnimation.erase(sAniName);
		return false;
	}

	return true;
}

bool
Model::EarlyAbortDraw() const
{
	return m_pGeometry == nullptr || m_pGeometry->m_Meshes.empty();
}

void
Model::DrawCelShaded()
{
	// First pass: shell. We only want the backfaces for this.
	DISPLAY->SetCelShaded(1);
	DISPLAY->SetCullMode(CULL_FRONT);
	this->SetZWrite(
	  false); // XXX: Why on earth isn't the culling working? -Colby
	this->Draw();

	// Second pass: cel shading
	DISPLAY->SetCelShaded(2);
	DISPLAY->SetCullMode(CULL_BACK);
	this->SetZWrite(true);
	this->Draw();

	DISPLAY->SetCelShaded(0);
}

void
Model::DrawPrimitives()
{
	Actor::SetGlobalRenderStates(); // set Actor-specified render states

	// Don't if we're fully transparent
	if (m_pTempState->diffuse[0].a < 0.001f && m_pTempState->glow.a < 0.001f)
		return;

	DISPLAY->Scale(1, -1, 1); // flip Y so positive is up

	//////////////////////
	// render the diffuse pass
	//////////////////////
	if (m_pTempState->diffuse[0].a > 0) {
		DISPLAY->SetTextureMode(TextureUnit_1, TextureMode_Modulate);

		for (unsigned i = 0; i < m_pGeometry->m_Meshes.size(); ++i) {
			const msMesh* pMesh = &m_pGeometry->m_Meshes[i];

			if (pMesh->nMaterialIndex != -1) // has a material
			{
				// apply material
				auto& mat = m_Materials[pMesh->nMaterialIndex];

				auto Emissive = mat.Emissive;
				auto Ambient = mat.Ambient;
				auto Diffuse = mat.Diffuse;

				Emissive *= m_pTempState->diffuse[0];
				Ambient *= m_pTempState->diffuse[0];
				Diffuse *= m_pTempState->diffuse[0];

				DISPLAY->SetMaterial(
				  Emissive, Ambient, Diffuse, mat.Specular, mat.fShininess);

				const auto vTexTranslate = mat.diffuse.GetTextureTranslate();
				if (vTexTranslate.x != 0 || vTexTranslate.y != 0) {
					DISPLAY->TexturePushMatrix();
					DISPLAY->TextureTranslate(vTexTranslate.x, vTexTranslate.y);
				}

				/* There's some common code that could be folded out here, but
				 * it seems clearer to keep it separate. */
				const auto bUseMultitexture =
				  PREFSMAN->m_bAllowMultitexture &&
				  DISPLAY->GetNumTextureUnits() >= 2;
				if (bUseMultitexture) {
					// render the diffuse texture with texture unit 1
					DISPLAY->SetTexture(
					  TextureUnit_1,
					  mat.diffuse.GetCurrentTexture()
						? mat.diffuse.GetCurrentTexture()->GetTexHandle()
						: 0);
					Actor::SetTextureRenderStates(); // set Actor-specified
													 // render states
					DISPLAY->SetSphereEnvironmentMapping(
					  TextureUnit_1, mat.diffuse.m_bSphereMapped);

					// render the additive texture with texture unit 2
					if (mat.alpha.GetCurrentTexture()) {
						DISPLAY->SetTexture(
						  TextureUnit_2,
						  mat.alpha.GetCurrentTexture()
							? mat.alpha.GetCurrentTexture()->GetTexHandle()
							: 0);
						Actor::SetTextureRenderStates(); // set Actor-specified
														 // render states
						DISPLAY->SetSphereEnvironmentMapping(
						  TextureUnit_2, mat.alpha.m_bSphereMapped);
						DISPLAY->SetTextureMode(TextureUnit_2, TextureMode_Add);
						DISPLAY->SetTextureFiltering(TextureUnit_2, true);
					} else {
						DISPLAY->SetTexture(TextureUnit_2, 0);

						// set current texture back to 0 or else texture
						// transform applied above  isn't used. Why?!?
						DISPLAY->SetTexture(
						  TextureUnit_1,
						  mat.diffuse.GetCurrentTexture()
							? mat.diffuse.GetCurrentTexture()->GetTexHandle()
							: 0);
					}

					// go
					DrawMesh(i);

					// Turn off environment mapping on tex unit 0.
					DISPLAY->SetSphereEnvironmentMapping(TextureUnit_1, false);
				} else {
					// render the diffuse texture
					DISPLAY->SetTexture(
					  TextureUnit_1,
					  mat.diffuse.GetCurrentTexture()
						? mat.diffuse.GetCurrentTexture()->GetTexHandle()
						: 0);
					Actor::SetTextureRenderStates(); // set Actor-specified
													 // render states
					DISPLAY->SetSphereEnvironmentMapping(
					  TextureUnit_1, mat.diffuse.m_bSphereMapped);
					DrawMesh(i);

					// render the additive texture
					if (mat.alpha.GetCurrentTexture()) {
						DISPLAY->SetTexture(
						  TextureUnit_1,
						  mat.alpha.GetCurrentTexture()
							? mat.alpha.GetCurrentTexture()->GetTexHandle()
							: 0);
						Actor::SetTextureRenderStates(); // set Actor-specified
														 // render states

						DISPLAY->SetSphereEnvironmentMapping(
						  TextureUnit_1, mat.alpha.m_bSphereMapped);
						// UGLY: This overrides the Actor's BlendMode.
						DISPLAY->SetBlendMode(BLEND_ADD);
						DISPLAY->SetTextureFiltering(TextureUnit_1, true);
						DrawMesh(i);
					}
				}

				if (vTexTranslate.x != 0 || vTexTranslate.y != 0)
					DISPLAY->TexturePopMatrix();
			} else {
				static const RageColor emissive(0, 0, 0, 0);
				static const RageColor ambient(0.2f, 0.2f, 0.2f, 1);
				static const RageColor diffuse(0.7f, 0.7f, 0.7f, 1);
				static const RageColor specular(0.2f, 0.2f, 0.2f, 1);
				static const float shininess = 1;
				DISPLAY->SetMaterial(
				  emissive, ambient, diffuse, specular, shininess);
				DISPLAY->SetSphereEnvironmentMapping(TextureUnit_1, false);
				DrawMesh(i);
			}

			DISPLAY->SetSphereEnvironmentMapping(TextureUnit_1, false);
			DISPLAY->SetBlendMode(BLEND_NORMAL);
		}
	}

	// render the glow pass
	if (m_pTempState->glow.a > 0.0001f) {
		DISPLAY->SetTextureMode(TextureUnit_1, TextureMode_Glow);

		for (unsigned i = 0; i < m_pGeometry->m_Meshes.size(); ++i) {
			const msMesh* pMesh = &m_pGeometry->m_Meshes[i];

			// apply material
			auto emissive = RageColor(0, 0, 0, 0);
			auto ambient = RageColor(0, 0, 0, 0);
			auto diffuse = m_pTempState->glow;
			auto specular = RageColor(0, 0, 0, 0);
			const float shininess = 1;

			DISPLAY->SetMaterial(
			  emissive, ambient, diffuse, specular, shininess);

			if (pMesh->nMaterialIndex != -1) {
				auto& mat = m_Materials[pMesh->nMaterialIndex];
				DISPLAY->SetTexture(
				  TextureUnit_1,
				  mat.diffuse.GetCurrentTexture()
					? mat.diffuse.GetCurrentTexture()->GetTexHandle()
					: 0);
				Actor::SetTextureRenderStates(); // set Actor-specified render
												 // states
			}

			DrawMesh(i);
		}
	}
}

void
Model::DrawMesh(int i) const
{
	const msMesh* pMesh = &m_pGeometry->m_Meshes[i];

	// apply mesh-specific bone (if any)
	if (pMesh->m_iBoneIndex != -1) {
		DISPLAY->PushMatrix();

		const auto& mat = m_vpBones[pMesh->m_iBoneIndex].m_Final;
		DISPLAY->PreMultMatrix(mat);
	}

	// Draw it
	const RageCompiledGeometry* TempGeometry =
	  m_pTempGeometry != nullptr ? m_pTempGeometry
								 : m_pGeometry->m_pCompiledGeometry;
	DISPLAY->DrawCompiledGeometry(TempGeometry, i, m_pGeometry->m_Meshes);

	if (pMesh->m_iBoneIndex != -1)
		DISPLAY->PopMatrix();
}

void
Model::SetDefaultAnimation(const std::string& sAnimation, float fPlayRate)
{
	m_sDefaultAnimation = sAnimation;
	m_fDefaultAnimationRate = fPlayRate;
}

void
Model::PlayAnimation(const std::string& sAniName, float fPlayRate)
{
	if (m_mapNameToAnimation.find(sAniName) == m_mapNameToAnimation.end())
		return;

	const msAnimation* pNewAnimation = &m_mapNameToAnimation[sAniName];

	m_fCurFrame = 0;
	m_fCurAnimationRate = fPlayRate;

	if (m_pCurAnimation == pNewAnimation)
		return;

	m_pCurAnimation = pNewAnimation;

	// setup bones
	m_vpBones.resize(m_pCurAnimation->Bones.size());

	for (unsigned i = 0; i < m_pCurAnimation->Bones.size(); i++) {
		const auto* const pBone = &m_pCurAnimation->Bones[i];
		const auto& vRot = pBone->Rotation;

		RageMatrixAngles(&m_vpBones[i].m_Relative, vRot);

		m_vpBones[i].m_Relative.m[3][0] = pBone->Position[0];
		m_vpBones[i].m_Relative.m[3][1] = pBone->Position[1];
		m_vpBones[i].m_Relative.m[3][2] = pBone->Position[2];

		const auto nParentBone =
		  m_pCurAnimation->FindBoneByName(pBone->sParentName);
		if (nParentBone != -1) {
			RageMatrixMultiply(&m_vpBones[i].m_Absolute,
							   &m_vpBones[nParentBone].m_Absolute,
							   &m_vpBones[i].m_Relative);
		} else {
			m_vpBones[i].m_Absolute = m_vpBones[i].m_Relative;
		}
		m_vpBones[i].m_Final = m_vpBones[i].m_Absolute;
	}

	// subtract out the bone's resting position
	for (auto& m_Meshe : m_pGeometry->m_Meshes) {
		auto* pMesh = &m_Meshe;
		auto& Vertices = pMesh->Vertices;
		for (auto& Vertice : Vertices) {
			// int iBoneIndex = (pMesh->m_iBoneIndex!=-1) ? pMesh->m_iBoneIndex
			// : bone;
			auto& pos = Vertice.p;
			const auto bone = Vertice.bone;
			if (bone != -1) {
				pos[0] -= m_vpBones[bone].m_Absolute.m[3][0];
				pos[1] -= m_vpBones[bone].m_Absolute.m[3][1];
				pos[2] -= m_vpBones[bone].m_Absolute.m[3][2];

				RageVector3 vTmp;

				RageMatrix inverse;
				RageMatrixTranspose(
				  &inverse,
				  &m_vpBones[bone]
					 .m_Absolute); // transpose = inverse for rotation matrices
				RageVec3TransformNormal(&vTmp, &pos, &inverse);

				pos = vTmp;
			}
		}
	}

	// Set up m_vpBones, just in case we're drawn without being Update()d.
	SetBones(m_pCurAnimation, m_fCurFrame, m_vpBones);
	UpdateTempGeometry();
}

void
Model::SetPosition(float fSeconds)
{
	m_fCurFrame = FRAMES_PER_SECOND * fSeconds;
	m_fCurFrame = std::clamp(
	  m_fCurFrame, 0.F, static_cast<float>(m_pCurAnimation->nTotalFrames));
}

void
Model::AdvanceFrame(float fDeltaTime)
{
	if (m_pGeometry == nullptr || m_pGeometry->m_Meshes.empty() ||
		!m_pCurAnimation) {
		return; // bail early
	}

	// LOG->Trace( "m_fCurFrame = %f", m_fCurFrame );

	m_fCurFrame += FRAMES_PER_SECOND * fDeltaTime * m_fCurAnimationRate;
	if (m_fCurFrame < 0 || m_fCurFrame >= m_pCurAnimation->nTotalFrames) {
		if (!m_sDefaultAnimation.empty()) {
			this->PlayAnimation(m_sDefaultAnimation, m_fDefaultAnimationRate);
			/* XXX: add to m_fCurFrame the wrapover from the previous
			 * m_fCurFrame-m_pCurAnimation->nTotalFrames, so it doesn't skip */
		} else if (m_bLoop)
			wrap(m_fCurFrame,
				 static_cast<float>(m_pCurAnimation->nTotalFrames));
		else
			m_fCurFrame =
			  std::clamp(m_fCurFrame,
						 0.F,
						 static_cast<float>(m_pCurAnimation->nTotalFrames));
	}

	SetBones(m_pCurAnimation, m_fCurFrame, m_vpBones);
	UpdateTempGeometry();
}

void
Model::SetBones(const msAnimation* pAnimation,
				float fFrame,
				vector<myBone_t>& vpBones)
{
	for (size_t i = 0; i < pAnimation->Bones.size(); ++i) {
		const auto* pBone = &pAnimation->Bones[i];
		if (pBone->PositionKeys.empty() && pBone->RotationKeys.empty()) {
			vpBones[i].m_Final = vpBones[i].m_Absolute;
			continue;
		}

		// search for the adjacent position keys
		const msPositionKey *pLastPositionKey = nullptr,
							*pThisPositionKey = nullptr;
		for (const auto& PositionKey : pBone->PositionKeys) {
			const auto* const pPositionKey = &PositionKey;
			if (pPositionKey->fTime >= fFrame) {
				pThisPositionKey = pPositionKey;
				break;
			}
			pLastPositionKey = pPositionKey;
		}

		RageVector3 vPos;
		if (pLastPositionKey != nullptr && pThisPositionKey != nullptr) {
			const auto s = SCALE(
			  fFrame, pLastPositionKey->fTime, pThisPositionKey->fTime, 0, 1);
			vPos =
			  pLastPositionKey->Position +
			  (pThisPositionKey->Position - pLastPositionKey->Position) * s;
		} else if (pLastPositionKey == nullptr && pThisPositionKey != nullptr)
			vPos = pThisPositionKey->Position;
		else if (pThisPositionKey == nullptr && pLastPositionKey != nullptr)
			vPos = pLastPositionKey->Position;

		// search for the adjacent rotation keys
		const msRotationKey *pLastRotationKey = nullptr,
							*pThisRotationKey = nullptr;
		for (const auto& RotationKey : pBone->RotationKeys) {
			const auto* const pRotationKey = &RotationKey;
			if (pRotationKey->fTime >= fFrame) {
				pThisRotationKey = pRotationKey;
				break;
			}
			pLastRotationKey = pRotationKey;
		}

		RageVector4 vRot;
		if (pLastRotationKey != nullptr && pThisRotationKey != nullptr) {
			const auto s = SCALE(
			  fFrame, pLastRotationKey->fTime, pThisRotationKey->fTime, 0, 1);
			RageQuatSlerp(
			  &vRot, pLastRotationKey->Rotation, pThisRotationKey->Rotation, s);
		} else if (pLastRotationKey == nullptr && pThisRotationKey != nullptr) {
			vRot = pThisRotationKey->Rotation;
		} else if (pThisRotationKey == nullptr && pLastRotationKey != nullptr) {
			vRot = pLastRotationKey->Rotation;
		}

		RageMatrix m;
		RageMatrixIdentity(&m);
		RageMatrixFromQuat(&m, vRot);
		m.m[3][0] = vPos[0];
		m.m[3][1] = vPos[1];
		m.m[3][2] = vPos[2];

		RageMatrix RelativeFinal;
		RageMatrixMultiply(&RelativeFinal, &vpBones[i].m_Relative, &m);

		const auto iParentBone = pAnimation->FindBoneByName(pBone->sParentName);
		if (iParentBone == -1)
			vpBones[i].m_Final = RelativeFinal;
		else
			RageMatrixMultiply(&vpBones[i].m_Final,
							   &vpBones[iParentBone].m_Final,
							   &RelativeFinal);
	}
}

void
Model::UpdateTempGeometry()
{
	if (m_pGeometry == nullptr || m_pTempGeometry == nullptr)
		return;

	for (unsigned i = 0; i < m_pGeometry->m_Meshes.size(); ++i) {
		const auto& origMesh = m_pGeometry->m_Meshes[i];
		auto& tempMesh = m_vTempMeshes[i];
		const auto& origVertices = origMesh.Vertices;
		auto& tempVertices = tempMesh.Vertices;
		for (unsigned j = 0; j < origVertices.size(); j++) {
			auto& tempPos = tempVertices[j].p;
			auto& tempNormal = tempVertices[j].n;
			const auto& originalPos = origVertices[j].p;
			const auto& originalNormal = origVertices[j].n;
			const auto bone = origVertices[j].bone;

			if (bone == -1) {
				tempNormal = originalNormal;
				tempPos = originalPos;
			} else {
				RageVec3TransformNormal(
				  &tempNormal, &originalNormal, &m_vpBones[bone].m_Final);
				RageVec3TransformCoord(
				  &tempPos, &originalPos, &m_vpBones[bone].m_Final);
			}
		}
	}

	// send the new vertices to the graphics card
	m_pTempGeometry->Change(m_vTempMeshes);
}

void
Model::Update(float fDelta)
{
	Actor::Update(fDelta);
	AdvanceFrame(fDelta);

	for (auto& m_Material : m_Materials) {
		m_Material.diffuse.Update(fDelta);
		m_Material.alpha.Update(fDelta);
	}
}

int
Model::GetNumStates() const
{
	auto iMaxStates = 0;
	for (const auto& m : m_Materials)
		iMaxStates = std::max(iMaxStates, m.diffuse.GetNumStates());
	return iMaxStates;
}

void
Model::SetState(int iNewState)
{
	for (auto& m : m_Materials) {
		m.diffuse.SetState(iNewState);
		m.alpha.SetState(iNewState);
	}
}

void
Model::RecalcAnimationLengthSeconds()
{
	m_animation_length_seconds = 0;
	for (auto& m : m_Materials) {
		m_animation_length_seconds = std::max(
		  m_animation_length_seconds, m.diffuse.GetAnimationLengthSeconds());
	}
}

void
Model::SetSecondsIntoAnimation(float fSeconds)
{
	for (auto& m : m_Materials) {
		m.diffuse.SetSecondsIntoAnimation(fSeconds);
		m.alpha.SetSecondsIntoAnimation(fSeconds);
	}
}

bool
Model::MaterialsNeedNormals() const
{
	for (auto& m : m_Materials) {
		if (m.NeedsNormals())
			return true;
	}
	return false;
}

// lua start
#include "Etterna/Models/Lua/LuaBinding.h"

/** @brief Allow Lua to have access to the Model. */
class LunaModel : public Luna<Model>
{
  public:
	static int position(T* p, lua_State* L)
	{
		p->SetPosition(FArg(1));
		COMMON_RETURN_SELF;
	}
	static int playanimation(T* p, lua_State* L)
	{
		p->PlayAnimation(SArg(1), FArg(2));
		COMMON_RETURN_SELF;
	}
	static int SetDefaultAnimation(T* p, lua_State* L)
	{
		p->SetDefaultAnimation(SArg(1), FArg(2));
		COMMON_RETURN_SELF;
	}
	static int GetDefaultAnimation(T* p, lua_State* L)
	{
		lua_pushstring(L, p->GetDefaultAnimation().c_str());
		return 1;
	}
	static int loop(T* p, lua_State* L)
	{
		p->SetLoop(BArg(1));
		COMMON_RETURN_SELF;
	}
	static int rate(T* p, lua_State* L)
	{
		p->SetRate(FArg(1));
		COMMON_RETURN_SELF;
	}
	static int GetNumStates(T* p, lua_State* L)
	{
		lua_pushnumber(L, p->GetNumStates());
		return 1;
	}
	// static int CelShading( T* p, lua_State *L )		{
	// p->SetCelShading(BArg(1)); COMMON_RETURN_SELF; }

	LunaModel()
	{
		ADD_METHOD(position);
		ADD_METHOD(playanimation);
		ADD_METHOD(SetDefaultAnimation);
		ADD_METHOD(GetDefaultAnimation);
		ADD_METHOD(loop);
		ADD_METHOD(rate);
		// sm-ssc adds:
		ADD_METHOD(GetNumStates);
		// ADD_METHOD( CelShading );
		// LoadMilkshapeAsciiBones?
	}
};

LUA_REGISTER_DERIVED_CLASS(Model, Actor)
// lua end
