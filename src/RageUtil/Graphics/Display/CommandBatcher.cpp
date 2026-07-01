#include "CommandBatcher.h"
#include "CompiledGeometry.h"
#include <cassert>
#include <algorithm>

void
DisplayAdapter::CommandBatcher::InsertPipelineChangeCommand(
  intptr_t pipeline,
  const std::vector<uint8_t>& vertexShaderArgs,
  const std::vector<uint8_t>& fragShaderArgs,
  bool persist)
{
	uint64_t vertexShaderInfo = UINT64_MAX;
	uint64_t fragShaderInfo = UINT64_MAX;

	if (vertexShaderArgs.size()) {
		vertexShaderInfo = m_ShaderScratchBuffer.size();
		m_ShaderScratchBuffer.resize(m_ShaderScratchBuffer.size() +
									 vertexShaderArgs.size());
		std::memcpy(&m_ShaderScratchBuffer[vertexShaderInfo],
					vertexShaderArgs.data(),
					vertexShaderArgs.size() * sizeof(uint8_t));
	}

	if (fragShaderArgs.size()) {
		fragShaderInfo = m_ShaderScratchBuffer.size();
		m_ShaderScratchBuffer.resize(m_ShaderScratchBuffer.size() +
									 fragShaderArgs.size());
		std::memcpy(&m_ShaderScratchBuffer[fragShaderInfo],
					fragShaderArgs.data(),
					fragShaderArgs.size() * sizeof(uint8_t));
	}

	PipelineSettings settings = {};
	if (persist) {
		if (pipeline) {
			m_PipelineStack.push(
			  { pipeline, vertexShaderInfo, fragShaderInfo });
		} else {
			m_PipelineStack.pop();
		}
		settings = m_PipelineStack.top();
	} else {
		if (m_PipelineStack.size()) {
			return;
		}
		settings = { pipeline, vertexShaderInfo, fragShaderInfo };
	}

	if (!m_RenderNodes.size()) {
		InsertRenderTargetCommand(0, false);
	}

	if (!m_CurrentPipeline.has_value() ||
		std::tie(pipeline, vertexShaderInfo, fragShaderInfo) !=
		  std::tie(m_CurrentPipeline->GraphicsPipeline,
				   m_CurrentPipeline->VertexShaderArg,
				   m_CurrentPipeline->FragShaderArg)) {
		m_RenderNodes[m_CurrentNodeIndex].DrawCalls.push_back(
		  { settings, m_IndexBuffer.size(), (size_t)0 });
	}

	m_CurrentPipeline = settings;
}

void
DisplayAdapter::CommandBatcher::InsertRenderTargetCommand(intptr_t renderTarget,
														  bool preserveTexture)
{
	if (renderTarget == 0) {
		if (!m_SwapchainNodeIndex.has_value()) {
			m_RenderNodes.push_back(
			  { renderTarget, preserveTexture, std::vector<DrawCall>() });
			m_SwapchainNodeIndex = m_RenderNodes.size() - 1;
		}
		m_CurrentNodeIndex = *m_SwapchainNodeIndex;
		m_RenderNodes[m_CurrentNodeIndex].PreserveRenderTarget =
		  preserveTexture;
	} else {
		m_RenderNodes.push_back(
		  { renderTarget, preserveTexture, std::vector<DrawCall>() });
		m_CurrentNodeIndex = m_RenderNodes.size() - 1;
	}
}

uint32_t
GetSamplerFlagsFromRenderState(const DisplayAdapter::RenderState& state)
{
	return (uint8_t)state.textureWrapping |
		   ((uint8_t)state.textureFiltering << 1);
}

void
DisplayAdapter::CommandBatcher::InsertSpriteDrawCommand(
  DrawMode drawMode,
  MatrixState&& matrixState,
  const RageSpriteVertex* vertexData,
  int vertexCount,
  const RenderState& renderState)
{
	assert(drawMode != DrawMode::Invalid);
	assert(drawMode != DrawMode::CompiledGeometry);

	m_MatrixStateBuffer.push_back(matrixState);

	const auto previousVertexCount = m_VertexBuffer.size();
	for (int i = 0; i < vertexCount; i++) {
		m_VertexBuffer.push_back(
		  { vertexData[i],
			(uint32_t)m_MatrixStateBuffer.size() - 1,
			(uint32_t)renderState.textureHandle,
			GetSamplerFlagsFromRenderState(renderState) });
	}

	const auto prevCount = m_IndexBuffer.size();
	switch (drawMode) {
		case DrawMode::Triangles: {
			m_IndexBuffer.resize(prevCount + vertexCount);
			for (size_t i = 0; i < vertexCount / 3; i++) {
				m_IndexBuffer[prevCount + 3 * i] = previousVertexCount + 3 * i;
				m_IndexBuffer[prevCount + 3 * i + 1] =
				  previousVertexCount + 3 * i + 1;
				m_IndexBuffer[prevCount + 3 * i + 2] =
				  previousVertexCount + 3 * i + 2;
			}
			break;
		}
		case DrawMode::Quads: {
			m_IndexBuffer.resize(prevCount + 6 * vertexCount / 4);
			for (size_t i = 0; i < vertexCount / 4; i++) {
				m_IndexBuffer[prevCount + i * 6 + 0] =
				  previousVertexCount + i * 4 + 0;
				m_IndexBuffer[prevCount + i * 6 + 1] =
				  previousVertexCount + i * 4 + 1;
				m_IndexBuffer[prevCount + i * 6 + 2] =
				  previousVertexCount + i * 4 + 2;
				m_IndexBuffer[prevCount + i * 6 + 3] =
				  previousVertexCount + i * 4 + 2;
				m_IndexBuffer[prevCount + i * 6 + 4] =
				  previousVertexCount + i * 4 + 3;
				m_IndexBuffer[prevCount + i * 6 + 5] =
				  previousVertexCount + i * 4 + 0;
			}

			break;
		}
		case DrawMode::QuadStrip: {
			m_IndexBuffer.resize(prevCount + 6 * (vertexCount - 2) / 2);
			for (size_t i = 0; i < (vertexCount - 2) / 2; i++) {
				m_IndexBuffer[prevCount + i * 6 + 0] =
				  previousVertexCount + i * 2 + 0;
				m_IndexBuffer[prevCount + i * 6 + 1] =
				  previousVertexCount + i * 2 + 1;
				m_IndexBuffer[prevCount + i * 6 + 2] =
				  previousVertexCount + i * 2 + 2;
				m_IndexBuffer[prevCount + i * 6 + 3] =
				  previousVertexCount + i * 2 + 1;
				m_IndexBuffer[prevCount + i * 6 + 4] =
				  previousVertexCount + i * 2 + 2;
				m_IndexBuffer[prevCount + i * 6 + 5] =
				  previousVertexCount + i * 2 + 3;
			}

			break;
		}
		case DrawMode::Fan: {
			assert(vertexCount >= 3);
			m_IndexBuffer.resize(prevCount + 3 * (vertexCount - 1));
			for (size_t i = 1; i < vertexCount - 1; i++) {
				m_IndexBuffer[prevCount + 3 * i] = previousVertexCount;
				m_IndexBuffer[prevCount + 3 * i + 1] = previousVertexCount + i;
				m_IndexBuffer[prevCount + 3 * i + 2] =
				  previousVertexCount + i + 1;
			}

			break;
		}
		case DrawMode::Strip: {
			assert(vertexCount >= 3);
			m_IndexBuffer.resize(prevCount + 3 * (vertexCount - 2));

			for (size_t i = 0; i < vertexCount - 2; i++) {
				if (i % 2 == 0) {
					m_IndexBuffer[prevCount + 3 * i] = previousVertexCount + i;
					m_IndexBuffer[prevCount + 3 * i + 1] =
					  previousVertexCount + i + 1;
					m_IndexBuffer[prevCount + 3 * i + 2] =
					  previousVertexCount + i + 2;
				} else {
					m_IndexBuffer[prevCount + 3 * i] =
					  previousVertexCount + i + 1;
					m_IndexBuffer[prevCount + 3 * i + 1] =
					  previousVertexCount + i;
					m_IndexBuffer[prevCount + 3 * i + 2] =
					  previousVertexCount + i + 2;
				}
			}

			break;
		}
		case DrawMode::SymmetricQuadStrip: {
			m_IndexBuffer.resize(prevCount + 12 * (vertexCount - 3) / 3);
			for (size_t i = 0; i < (vertexCount - 3) / 3; i++) {
				m_IndexBuffer[prevCount + i * 12 + 0] =
				  previousVertexCount + i * 3 + 3;
				m_IndexBuffer[prevCount + i * 12 + 1] =
				  previousVertexCount + i * 3 + 1;
				m_IndexBuffer[prevCount + i * 12 + 2] =
				  previousVertexCount + i * 3 + 0;

				m_IndexBuffer[prevCount + i * 12 + 3] =
				  previousVertexCount + i * 3 + 4;
				m_IndexBuffer[prevCount + i * 12 + 4] =
				  previousVertexCount + i * 3 + 1;
				m_IndexBuffer[prevCount + i * 12 + 5] =
				  previousVertexCount + i * 3 + 3;

				m_IndexBuffer[prevCount + i * 12 + 6] =
				  previousVertexCount + i * 3 + 5;
				m_IndexBuffer[prevCount + i * 12 + 7] =
				  previousVertexCount + i * 3 + 1;
				m_IndexBuffer[prevCount + i * 12 + 8] =
				  previousVertexCount + i * 3 + 4;

				m_IndexBuffer[prevCount + i * 12 + 9] =
				  previousVertexCount + i * 3 + 2;
				m_IndexBuffer[prevCount + i * 12 + 10] =
				  previousVertexCount + i * 3 + 1;
				m_IndexBuffer[prevCount + i * 12 + 11] =
				  previousVertexCount + i * 3 + 5;
			}

			break;
		}
		default:
			throw std::runtime_error("Unknown draw command type");
	}

	HandleDrawCommand(prevCount, m_IndexBuffer.size() - prevCount, renderState);
}

void
DisplayAdapter::CommandBatcher::InsertCompiledGeometryDrawCommand(
  MatrixState&& matrixState,
  const RageCompiledGeometry* p,
  int iMeshIndex,
  const RenderState& renderState)
{
	const auto geometry = reinterpret_cast<const CompiledGeometry*>(p);
	const auto& meshInfo = geometry->m_vMeshInfo[iMeshIndex];

	m_MatrixStateBuffer.push_back(matrixState);
	if (meshInfo.m_bNeedsTextureMatrixScale) {
		m_MatrixStateBuffer.back().texture.m[3][0] = 0;
		m_MatrixStateBuffer.back().texture.m[3][1] = 0;
	}

	RageVColor whiteVColor = {};
	whiteVColor.r = UINT8_MAX;
	whiteVColor.g = UINT8_MAX;
	whiteVColor.b = UINT8_MAX;
	whiteVColor.a = UINT8_MAX;

	const auto previousVertexCount = m_VertexBuffer.size();
	for (int i = 0; i < meshInfo.iVertexCount; i++) {
		const auto& vertex = geometry->m_Vertices[meshInfo.iVertexStart + i];
		m_VertexBuffer.push_back(
		  { { vertex.p, vertex.n, whiteVColor, vertex.t },
			(uint32_t)m_MatrixStateBuffer.size() - 1,
			(uint32_t)renderState.textureHandle,
			GetSamplerFlagsFromRenderState(renderState) });
	}

	const auto prevIndexCount = m_IndexBuffer.size();

	for (int i = meshInfo.iTriangleStart;
		 i < meshInfo.iTriangleStart + meshInfo.iTriangleCount;
		 i++) {
		for (int j = 0; j < 3; j++) {
			m_IndexBuffer.push_back(previousVertexCount +
									geometry->m_Triangles[i].nVertexIndices[j] -
									meshInfo.iVertexStart);
		}
	}

	HandleDrawCommand(
	  prevIndexCount, m_IndexBuffer.size() - prevIndexCount, renderState);
}

void
DisplayAdapter::CommandBatcher::HandleDrawCommand(
  int indexOffset,
  int indexCount,
  const RenderState& renderState)
{
	assert(indexCount > 0);
	assert(m_CurrentPipeline.has_value());
	if (!m_RenderNodes.size()) {
		InsertRenderTargetCommand(0, false);
	}

	auto& node = m_RenderNodes[m_CurrentNodeIndex];
	if (!node.DrawCalls.size()) {
		m_RenderNodes[m_CurrentNodeIndex].DrawCalls.push_back(
		  { *m_CurrentPipeline,
			(size_t)indexOffset,
			(size_t)0,
			renderState.blendingMode,
			renderState.depthTestMode,
			renderState.depthWriteEnabled });
	}

	// if we previously filled in a different draw call, we should create a new
	// one
	// or if we changed render state stuffs

	bool filledPreviousCall =
	  node.DrawCalls[node.DrawCalls.size() - 1].IndexCount != 0 &&
	  node.DrawCalls[node.DrawCalls.size() - 1].IndexCount +
		  node.DrawCalls[node.DrawCalls.size() - 1].IndexOffset !=
		indexOffset;
	bool differentRenderState =
	  !filledPreviousCall &&
	  std::tie(node.DrawCalls[node.DrawCalls.size() - 1].BlendingMode,
			   node.DrawCalls[node.DrawCalls.size() - 1].DepthTestMode,
			   node.DrawCalls[node.DrawCalls.size() - 1].DepthWriteEnabled) !=
		std::tie(renderState.blendingMode,
				 renderState.depthTestMode,
				 renderState.depthWriteEnabled);

	if (filledPreviousCall || differentRenderState) {
		node.DrawCalls.push_back({ *m_CurrentPipeline,
								   (size_t)indexOffset,
								   (size_t)0,
								   renderState.blendingMode,
								   renderState.depthTestMode,
								   renderState.depthWriteEnabled });
	}

	node.DrawCalls[node.DrawCalls.size() - 1].IndexCount += indexCount;
}

void
DisplayAdapter::CommandBatcher::Clear()
{
	m_VertexBuffer.clear();
	m_IndexBuffer.clear();
	m_MatrixStateBuffer.clear();
	m_RenderNodes.clear();

	// std::stack has no .clear() :|
	while (m_PipelineStack.size()) {
		m_PipelineStack.pop();
	}

	m_CurrentPipeline = std::nullopt;
	m_SwapchainNodeIndex = std::nullopt;
	m_CurrentNodeIndex = 0;

	m_ShaderScratchBuffer.clear();
}

void
DisplayAdapter::CommandBatcher::FixRenderNodeOrder()
{
	if (!m_RenderNodes.size()) {
		return;
	}

	auto node = m_RenderNodes[*m_SwapchainNodeIndex];
	m_RenderNodes.erase(m_RenderNodes.begin() + *m_SwapchainNodeIndex);
	m_RenderNodes.push_back(node);
}
