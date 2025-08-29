#ifndef DISPLAY_COMMAND_BATCHER_H
#define DISPLAY_COMMAND_BATCHER_H

#include "DrawCommand.h"
#include "RenderState.h"
#include <queue>
#include <string>

// TODO: put commands for an actor into a blob and then make blob queue!
namespace Display
{

class CommandBatcher
{
  public:
    void InsertRenderStateCommand(RenderState renderState);
    void InsertSpriteDrawCommand(DrawMode drawMode, MatrixState &&matrixState, const RageSpriteVertex *vertexData,
                                 int vertexCount);
    void InsertCompiledGeometryDrawCommand(DrawMode drawMode, MatrixState &&matrixState, const RageCompiledGeometry *p,
                                           int iMeshIndex);
    void Clear();

    std::vector<IndirectCommand> m_IndirectCommandBuffer;
    std::vector<RageSpriteVertex> m_SpriteVertexBuffer;
    std::vector<RageModelVertex> m_ModelVertexBuffer;
    std::vector<RenderState> m_RenderStateBuffer;
    std::vector<MatrixState> m_MatrixStateBuffer;
};

} // namespace Display

#endif
