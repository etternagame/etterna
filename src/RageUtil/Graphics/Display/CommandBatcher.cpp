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

    DrawCommand command = {};

    command.StartVertexLocation = m_SpriteVertexBuffer.size();
    command.InstanceCount = 1;
    command.StartInstanceLocation = 0;

    // -- changing draw mode in the middle of the queue would likely require switching pipeline state objects
    //	  and most of the ye olde draw modes aren't supported
    //    so just convert to a triangle list
    // -- unrolled loops look funny though
    // -- maybe this can be done on the GPU via mesh shaders and/or work graphs but that's for unstable_d3d_mintyfresh
    switch (drawMode)
    {
    case DrawMode::Triangles: {
        std::copy(vertexData, vertexData + vertexCount, std::back_inserter(m_SpriteVertexBuffer));
        break;
    }
    case DrawMode::Quads: {
        const size_t quadCount = vertexCount / 4;
        const size_t triangleCount = quadCount * 2;
        const size_t expandedVertexCount = triangleCount * 3;

        m_SpriteVertexBuffer.resize(m_SpriteVertexBuffer.size() + expandedVertexCount);

        for (size_t i = 0; i < quadCount; i++)
        {
            m_SpriteVertexBuffer[i * 6 + 0] = vertexData[i * 4 + 0];
            m_SpriteVertexBuffer[i * 6 + 1] = vertexData[i * 4 + 1];
            m_SpriteVertexBuffer[i * 6 + 2] = vertexData[i * 4 + 2];
            m_SpriteVertexBuffer[i * 6 + 3] = vertexData[i * 4 + 2];
            m_SpriteVertexBuffer[i * 6 + 4] = vertexData[i * 4 + 3];
            m_SpriteVertexBuffer[i * 6 + 5] = vertexData[i * 4 + 0];
        }

        break;
    }
    case DrawMode::QuadStrip: {
        const size_t quadCount = (vertexCount - 2) / 2;
        const size_t triangleCount = quadCount * 2;
        const size_t expandedVertexCount = triangleCount * 3;

        m_SpriteVertexBuffer.resize(m_SpriteVertexBuffer.size() + expandedVertexCount);

        for (size_t i = 0; i < quadCount; i++)
        {
            m_SpriteVertexBuffer[i * 6 + 0] = vertexData[i * 2 + 0];
            m_SpriteVertexBuffer[i * 6 + 1] = vertexData[i * 2 + 1];
            m_SpriteVertexBuffer[i * 6 + 2] = vertexData[i * 2 + 2];
            m_SpriteVertexBuffer[i * 6 + 3] = vertexData[i * 2 + 1];
            m_SpriteVertexBuffer[i * 6 + 4] = vertexData[i * 2 + 2];
            m_SpriteVertexBuffer[i * 6 + 5] = vertexData[i * 2 + 3];
        }

        break;
    }
    case DrawMode::Fan: {
        assert(vertexCount >= 3);

        for (size_t i = 1; i < vertexCount - 1; i++)
        {
            m_SpriteVertexBuffer.push_back(vertexData[0]);
            m_SpriteVertexBuffer.push_back(vertexData[i]);
            m_SpriteVertexBuffer.push_back(vertexData[i + 1]);
        }

        break;
    }
    case DrawMode::Strip: {
        assert(vertexCount >= 3);

        for (size_t i = 0; i < vertexCount - 2; i++)
        {
            if (i % 2 == 0)
            {
                m_SpriteVertexBuffer.push_back(vertexData[i]);
                m_SpriteVertexBuffer.push_back(vertexData[i + 1]);
                m_SpriteVertexBuffer.push_back(vertexData[i + 2]);
            }
            else
            {
                m_SpriteVertexBuffer.push_back(vertexData[i + 1]);
                m_SpriteVertexBuffer.push_back(vertexData[i]);
                m_SpriteVertexBuffer.push_back(vertexData[i + 2]);
            }
        }

        break;
    }
    case DrawMode::SymmetricQuadStrip: {
        const size_t quadCount = (vertexCount - 3) / 3;
        const size_t triangleCount = quadCount * 2;
        const size_t expandedVertexCount = triangleCount * 3;

        m_SpriteVertexBuffer.resize(m_SpriteVertexBuffer.size() + expandedVertexCount);

        for (size_t i = 0; i < quadCount; i++)
        {
            // { 1, 3, 0 } { 1, 4, 3 } { 1, 5, 4 } { 1, 2, 5 }
            m_SpriteVertexBuffer[i * 12 + 0] = vertexData[i * 3 + 1];
            m_SpriteVertexBuffer[i * 12 + 1] = vertexData[i * 3 + 3];
            m_SpriteVertexBuffer[i * 12 + 2] = vertexData[i * 3 + 0];
            m_SpriteVertexBuffer[i * 12 + 3] = vertexData[i * 3 + 1];
            m_SpriteVertexBuffer[i * 12 + 4] = vertexData[i * 3 + 4];
            m_SpriteVertexBuffer[i * 12 + 5] = vertexData[i * 3 + 3];
            m_SpriteVertexBuffer[i * 12 + 6] = vertexData[i * 3 + 1];
            m_SpriteVertexBuffer[i * 12 + 7] = vertexData[i * 3 + 5];
            m_SpriteVertexBuffer[i * 12 + 8] = vertexData[i * 3 + 4];
            m_SpriteVertexBuffer[i * 12 + 9] = vertexData[i * 3 + 1];
            m_SpriteVertexBuffer[i * 12 + 10] = vertexData[i * 3 + 2];
            m_SpriteVertexBuffer[i * 12 + 11] = vertexData[i * 3 + 5];
        }

        break;
    }
    default:
        break;
    }

    command.VertexCountPerInstance = m_SpriteVertexBuffer.size() - command.StartVertexLocation;

    m_MatrixStateBuffer.push_back(matrixState);

    DrawCommandArgument argument = {.matrixStateIndex = (uint32_t)m_MatrixStateBuffer.size() - 1,
                                    .renderStateIndex = (uint32_t)m_RenderStateBuffer.size() - 1};
    m_IndirectCommandArgumentBuffer.push_back(argument);
    m_IndirectCommandBuffer.push_back(command);
}

void Display::CommandBatcher::InsertCompiledGeometryDrawCommand(DrawMode drawMode, MatrixState &&matrixState,
                                                                const RageCompiledGeometry *p, int iMeshIndex)
{
    // TODO (^_^)

    /*assert(drawMode == DrawMode::CompiledGeometry);
    assert(m_RenderStateBuffer.size() >= 1 && "Rendering information must be set before drawing");

    DrawCommand command = { .useSpriteVertex = false,
                            .matrixState = matrixState };

    assert(false && "TODO: fix whatever this RageCompiledGeometry thingy should do");

    command.renderStateIndex = m_RenderStateBuffer.size() - 1;

    m_CommandBuffer.push_back(command);*/
}

void Display::CommandBatcher::Clear()
{
    m_IndirectCommandBuffer.clear();
    m_SpriteVertexBuffer.clear();
    m_ModelVertexBuffer.clear();
    m_RenderStateBuffer.clear();
    m_MatrixStateBuffer.clear();
}
