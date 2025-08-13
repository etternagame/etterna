#include "CommandBatcher.h"
#include <cassert>

void
Display::CommandBatcher::InsertCommand(const Command& command)
{
	assert(command.index() != static_cast<size_t>(CommandIndex::Draw));

	m_CommandBuffer.push_back(command);
}

void
Display::CommandBatcher::InsertDrawCommand(DrawMode drawMode,
										   MatrixState matrixState,
										   const RageSpriteVertex* vertexData, int vertexCount)
{
	assert(drawMode != DrawMode::Invalid);

	DrawCommand command = {};
	command.type = CommandType::Draw;
	command.drawMode = drawMode;
	command.matrices = matrixState;
	command.vertex = vertexData;
	command.vertexCount = 
	vertexCount;

	m_CommandBuffer.push_back(command);
}

void
Display::CommandBatcher::Clear()
{
	m_CommandBuffer.clear();
}
