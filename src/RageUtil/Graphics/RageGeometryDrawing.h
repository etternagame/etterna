/* RageGeometryDrawing - information/data necessary to draw RageCompiledGeometry objects */
#ifndef RAGE_GEOMETRY_DRAWING_H
#define RAGE_GEOMETRY_DRAWING_H

#include "RageModelGeometry.h"
#include <vector>

class RageGeometryDrawing
{
	const RageCompiledGeometry* p;
	int iMeshIndex;
	const std::vector<msMesh> vMeshes;
};

#endif
