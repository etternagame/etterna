#include "CommandBatcher.h"
#include <cassert>

void Display::CommandBatcher::InsertCommand(const Command &command)
{
    assert(command.type != CommandType::Draw);

    m_CommandBuffer.push_back(command);
	m_RenderStateCount += command.type == CommandType::RenderStateChanged;
}

void Display::CommandBatcher::InsertDrawCommand(DrawMode drawMode, MatrixState matrixState,
                                                const RageSpriteVertex *vertexData, int vertexCount)
{
    assert(drawMode != DrawMode::Invalid);

    Command command = {};
    command.type = CommandType::Draw;
    command.draw.drawMode = drawMode;
    command.draw.matrices = matrixState;
    command.draw.vertex = vertexData;
    command.draw.vertexCount = vertexCount;

	assert(m_RenderStateCount >= 1 && "Rendering information must be set before drawing");
	command.draw.renderStateIndex = m_RenderStateCount - 1;

    m_CommandBuffer.push_back(command);
}

void Display::CommandBatcher::Clear()
{
    m_CommandBuffer.clear();
	m_RenderStateCount = 0;
}
