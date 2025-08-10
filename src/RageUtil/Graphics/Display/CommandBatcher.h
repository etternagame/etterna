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
    void InsertCommand(Command command);
    void InsertDrawCommand(DrawMode drawMode, MatrixState matrixState, uint8_t *vertexData, size_t vertexDataLength);
    void Clear();
	void CopyIntoBuffer(uint8_t* source, size_t sourceLength);

    std::vector<uint8_t> m_CommandBuffer;
    size_t m_CommandCount;
};

} // namespace Display

#endif
