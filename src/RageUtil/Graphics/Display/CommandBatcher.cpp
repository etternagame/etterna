#include "CommandBatcher.h"
#include <cassert>

void Display::CommandBatcher::InsertRenderStateCommand(RenderState renderState)
{
    m_RenderStateBuffer.push_back(renderState);
}

void Display::CommandBatcher::InsertSpriteDrawCommand(DrawMode drawMode, MatrixState &&matrixState,
                                                      const RageSpriteVertex *vertexData, int vertexCount)
{
    assert(drawMode != DrawMode::Invalid);
    assert(m_RenderStateBuffer.size() >= 1 && "Rendering information must be set before drawing");

    DrawCommand command = {.drawMode = drawMode, .matrixState = matrixState};

    // TODO: adjust stuff based on draw mode
    command.vertexCount = vertexCount;
    command.vertexOffset = m_SpriteVertexBuffer.size();
    std::copy(vertexData, vertexData + vertexCount, std::back_inserter(m_SpriteVertexBuffer));

    command.renderStateIndex = m_RenderStateBuffer.size() - 1;

    m_CommandBuffer.push_back(command);
}

void Display::CommandBatcher::InsertCompiledGeometryDrawCommand(DrawMode drawMode, MatrixState &&matrixState,
                                                     const RageCompiledGeometry *p, int iMeshIndex)
{
    assert(drawMode == DrawMode::CompiledGeometry);
    assert(m_RenderStateBuffer.size() >= 1 && "Rendering information must be set before drawing");

    DrawCommand command = {.drawMode = drawMode, .matrixState = matrixState};

	assert(false && "TODO: fix whatever this RageCompiledGeometry thingy should do");

    command.renderStateIndex = m_RenderStateBuffer.size() - 1;

    m_CommandBuffer.push_back(command);
}

void Display::CommandBatcher::Clear()
{
    m_CommandBuffer.clear();
    m_SpriteVertexBuffer.clear();
    m_ModelVertexBuffer.clear();
    m_RenderStateBuffer.clear();
}
