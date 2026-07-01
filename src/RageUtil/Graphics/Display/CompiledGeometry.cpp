#include "CompiledGeometry.h"
#include <cassert>

void
DisplayAdapter::CompiledGeometry::Allocate(const std::vector<msMesh>& vMeshes)
{
	m_Vertices.resize(std::max(1U, static_cast<unsigned>(GetTotalVertices())));
	m_Triangles.resize(
	  std::max(1U, static_cast<unsigned>(GetTotalTriangles())));
}

void
DisplayAdapter::CompiledGeometry::Change(const std::vector<msMesh>& vMeshes)
{
	for (unsigned i = 0; i < vMeshes.size(); i++) {
		const auto& meshInfo = m_vMeshInfo[i];
		const auto& mesh = vMeshes[i];
		const auto& Vertices = mesh.Vertices;
		const auto& Triangles = mesh.Triangles;

		for (unsigned j = 0; j < Vertices.size(); j++) {
			m_Vertices[meshInfo.iVertexStart + j] = Vertices[j];
		}

		for (unsigned j = 0; j < Triangles.size(); j++) {
			for (unsigned k = 0; k < 3; k++) {
				m_Triangles[meshInfo.iTriangleStart + j].nVertexIndices[k] =
				  meshInfo.iVertexStart + Triangles[j].nVertexIndices[k];
			}
		}
	}
}

void
DisplayAdapter::CompiledGeometry::Draw(int iMeshIndex) const
{
	assert(false && "This should never be called, "
					"CommandBatcher::InsertCompiledGeometryDrawCommand should "
					"handle the drawing thingy");
}
