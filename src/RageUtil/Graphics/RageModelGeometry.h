/* RageModelGeometry - Stores mesh data. */

#ifndef RAGE_MODEL_GEOMETRY_H
#define RAGE_MODEL_GEOMETRY_H

#include "Etterna/Actor/Base/ModelTypes.h"
#include "RageUtil/Misc/RageTypes.h"
#include <vector>

class RageCompiledGeometry;

class RageModelGeometry
{
  public:
	RageModelGeometry();
	virtual ~RageModelGeometry();

	void LoadMilkshapeAscii(const std::string& sMilkshapeAsciiFile,
							bool bNeedsNormals);
	void OptimizeBones();
	void MergeMeshes(int iFromIndex, int iToIndex);
	bool HasAnyPerVertexBones() const;

	int m_iRefCount;

	vector<msMesh> m_Meshes;
	RageCompiledGeometry*
	  m_pCompiledGeometry; // video memory copy of geometry shared by all meshes

	RageVector3 m_vMins, m_vMaxs;
};

#endif
