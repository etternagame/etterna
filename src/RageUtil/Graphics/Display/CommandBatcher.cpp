#include "CommandBatcher.h"
#include <cassert>


void
Display::CommandBatcher::InsertRenderStateCommand(
  RenderState renderState)
{
	m_RenderStateBuffer.push_back(renderState);
}

void
Display::CommandBatcher::InsertDrawCommand(DrawMode drawMode,
										   MatrixState matrixState,
                                                const RageSpriteVertex *vertexData, int vertexCount)
{
    assert(drawMode != DrawMode::Invalid);

    DrawCommand command = {.drawMode = drawMode, .matrices = matrixState};

	// TODO: adjust stuff based on draw mode
    command.vertex = vertexData;
    command.vertexCount = vertexCount;

	assert(m_RenderStateBuffer.size() >= 1 &&
		   "Rendering information must be set before drawing");
	command.renderStateIndex = m_RenderStateBuffer.size() - 1;

    m_CommandBuffer.push_back(command);
}

void Display::CommandBatcher::Clear()
{
    m_CommandBuffer.clear();
	m_RenderStateBuffer.clear();
}
