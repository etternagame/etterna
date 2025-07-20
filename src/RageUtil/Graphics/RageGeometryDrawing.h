/* RageGeometryDrawing - information/data necessary to draw RageCompiledGeometry objects */
#ifndef RAGE_GEOMETRY_DRAWING_H
#define RAGE_GEOMETRY_DRAWING_H

#include "RageModelGeometry.h"
#include <vector>

class RageGeometryDrawing
{
	RageCompiledGeometry* p;
	int iMeshIndex;
	std::vector<msMesh> vMeshes;
};

#endif
