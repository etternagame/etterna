#include "Etterna/Globals/global.h"
#include "RageDisplay.h"
#include "RageUtil/File/RageFile.h"
#include "RageUtil/Misc/RageMath.h"
#include "RageModelGeometry.h"
#include "RageUtil/Utils/RageUtil.h"

#define MS_MAX_NAME 32

RageModelGeometry::RageModelGeometry()
{
	m_iRefCount = 1;
	m_pCompiledGeometry = DISPLAY->CreateCompiledGeometry();
}

RageModelGeometry::~RageModelGeometry()
{
	DISPLAY->DeleteCompiledGeometry(m_pCompiledGeometry);
}

void
RageModelGeometry::OptimizeBones()
{
	for (auto& mesh : m_Meshes) {
		if (mesh.Vertices.empty())
			continue; // nothing to optimize

		// check to see if all vertices have the same bone index
		auto bAllVertsUseSameBone = true;

		const char iBoneIndex = mesh.Vertices[0].bone;

		for (unsigned j = 1; j < mesh.Vertices.size(); j++) {
			if (mesh.Vertices[j].bone != iBoneIndex) {
				bAllVertsUseSameBone = false;
				break;
			}
		}

		if (bAllVertsUseSameBone) {
			mesh.m_iBoneIndex = iBoneIndex;

			// clear all vertex/bone associations;
			for (auto& Vertice : mesh.Vertices) {
				Vertice.bone = -1;
			}
		}
	}
}

void
RageModelGeometry::MergeMeshes(int iFromIndex, int iToIndex)
{
	auto& meshFrom = m_Meshes[iFromIndex];
	auto& meshTo = m_Meshes[iToIndex];

	const int iShiftTriangleVertexIndicesBy = meshTo.Vertices.size();
	const int iStartShiftingAtTriangleIndex = meshTo.Triangles.size();

	meshTo.Vertices.insert(meshTo.Vertices.end(),
						   meshFrom.Vertices.begin(),
						   meshFrom.Vertices.end());
	meshTo.Triangles.insert(meshTo.Triangles.end(),
							meshFrom.Triangles.begin(),
							meshFrom.Triangles.end());

	for (unsigned i = iStartShiftingAtTriangleIndex;
		 i < meshTo.Triangles.size();
		 i++) {
		for (auto& iIndex : meshTo.Triangles[i].nVertexIndices) {
			iIndex = uint16_t(iIndex + iShiftTriangleVertexIndicesBy);
		}
	}
}

bool
RageModelGeometry::HasAnyPerVertexBones() const
{
	for (const auto& mesh : m_Meshes) {
		for (const auto& Vertice : mesh.Vertices)
			if (Vertice.bone != -1)
				return true;
	}

	return false;
}

#define THROW                                                                  \
	RageException::Throw("Parse error in \"%s\" at line %d: \"%s\".",          \
						 sPath.c_str(),                                        \
						 iLineNum,                                             \
						 sLine.c_str())

void
RageModelGeometry::LoadMilkshapeAscii(const std::string& _sPath,
									  bool bNeedsNormals)
{
	auto sPath = _sPath;
	FixSlashesInPlace(sPath);
	const auto sDir = Dirname(sPath);

	RageFile f;
	if (!f.Open(sPath))
		RageException::Throw(
		  "RageModelGeometry::LoadMilkshapeAscii Could not open \"%s\": %s",
		  sPath.c_str(),
		  f.GetError().c_str());

	std::string sLine;
	auto iLineNum = 0;
	char szName[MS_MAX_NAME];
	int nFlags, nIndex;

	RageVec3ClearBounds(m_vMins, m_vMaxs);

	while (f.GetLine(sLine) > 0) {
		iLineNum++;

		if (!strncmp(sLine.c_str(), "//", 2))
			continue;

		int nFrame;
		if (sscanf(sLine.c_str(), "Frames: %d", &nFrame) == 1) {
			// ignore
			// m_pRageModelGeometry->nTotalFrames = nFrame;
		}
		if (sscanf(sLine.c_str(), "Frame: %d", &nFrame) == 1) {
			// ignore
			// m_pRageModelGeometry->nFrame = nFrame;
		}

		auto nNumMeshes = 0;
		if (sscanf(sLine.c_str(), "Meshes: %d", &nNumMeshes) == 1) {
			ASSERT(m_Meshes.empty());
			m_Meshes.resize(nNumMeshes);

			for (auto i = 0; i < nNumMeshes; i++) {
				auto& mesh = m_Meshes[i];
				auto& Vertices = mesh.Vertices;
				auto& Triangles = mesh.Triangles;

				if (f.GetLine(sLine) <= 0)
					THROW;

				// mesh: name, flags, material index
				if (sscanf(sLine.c_str(),
						   "\"%31[^\"]\" %d %d",
						   szName,
						   &nFlags,
						   &nIndex) != 3)
					THROW;

				mesh.sName = szName;
				// mesh.nFlags = nFlags;
				mesh.nMaterialIndex = static_cast<uint8_t>(nIndex);

				mesh.m_iBoneIndex = -1;

				//
				// vertices
				//
				if (f.GetLine(sLine) <= 0)
					THROW;

				auto nNumVertices = 0;
				if (sscanf(sLine.c_str(), "%d", &nNumVertices) != 1)
					THROW;

				Vertices.resize(nNumVertices);

				for (auto j = 0; j < nNumVertices; j++) {
					auto& v = Vertices[j];

					if (f.GetLine(sLine) <= 0)
						THROW;

					if (sscanf(sLine.c_str(),
							   "%d %f %f %f %f %f %d",
							   &nFlags,
							   &v.p[0],
							   &v.p[1],
							   &v.p[2],
							   &v.t[0],
							   &v.t[1],
							   &nIndex) != 7) {
						THROW;
					}

					// vertex.nFlags = nFlags;
					if (nFlags & 1)
						v.TextureMatrixScale.x = 0;
					if (nFlags & 2)
						v.TextureMatrixScale.y = 0;
					if (nFlags & 4) {
						v.t[0] = v.p[0] / v.t[0];
						v.t[1] = v.p[1] / v.t[1];
					}
					v.bone = static_cast<uint8_t>(nIndex);
					RageVec3AddToBounds(v.p, m_vMins, m_vMaxs);
				}

				//
				// normals
				//
				if (f.GetLine(sLine) <= 0)
					THROW;

				auto nNumNormals = 0;
				if (sscanf(sLine.c_str(), "%d", &nNumNormals) != 1)
					THROW;

				vector<RageVector3> Normals;
				Normals.resize(nNumNormals);
				for (auto j = 0; j < nNumNormals; j++) {
					if (f.GetLine(sLine) <= 0)
						THROW;

					RageVector3 Normal;
					if (sscanf(sLine.c_str(),
							   "%f %f %f",
							   &Normal[0],
							   &Normal[1],
							   &Normal[2]) != 3)
						THROW;

					RageVec3Normalize(static_cast<RageVector3*>(&Normal),
									  static_cast<RageVector3*>(&Normal));
					Normals[j] = Normal;
				}

				//
				// triangles
				//
				if (f.GetLine(sLine) <= 0)
					THROW;

				auto nNumTriangles = 0;
				if (sscanf(sLine.c_str(), "%d", &nNumTriangles) != 1)
					THROW;

				Triangles.resize(nNumTriangles);

				for (auto j = 0; j < nNumTriangles; j++) {
					if (f.GetLine(sLine) <= 0)
						THROW;

					uint16_t nIndices[3];
					uint16_t nNormalIndices[3];
					if (sscanf(sLine.c_str(),
							   "%d %hd %hd %hd %hd %hd %hd %d",
							   &nFlags,
							   &nIndices[0],
							   &nIndices[1],
							   &nIndices[2],
							   &nNormalIndices[0],
							   &nNormalIndices[1],
							   &nNormalIndices[2],
							   &nIndex) != 8) {
						THROW;
					}

					// deflate the normals into vertices
					for (auto k = 0; k < 3; k++) {
						ASSERT_M(nIndices[k] < Vertices.size(),
								 ssprintf("mesh \"%s\" tri #%i accesses vertex "
										  "%i, but we only have %i",
										  szName,
										  j,
										  nIndices[k],
										  static_cast<int>(Vertices.size())));
						ASSERT_M(nNormalIndices[k] < Normals.size(),
								 ssprintf("mesh \"%s\" tri #%i accesses normal "
										  "%i, but we only have %i",
										  szName,
										  j,
										  nNormalIndices[k],
										  static_cast<int>(Normals.size())));
						auto& vertex = Vertices[nIndices[k]];
						auto& normal = Normals[nNormalIndices[k]];
						vertex.n = normal;
						// mesh.Vertices[nIndices[k]].n = Normals[
						// nNormalIndices[k] ];
					}

					auto& Triangle = Triangles[j];
					// Triangle.nFlags = nFlags;
					memcpy(&Triangle.nVertexIndices,
						   nIndices,
						   sizeof(Triangle.nVertexIndices));
					// Triangle.nSmoothingGroup = nIndex;
				}
			}
		}
	}

	OptimizeBones();

	if (DISPLAY->SupportsPerVertexMatrixScale()) {
		if (m_Meshes.size() == 2 && m_Meshes[0].sName == m_Meshes[1].sName) {
			MergeMeshes(1, 0);
		}
	}

	// send the finalized vertices to the graphics card
	m_pCompiledGeometry->Set(m_Meshes, bNeedsNormals);
}
