#ifndef MANTARAY_SCENE_GEOMETRY_H
#define MANTARAY_SCENE_GEOMETRY_H

#include "object_reference_node.h"

#include "manta_math.h"
#include "runtime_statistics.h"

namespace manta {

    // Forward declarations
    struct IntersectionPoint;
    struct CoarseIntersection;
    class LightRay;
    class IntersectionList;
    class SceneObject;
    class StackAllocator;

    class SceneGeometry : public ObjectReferenceNode<SceneGeometry> {
    public:
        SceneGeometry();
        ~SceneGeometry();

        virtual bool findClosestIntersection(LightRay *ray, 
            CoarseIntersection *intersection, math::real minDepth, math::real maxDepth, 
            StackAllocator *s /**/ STATISTICS_PROTOTYPE) const = 0;
        virtual bool occluded(const math::Vector &p0, const math::Vector &d, math::real maxDepth /**/ STATISTICS_PROTOTYPE) const = 0;
        virtual void fineIntersection(const math::Vector &r, 
            IntersectionPoint *p, const CoarseIntersection *hint) const = 0;
        virtual bool fastIntersection(LightRay *ray) const = 0;

        void setId(int id);
        int getId() const;

        void setPosition(const math::Vector &position) { m_position = position; }
        math::Vector getPosition() const { return m_position; }

    protected:
        virtual void _initialize();
        virtual void _evaluate();
        virtual void registerInputs();

        piranha::pNodeInput m_positionInput;
        piranha::pNodeInput m_materialsInput;
        piranha::pNodeInput m_defaultMaterial;

    protected:
        math::Vector m_position;

        int m_id;

        MaterialLibrary *m_library;
        int m_defaultMaterialIndex;
    };

} /* namespace manta */

#endif /* MANTARAY_SCENE_GEOMETRY_H */
