#ifndef DISPLAY_COMMAND_BATCHER_H
#define DISPLAY_COMMAND_BATCHER_H

#include <queue>
#include <stack>
#include <string>
#include <map>
#include <optional>
#include "DrawMode.h"
#include "MatrixState.h"
#include "RenderState.h"
#include "RenderNode.h"
#include "Vertex.h"

namespace DisplayAdapter {

class CommandBatcher
{
  public:
	void InsertPipelineChangeCommand(
	  intptr_t pipeline,
	  const std::vector<uint8_t>& vertexShaderArgs,
	  const std::vector<uint8_t>& fragShaderArgs,
	  bool persist);
	void InsertRenderTargetCommand(intptr_t renderTarget, bool preserveTexture);
	void InsertSpriteDrawCommand(DrawMode drawMode,
								 MatrixState&& matrixState,
								 const RageSpriteVertex* vertexData,
								 int vertexCount,
								 const RenderState& renderState);
	void InsertCompiledGeometryDrawCommand(MatrixState&& matrixState,
										   const RageCompiledGeometry* p,
										   int iMeshIndex,
										   const RenderState& renderState);
	void HandleDrawCommand(int indexOffset,
						   int indexCount,
						   const RenderState& renderState);
	void Clear();
	void FixRenderNodeOrder();

	std::vector<Vertex> m_VertexBuffer;
	std::vector<uint32_t> m_IndexBuffer;
	std::vector<MatrixState> m_MatrixStateBuffer;
	std::vector<RenderNode> m_RenderNodes;
	std::stack<PipelineSettings> m_PipelineStack;
	std::vector<uint8_t> m_ShaderScratchBuffer;

	std::optional<size_t> m_SwapchainNodeIndex;
	size_t m_CurrentNodeIndex = 0;

	std::optional<PipelineSettings> m_CurrentPipeline = std::nullopt;
};

} // namespace Display

#endif
