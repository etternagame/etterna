#ifndef RAGE_COMPILED_GEOMETRY_SWD3D_H
#define RAGE_COMPILED_GEOMETRY_SWD3D_H

#include "RageUtil/Graphics/RageDisplay.h"

class RageCompiledGeometrySWD3D : public RageCompiledGeometry
{
  public:
	void Allocate(const std::vector<msMesh>& /*vMeshes*/) override;

	void Change(const std::vector<msMesh>& vMeshes) override;

	void Draw(int iMeshIndex) const override;

  protected:
	std::vector<RageModelVertex> m_vVertex;
	std::vector<msTriangle> m_vTriangles;
};

#endif
