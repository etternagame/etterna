#ifndef DISPLAY_COMMAND_BATCHER_H
#define DISPLAY_COMMAND_BATCHER_H

#include "Commands.h"
#include <queue>
#include <string>

// TODO: put commands for an actor into a blob and then make blob queue!
namespace Display
{

class CommandBatcher
{
  public:
    void InsertCommand(const Command& command);
	void InsertDrawCommand(DrawMode drawMode,
						   MatrixState matrixState,
						   const RageSpriteVertex* vertexData,
						   int vertexCount);
    void Clear();

    std::vector<Command> m_CommandBuffer;
};

} // namespace Display

#endif
