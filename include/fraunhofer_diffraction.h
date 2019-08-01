#ifndef MANTARAY_FRAUNHOFER_DIFFRACTION_H
#define MANTARAY_FRAUNHOFER_DIFFRACTION_H

#include "convolution.h"

#include "vector_map_2d.h"
#include "scalar_map_2d.h"
#include "complex_map_2d.h"

namespace manta {

	// Forward declarations
	class ImageByteBuffer;
	class Aperture;
	class CftEstimator2D;
	class CmfTable;
	class Spectrum;
	class TextureNode;

	class FraunhoferDiffraction {
	public:
		struct Settings {
			int maxSamples;
			int textureSamples;
			int minWavelength; // nm
			int maxWavelength; // nm
			int wavelengthStep; // nm
			math::real_d frequencyMultiplier;
			math::real_d safetyFactor;

			// Debugging flags
			bool saveApertureFunction;
		};

	public:
		FraunhoferDiffraction();
		~FraunhoferDiffraction();

		void generate(const Aperture *aperture, const VectorMap2D *dirtMap, 
			int outputResolution, math::real physicalSensorWidth, 
			CmfTable *colorTable, Spectrum *sourceSpectrum, 
			const Settings *settings = nullptr);
		virtual void destroy();

		math::Vector samplePattern(math::real dx, math::real dy) const;

		const VectorMap2D *getDiffractionPattern() const { return &m_diffractionPattern; }
		const ComplexMap2D *getApertureFunction() const { return &m_apertureFunction; }
		math::real getPhysicalSensorWidth() const { return m_physicalSensorWidth; }
		math::real getSensorElementWidth() const { return m_sensorElementWidth; }

		static void setDefaultSettings(Settings *settings);

	protected:
		void normalize();

		void generateMap(const CftEstimator2D *estimator, const Settings *settings, 
			int threadCount, VectorMap2D *target) const;
		void _generateMap(const CftEstimator2D *estimator, const Settings *settings, 
			int startRow, int endRow, VectorMap2D *target) const;

	protected:
		VectorMap2D m_diffractionPattern;
		ComplexMap2D m_apertureFunction;
		CmfTable *m_colorTable;
		Spectrum *m_sourceSpectrum;

		math::real m_physicalSensorWidth;
		math::real m_sensorElementWidth;
	};

} /* namespace manta */

#endif /* MANTARAY_FRAUNHOFER_DIFFRACTION_H */
