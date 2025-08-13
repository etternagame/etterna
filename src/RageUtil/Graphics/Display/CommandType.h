#ifndef DISPLAY_COMMAND_TYPE_H
#define DISPLAY_COMMAND_TYPE_H

namespace Display {

enum class CommandType
{
	Invalid,
	ClearZBuffer,
	Draw,
	RenderStateChanged,
};

}

#endif
