#ifndef DISPLAY_MATRIX_STATE_H
#define DISPLAY_MATRIX_STATE_H

namespace Display {

struct MatrixState
{
	RageMatrix projection;
	RageMatrix view;
	RageMatrix world;
	RageMatrix texture;
};

}

#endif