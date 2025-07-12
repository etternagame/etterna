#include "RageCompiledGeometrySWD3D.h"
#include "RageDisplay_D3D_Helpers.h"
#include "RageDisplay_D3D.h"
#undef max // >:( (fix later)

void
RageCompiledGeometrySWD3D::Allocate(const std::vector<msMesh>& /*vMeshes*/)
{
	m_vVertex.resize(std::max(1U, static_cast<unsigned>(GetTotalVertices())));
	m_vTriangles.resize(
	  std::max(1U, static_cast<unsigned>(GetTotalTriangles())));
}

void
RageCompiledGeometrySWD3D::Change(const std::vector<msMesh>& vMeshes)
{
	for (unsigned i = 0; i < vMeshes.size(); i++) {
		const auto& meshInfo = m_vMeshInfo[i];
		const auto& mesh = vMeshes[i];
		const auto& Vertices = mesh.Vertices;
		const auto& Triangles = mesh.Triangles;

		for (unsigned j = 0; j < Vertices.size(); j++) {
			m_vVertex[meshInfo.iVertexStart + j] = Vertices[j];
		}

		for (unsigned j = 0; j < Triangles.size(); j++) {
			for (unsigned k = 0; k < 3; k++) {
				m_vTriangles[meshInfo.iTriangleStart + j].nVertexIndices[k] =
				  static_cast<uint16_t>(meshInfo.iVertexStart) +
				  Triangles[j].nVertexIndices[k];
			}
		}
	}
}

void
RageCompiledGeometrySWD3D::Draw(int iMeshIndex) const
{
	const auto& meshInfo = m_vMeshInfo[iMeshIndex];

	// oh god
	auto display = reinterpret_cast<RageDisplay_D3D*>(DISPLAY);
	auto device = display->GetD3DDevice();

	if (meshInfo.m_bNeedsTextureMatrixScale) {
		// Kill the texture translation.
		// XXX: Change me to scale the translation by the
		// TextureTranslationScale of the first vertex.
		RageMatrix m;
		device->GetTransform(D3DTS_TEXTURE0, reinterpret_cast<D3DMATRIX*>(&m));

		m.m[2][0] = 0;
		m.m[2][1] = 0;

		device->SetTransform(D3DTS_TEXTURE0, reinterpret_cast<D3DMATRIX*>(&m));
	}

	device->DrawIndexedPrimitiveUP(
	  D3DPT_TRIANGLELIST,
	  // PrimitiveType
	  meshInfo.iVertexStart,
	  // MinIndex
	  meshInfo.iVertexCount,
	  // NumVertices
	  meshInfo.iTriangleCount,
	  // PrimitiveCount,
	  &m_vTriangles[0] + meshInfo.iTriangleStart,
	  // pIndexData,
	  D3DFMT_INDEX16,
	  // IndexDataFormat,
	  &m_vVertex[0],
	  // pVertexStreamZeroData,
	  sizeof(m_vVertex[0]) // VertexStreamZeroStride
	);
}
