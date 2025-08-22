#ifndef DISPLAY_MATRIX_STATE_H
#define DISPLAY_MATRIX_STATE_H

#include "RageUtil/Misc/RageTypes.h"

namespace Display {

struct alignas(256) MatrixState
{
	RageMatrix projection;
	RageMatrix view;
	RageMatrix world;
	RageMatrix texture;
};

}

#endif
