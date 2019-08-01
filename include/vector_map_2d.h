#ifndef MANTARAY_VECTOR_MAP_2D_H
#define MANTARAY_VECTOR_MAP_2D_H

#include "manta_math.h"

#include "margins.h"

namespace manta {

	// Forward declarations
	class ImageByteBuffer;
	class ImagePlane;

	class VectorMap2D {
	public:
		VectorMap2D();
		~VectorMap2D();

		void initialize(int width, int height, 
			const math::Vector &value = math::constants::Zero);
		void destroy();

		math::Vector sample(math::real u, math::real v) const;
		math::Vector triangleSample(math::real u, math::real v) const;
		math::Vector get(int u, int v) const;
		math::Vector getClip(int u, int v) const;
		void set(const math::Vector &value, int u, int v);

		void fillByteBuffer(ImageByteBuffer *target, bool correctGamma) const;

		int getWidth() const { return m_width; }
		int getHeight() const { return m_height; }

		void scale(const math::Vector &s);
		void applyGamma(math::real gamma);

		math::real getMaxMagnitude() const;

		void padSafe(VectorMap2D *target, Margins *margins) const;
		int getSafeWidth() const;
		int getSafeHeight() const;

		void roll(VectorMap2D *target) const;
		void copy(const VectorMap2D *source);
		void copy(const ImagePlane *plane);

	protected:
		math::Vector *m_data;

		int m_width;
		int m_height;
	};

} /* namespace manta */

#endif /* MANTARAY_VECTOR_MAP_2D_H */
