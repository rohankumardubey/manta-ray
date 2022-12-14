#ifndef MANTARAY_PERFECT_SPECULAR_REFLECTION_BRDF_H
#define MANTARAY_PERFECT_SPECULAR_REFLECTION_BRDF_H

#include "bxdf.h"

namespace manta {

    class StackAllocator;

    class PerfectSpecularReflectionBRDF : public BXDF {
    public:
        PerfectSpecularReflectionBRDF();
        virtual ~PerfectSpecularReflectionBRDF();

        virtual math::Vector sampleF(
            const IntersectionPoint *surfaceInteraction,
            const math::Vector2 &u,
            const math::Vector &i,
            math::Vector *o,
            math::real *pdf,
            RayFlags *flags,
            StackAllocator *stackAllocator);

        virtual math::Vector f(
            const IntersectionPoint *surfaceInteraction,
            const math::Vector &i,
            const math::Vector &o,
            StackAllocator *stackAllocator);

        virtual math::real pdf(
            const IntersectionPoint *surfaceInteraction,
            const math::Vector &i,
            const math::Vector &o);
    };

} /* namespace manta */

#endif /* MANTARAY_PERFECT_SPECULAR_REFLECTION_BRDF_H */
