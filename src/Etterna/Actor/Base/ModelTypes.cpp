#include "Etterna/Globals/global.h"
#include "Etterna/FileTypes/IniFile.h"
#include "ModelTypes.h"
#include "RageUtil/Graphics/RageDisplay.h"
#include "RageUtil/File/RageFile.h"
#include "RageUtil/Misc/RageMath.h"
#include "RageUtil/Graphics/RageTexture.h"
#include "RageUtil/Graphics/RageTextureManager.h"
#include "RageUtil/Utils/RageUtil.h"

#include <cstring>
#include <algorithm>

#define MS_MAX_NAME 32

AnimatedTexture::AnimatedTexture()
{
	m_iCurState = 0;
	m_fSecsIntoFrame = 0;
	m_bSphereMapped = false;
	m_vTexOffset = RageVector2(0, 0);
	m_vTexVelocity = RageVector2(0, 0);
	m_BlendMode = BLEND_NORMAL;
}

AnimatedTexture::~AnimatedTexture()
{
	Unload();
}

void
AnimatedTexture::LoadBlank()
{
	const AnimatedTextureState state(nullptr, 1, RageVector2(0, 0));
	vFrames.push_back(state);
}

void
AnimatedTexture::Load(const std::string& sTexOrIniPath)
{
	ASSERT(vFrames.empty()); // don't load more than once

	m_bSphereMapped = sTexOrIniPath.find("sphere") != std::string::npos;
	if (sTexOrIniPath.find("add") != std::string::npos)
		m_BlendMode = BLEND_ADD;
	else
		m_BlendMode = BLEND_NORMAL;

	if (CompareNoCase(GetExtension(sTexOrIniPath), "ini") == 0) {
		IniFile ini;
		if (!ini.ReadFile(sTexOrIniPath))
			RageException::Throw("Error reading \"%s\": %s",
								 sTexOrIniPath.c_str(),
								 ini.GetError().c_str());

		const XNode* pAnimatedTexture = ini.GetChild("AnimatedTexture");
		if (pAnimatedTexture == nullptr)
			RageException::Throw("The animated texture file \"%s\" doesn't "
								 "contain a section called "
								 "\"AnimatedTexture\".",
								 sTexOrIniPath.c_str());

		pAnimatedTexture->GetAttrValue("TexVelocityX", m_vTexVelocity.x);
		pAnimatedTexture->GetAttrValue("TexVelocityY", m_vTexVelocity.y);
		pAnimatedTexture->GetAttrValue("TexOffsetX", m_vTexOffset.x);
		pAnimatedTexture->GetAttrValue("TexOffsetY", m_vTexOffset.y);

		for (auto i = 0; i < 1000; i++) {
			auto sFileKey = ssprintf("Frame%04d", i);
			auto sDelayKey = ssprintf("Delay%04d", i);

			std::string sFileName;
			float fDelay = 0;
			if (pAnimatedTexture->GetAttrValue(sFileKey, sFileName) &&
				pAnimatedTexture->GetAttrValue(sDelayKey, fDelay)) {
				auto sTranslateXKey = ssprintf("TranslateX%04d", i);
				auto sTranslateYKey = ssprintf("TranslateY%04d", i);

				RageVector2 vOffset(0, 0);
				pAnimatedTexture->GetAttrValue(sTranslateXKey, vOffset.x);
				pAnimatedTexture->GetAttrValue(sTranslateYKey, vOffset.y);

				RageTextureID ID;
				ID.filename = Dirname(sTexOrIniPath) + sFileName;
				ID.bStretch = true;
				ID.bHotPinkColorKey = true;
				ID.bMipMaps = true; // use mipmaps in Models
				AnimatedTextureState state(
				  TEXTUREMAN->LoadTexture(ID), fDelay, vOffset);
				vFrames.push_back(state);
			} else {
				break;
			}
		}
	} else {
		RageTextureID ID;
		ID.filename = sTexOrIniPath;
		ID.bHotPinkColorKey = true;
		ID.bStretch = true;
		ID.bMipMaps = true; // use mipmaps in Models
		AnimatedTextureState state(
		  TEXTUREMAN->LoadTexture(ID), 1, RageVector2(0, 0));
		vFrames.push_back(state);
	}
}

void
AnimatedTexture::Update(float fDelta)
{
	if (vFrames.empty())
		return;
	ASSERT(m_iCurState < static_cast<int>(vFrames.size()));
	m_fSecsIntoFrame += fDelta;
	if (m_fSecsIntoFrame > vFrames[m_iCurState].fDelaySecs) {
		m_fSecsIntoFrame -= vFrames[m_iCurState].fDelaySecs;
		m_iCurState = (m_iCurState + 1) % vFrames.size();
	}
}

std::shared_ptr<RageTexture>
AnimatedTexture::GetCurrentTexture()
{
	if (vFrames.empty())
		return nullptr;
	ASSERT(m_iCurState < static_cast<int>(vFrames.size()));
	return vFrames[m_iCurState].pTexture;
}

int
AnimatedTexture::GetNumStates() const
{
	return vFrames.size();
}

void
AnimatedTexture::SetState(int iState)
{
	CLAMP(iState, 0, GetNumStates() - 1);
	m_iCurState = iState;
}

float
AnimatedTexture::GetAnimationLengthSeconds() const
{
	float fTotalSeconds = 0;
	for (const auto& ats : vFrames)
		fTotalSeconds += ats.fDelaySecs;
	return fTotalSeconds;
}

void
AnimatedTexture::SetSecondsIntoAnimation(float fSeconds)
{
	fSeconds = fmodf(fSeconds, GetAnimationLengthSeconds());

	m_iCurState = 0;
	for (unsigned i = 0; i < vFrames.size(); i++) {
		auto& ats = vFrames[i];
		if (fSeconds >= ats.fDelaySecs) {
			fSeconds -= ats.fDelaySecs;
			m_iCurState = i + 1;
		} else {
			break;
		}
	}
	m_fSecsIntoFrame = fSeconds; // remainder
}

float
AnimatedTexture::GetSecondsIntoAnimation() const
{
	float fSeconds = 0;

	for (unsigned i = 0; i < vFrames.size(); i++) {
		const auto& ats = vFrames[i];
		if (static_cast<int>(i) >= m_iCurState)
			break;

		fSeconds += ats.fDelaySecs;
	}
	fSeconds += m_fSecsIntoFrame;
	return fSeconds;
}

void
AnimatedTexture::Unload()
{
	for (auto& vFrame : vFrames)
		TEXTUREMAN->UnloadTexture(vFrame.pTexture);
	vFrames.clear();
	m_iCurState = 0;
	m_fSecsIntoFrame = 0;
}

RageVector2
AnimatedTexture::GetTextureTranslate()
{
	const auto fPercentIntoAnimation =
	  GetSecondsIntoAnimation() / GetAnimationLengthSeconds();
	auto v = m_vTexVelocity * fPercentIntoAnimation + m_vTexOffset;

	if (vFrames.empty())
		return v;

	ASSERT(m_iCurState < static_cast<int>(vFrames.size()));
	v += vFrames[m_iCurState].vTranslate;

	return v;
}

#define THROW                                                                  \
	RageException::Throw("Parse error in \"%s\" at line %d: \"%s\".",          \
						 sPath.c_str(),                                        \
						 iLineNum,                                             \
						 sLine.c_str())

bool
msAnimation::LoadMilkshapeAsciiBones(const std::string& sAniName,
									 std::string sPath)
{
	FixSlashesInPlace(sPath);
	const auto sDir = Dirname(sPath);

	RageFile f;
	if (!f.Open(sPath))
		RageException::Throw("Model:: Could not open \"%s\": %s",
							 sPath.c_str(),
							 f.GetError().c_str());

	std::string sLine;
	auto iLineNum = 0;

	auto& Animation = *this;

	const auto bLoaded = false;
	while (f.GetLine(sLine) > 0) {
		iLineNum++;

		if (!strncmp(sLine.c_str(), "//", 2))
			continue;

		// bones
		auto nNumBones = 0;
		if (sscanf(sLine.c_str(), "Bones: %d", &nNumBones) != 1)
			continue;

		char szName[MS_MAX_NAME];

		Animation.Bones.resize(nNumBones);

		for (auto i = 0; i < nNumBones; i++) {
			auto& Bone = Animation.Bones[i];

			// name
			if (f.GetLine(sLine) <= 0)
				THROW;
			if (sscanf(sLine.c_str(), "\"%31[^\"]\"", szName) != 1)
				THROW;
			Bone.sName = szName;

			// parent
			if (f.GetLine(sLine) <= 0)
				THROW;
			strcpy(szName, "");
			sscanf(sLine.c_str(), "\"%31[^\"]\"", szName);

			Bone.sParentName = szName;

			// flags, position, rotation
			RageVector3 Position, Rotation;
			if (f.GetLine(sLine) <= 0)
				THROW;

			int nFlags;
			if (sscanf(sLine.c_str(),
					   "%d %f %f %f %f %f %f",
					   &nFlags,
					   &Position[0],
					   &Position[1],
					   &Position[2],
					   &Rotation[0],
					   &Rotation[1],
					   &Rotation[2]) != 7) {
				THROW;
			}
			Rotation = RadianToDegree(Rotation);

			Bone.nFlags = nFlags;
			memcpy(&Bone.Position, &Position, sizeof(Bone.Position));
			memcpy(&Bone.Rotation, &Rotation, sizeof(Bone.Rotation));

			// position key count
			if (f.GetLine(sLine) <= 0)
				THROW;
			auto nNumPositionKeys = 0;
			if (sscanf(sLine.c_str(), "%d", &nNumPositionKeys) != 1)
				THROW;

			Bone.PositionKeys.resize(nNumPositionKeys);

			for (auto j = 0; j < nNumPositionKeys; ++j) {
				if (f.GetLine(sLine) <= 0)
					THROW;

				float fTime;
				if (sscanf(sLine.c_str(),
						   "%f %f %f %f",
						   &fTime,
						   &Position[0],
						   &Position[1],
						   &Position[2]) != 4)
					THROW;

				msPositionKey key;
				key.fTime = fTime;
				key.Position =
				  RageVector3(Position[0], Position[1], Position[2]);
				Bone.PositionKeys[j] = key;
			}

			// rotation key count
			if (f.GetLine(sLine) <= 0)
				THROW;
			auto nNumRotationKeys = 0;
			if (sscanf(sLine.c_str(), "%d", &nNumRotationKeys) != 1)
				THROW;

			Bone.RotationKeys.resize(nNumRotationKeys);

			for (auto j = 0; j < nNumRotationKeys; ++j) {
				if (f.GetLine(sLine) <= 0)
					THROW;

				float fTime;
				if (sscanf(sLine.c_str(),
						   "%f %f %f %f",
						   &fTime,
						   &Rotation[0],
						   &Rotation[1],
						   &Rotation[2]) != 4)
					THROW;
				Rotation = RadianToDegree(Rotation);

				msRotationKey key;
				key.fTime = fTime;
				Rotation = RageVector3(Rotation[0], Rotation[1], Rotation[2]);
				RageQuatFromHPR(&key.Rotation, Rotation);
				Bone.RotationKeys[j] = key;
			}
		}

		// Ignore "Frames:" in file.  Calculate it ourself
		Animation.nTotalFrames = 0;
		for (auto i = 0; i < static_cast<int>(Animation.Bones.size()); i++) {
			auto& Bone = Animation.Bones[i];
			for (auto& PositionKey : Bone.PositionKeys)
				Animation.nTotalFrames = std::max(
				  Animation.nTotalFrames, static_cast<int>(PositionKey.fTime));
			for (auto& RotationKey : Bone.RotationKeys)
				Animation.nTotalFrames = std::max(
				  Animation.nTotalFrames, static_cast<int>(RotationKey.fTime));
		}
	}

	return bLoaded;
}
