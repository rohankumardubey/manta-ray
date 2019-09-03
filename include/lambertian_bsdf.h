#ifndef MANTARAY_LAMBERTIAN_BSDF_H
#define MANTARAY_LAMBERTIAN_BSDF_H

#include "bsdf.h"

namespace manta {

	class VectorMaterialNode;

	struct LambertMemory {
		math::Vector reflectance;
	};

	class LambertianBSDF : public BSDF {
	public:
		LambertianBSDF();
		virtual ~LambertianBSDF();

		virtual math::Vector sampleF(const IntersectionPoint *surfaceInteraction, 
			const math::Vector &i, math::Vector *o, math::real *pdf, 
			StackAllocator *stackAllocator) const;
	};

} /* namespace manta */

#endif /* MANTARAY_LAMBERTIAN_BSDF_H */
