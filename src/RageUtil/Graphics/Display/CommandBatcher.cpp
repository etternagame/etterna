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
    assert(drawMode != DrawMode::CompiledGeometry);
    assert(m_RenderStateBuffer.size() >= 1 && "Rendering information must be set before drawing");

    DrawCommand command = {.drawMode = drawMode, .matrixState = matrixState};

    command.vertexOffset = m_SpriteVertexBuffer.size();

    // -- changing draw mode in the middle of the queue would likely require switching pipeline state objects
    //	  so just convert to a triangle list
    switch (drawMode)
    {
    case DrawMode::Triangles: {
        command.vertexCount = vertexCount;
        std::copy(vertexData, vertexData + vertexCount, std::back_inserter(m_SpriteVertexBuffer));
        break;
    }
    case DrawMode::Quads: {
        break;
    }
    case DrawMode::QuadStrip: {
        break;
    }
    case DrawMode::Fan: {
        break;
    }
    case DrawMode::Strip: {
        break;
    }
    case DrawMode::SymmetricQuadStrip: {
        break;
    }
    default:
        break;
    }

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
