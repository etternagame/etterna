#include "CommandBatcher.h"
#include <cassert>

void
Display::CommandBatcher::InsertCommand(Command command)
{
	assert(command.type != CommandType::Invalid);

	// InsertDrawCommand for draw commands
	assert(command.type != CommandType::Draw);

	command.sizeInBytes = sizeof(Command);
	CopyIntoBuffer(reinterpret_cast<uint8_t*>(&command), sizeof(Command));

	m_CommandCount++;
}

void
Display::CommandBatcher::InsertDrawCommand(DrawMode drawMode,
										   MatrixState matrixState,
										   uint8_t* vertexData,
										   size_t vertexDataLength)
{
	assert(drawMode != DrawMode::Invalid);

	Command command = {};
	command.type = CommandType::Draw;
	command.sizeInBytes =
	  sizeof(Command) + sizeof(MatrixState) + vertexDataLength;
	command.drawMode = drawMode;
	CopyIntoBuffer((uint8_t*)&command, sizeof(Command));
	CopyIntoBuffer((uint8_t*)&matrixState, sizeof(MatrixState));
	CopyIntoBuffer(vertexData, vertexDataLength);

	m_CommandCount++;
}

void
Display::CommandBatcher::Clear()
{
	m_CommandBuffer.clear();
	m_CommandCount = 0;
}

void
Display::CommandBatcher::CopyIntoBuffer(uint8_t* source,
													size_t sourceLength)
{
	assert(sourceLength > 0);
	assert(source != nullptr);

	size_t currentSize = m_CommandBuffer.size();
	m_CommandBuffer.resize(currentSize + sourceLength);
	std::memcpy(&m_CommandBuffer[currentSize], source, sourceLength);
}
