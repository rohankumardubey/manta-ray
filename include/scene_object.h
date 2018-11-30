#ifndef SCENE_OBJECT_H
#define SCENE_OBJECT_H

#include <string>

namespace manta {

	class SceneGeometry;
	class Material;

	class SceneObject {
	public:
		SceneObject();
		~SceneObject();

		void setGeometry(SceneGeometry *geometry) { m_geometry = geometry; }
		SceneGeometry *getGeometry() { return m_geometry; }

		void setMaterial(Material *material) { m_material = material; }
		Material *getMaterial() { return m_material; }

		void setName(const char *name) { m_name = name; }
		const char *getName() const { return m_name.c_str(); }

	protected:
		SceneGeometry *m_geometry;
		Material *m_material;

		std::string m_name;
	};

}

#endif /* SCENE_OBJECT_H */
