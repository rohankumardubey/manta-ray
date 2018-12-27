#ifndef OBJ_FILE_LOADER_H
#define OBJ_FILE_LOADER_H

#include <manta_math.h>

#include <istream>
#include <vector>

namespace manta {

	struct ObjFace {
		union {
			struct {
				int v1, v2, v3;
				int vn1, vn2, vn3;
				int vt1, vt2, vt3;
			};
			struct {
				int v[3];
				int vn[3];
				int vt[3];
			};
		};
	};

	class ObjFileLoader {
	public:
		ObjFileLoader();
		~ObjFileLoader();

		bool readObjFile(const char *fname);

		unsigned int getVertexCount() const { return (unsigned int)m_vertices.size(); }
		unsigned int getFaceCount() const { return (unsigned int)m_faces.size(); }
		unsigned int getNormalCount() const { return (unsigned int)m_normals.size(); }
		unsigned int getTexCoordCount() const { return (unsigned int)m_texCoords.size(); }

		ObjFace *getFace(unsigned int i) { return m_faces[i]; }
		math::Vector3 *getVertex(unsigned int i) { return m_vertices[i]; }
		math::Vector3 *getNormal(unsigned int i) { return m_normals[i]; }
		math::Vector2 *getTexCoords(unsigned int i) { return m_texCoords[i]; }

		void destroy();

	protected:
		bool readObjFile(std::istream &stream);
		bool readVertexPosition(std::stringstream &s, math::Vector3 *vertex) const;
		bool readVertexNormal(std::stringstream &s, math::Vector3 *normal) const;
		bool readVertexTextureCoords(std::stringstream &s, math::Vector2 *texCoords) const;
		bool readFace(std::stringstream &s, ObjFace *face);

	protected:
		std::vector<ObjFace *> m_faces;
		std::vector<math::Vector3 *> m_vertices;
		std::vector<math::Vector2 *> m_texCoords;
		std::vector<math::Vector3 *> m_normals;

		unsigned int m_currentLine;
	};

} /* namespace manta */

#endif /* OBJ_FILE_LOADER_H */
