#ifndef DISPLAY_COMPILED_GEOMETRY_H
#define DISPLAY_COMPILED_GEOMETRY_H

#include "RageUtil/Graphics/RageDisplay.h"

namespace DisplayAdapter {

struct CompiledGeometry : public RageCompiledGeometry
{
	void Allocate(const std::vector<msMesh>& vMeshes) override;
	void Change(const std::vector<msMesh>& vMeshes) override;
	void Draw(int iMeshIndex) const override;

	std::vector<RageModelVertex> m_Vertices;
	std::vector<msTriangle> m_Triangles;
	friend class CommandBatcher;
};

}

#endif
