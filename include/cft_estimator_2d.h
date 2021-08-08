#ifndef MANTARAY_CFT_ESTIMATOR_H
#define MANTARAY_CFT_ESTIMATOR_H

#include "complex_math.h"
#include "manta_math.h"
#include "complex_map_2d.h"
#include "mipmap.h"

namespace manta {

    // Forward declarations
    class ComplexMap2D;

    class CftEstimator2D {
    public:
        CftEstimator2D();
        ~CftEstimator2D();

        void initialize(
            ComplexMap2D *cft,
            math::real_d physicalWidth,
            math::real_d physicalHeight);
        void destroy();

        math::Complex sample(math::real_d freq_x, math::real_d freq_y, math::real_d w) const;
        math::Complex sampleDiscrete(int i, int j) const;

        math::real_d getFrequencyX(int i) const;
        math::real_d getFrequencyY(int i) const;

        const ComplexMap2D *getApproximation() const { return m_discreteApproximation; }

        math::real_d getPhysicalWidth() { return m_physicalWidth; }
        math::real_d getPhysicalHeight() { return m_physicalWidth; }

        math::real_d getHorizontalFreqRange() const;
        math::real_d getVerticalFreqRange() const;

        math::real_d getHorizontalFreqStep() const;
        math::real_d getVerticalFreqStep() const;

        static math::real_d getFreqRange(int samples, math::real_d physicalDim);
        static math::real_d getFreqStep(math::real_d physicalDim);

        static math::real_d getMinPhysicalDim(math::real_d freqStep, math::real_d minDim);
        static int getMinSamples(math::real_d maxFreq, math::real_d physicalDim, int maxSamples = -1);

    protected:
        ComplexMap2D *m_discreteApproximation;

        math::real_d m_physicalWidth;
        math::real_d m_physicalHeight;

        int m_horizontalSamples;
        int m_verticalSamples;
    };

} /* namespace manta */

#endif /* MANTARAY_CFT_ESTIMATOR_H */
