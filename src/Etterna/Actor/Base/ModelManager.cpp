﻿#include "Etterna/Globals/global.h"
#include "ModelManager.h"
#include "RageUtil/Graphics/RageDisplay.h"
#include "RageUtil/Misc/RageLog.h"
#include "RageUtil/Utils/RageUtil.h"

ModelManager* MODELMAN =
  NULL; // global and accessible from anywhere in our program

ModelManager::ModelManager() = default;

ModelManager::~ModelManager()
{
	for (std::map<RString, RageModelGeometry*>::iterator i =
		   m_mapFileToGeometry.begin();
		 i != m_mapFileToGeometry.end();
		 ++i) {
		RageModelGeometry* pGeom = i->second;
		if (pGeom->m_iRefCount)
			LOG->Trace("MODELMAN LEAK: '%s', RefCount = %d.",
					   i->first.c_str(),
					   pGeom->m_iRefCount);
		SAFE_DELETE(pGeom);
	}
}

RageModelGeometry*
ModelManager::LoadMilkshapeAscii(const RString& sFile, bool bNeedNormals)
{
	std::map<RString, RageModelGeometry*>::iterator p =
	  m_mapFileToGeometry.find(sFile);
	if (p != m_mapFileToGeometry.end()) {
		/* Found the geometry.  Just increase the refcount and return it. */
		RageModelGeometry* pGeom = p->second;
		++pGeom->m_iRefCount;
		return pGeom;
	}

	auto* pGeom = new RageModelGeometry;
	pGeom->LoadMilkshapeAscii(sFile, bNeedNormals);

	m_mapFileToGeometry[sFile] = pGeom;
	return pGeom;
}

void
ModelManager::UnloadModel(RageModelGeometry* m)
{
	m->m_iRefCount--;
	ASSERT(m->m_iRefCount >= 0);

	if (m->m_iRefCount)
		return; /* Can't unload models that are still referenced. */

	for (std::map<RString, RageModelGeometry*>::iterator i =
		   m_mapFileToGeometry.begin();
		 i != m_mapFileToGeometry.end();
		 ++i) {
		if (i->second == m) {
			if (m_Prefs.m_bDelayedUnload) {
				// leave this geometry loaded
				return;
			} else {
				m_mapFileToGeometry.erase(i); // remove map entry
				SAFE_DELETE(m);				  // free the texture
				return;
			}
		}
	}

	FAIL_M("Tried to delete a texture that wasn't loaded");
}

bool
ModelManager::SetPrefs(const ModelManagerPrefs& prefs)
{
	m_Prefs = prefs;
	return false;
}
