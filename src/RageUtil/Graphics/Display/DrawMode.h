#ifndef DISPLAY_DRAW_MODE_H
#define DISPLAY_DRAW_MODE_H

namespace DisplayAdapter {

enum class DrawMode
{
	Invalid,
	Quads,
	QuadStrip,
	Fan,
	Strip,
	Triangles,
	SymmetricQuadStrip,
	CompiledGeometry,
};

}

#endif