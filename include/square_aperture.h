#ifndef MANTARAY_SQUARE_APERTURE_H
#define MANTARAY_SQUARE_APERTURE_H

#include "aperture.h"

namespace manta {

	class SquareAperture : public Aperture {
	public:
		SquareAperture();
		virtual ~SquareAperture();

		virtual bool filter(math::real x, math::real y) const;
	};

} /* namespace manta */

#endif /* MANTARAY_SQUARE_APERTURE_H */
