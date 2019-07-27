#ifndef MANTARAY_MICROFACET_DISTRIBUTION_H
#define MANTARAY_MICROFACET_DISTRIBUTION_H

#include "node.h"

#include "manta_math.h"
#include "intersection_point.h"
#include "microfacet_distribution_node_output.h"

namespace manta {

	// Forward declarations
	class StackAllocator;

	class MicrofacetDistribution : public Node {
	public:
		MicrofacetDistribution();
		virtual ~MicrofacetDistribution();

		virtual math::Vector generateMicrosurfaceNormal(NodeSessionMemory *mem) const = 0;
		virtual math::real calculatePDF(const math::Vector &m, NodeSessionMemory *mem) const;
		virtual math::real calculateDistribution(const math::Vector &m, 
			NodeSessionMemory *mem) const = 0;
		virtual math::real calculateG1(const math::Vector &v, const math::Vector &m, 
			NodeSessionMemory *mem) const = 0;
		virtual math::real bidirectionalShadowMasking(const math::Vector &i, 
			const math::Vector &o, const math::Vector &m, NodeSessionMemory *mem) const;

		math::real smithBidirectionalShadowMasking(const math::Vector &i, const math::Vector &o, 
			const math::Vector &m, NodeSessionMemory *mem) const;

		MicrofacetDistributionNodeOutput *getMainOutput() { return &m_output; }

	protected:
		MicrofacetDistributionNodeOutput m_output;

		virtual void registerOutputs();
	};

} /* namespace manta */

#endif /* MANTARAY_MICROFACET_DISTRIBUTION_H */
