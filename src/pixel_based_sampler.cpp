#include "../include/pixel_based_sampler.h"

manta::PixelBasedSampler::PixelBasedSampler() {
    m_current1dDimension = m_current2dDimension = 0;
}

manta::PixelBasedSampler::~PixelBasedSampler() {
    /* void */
}

void manta::PixelBasedSampler::configure(int samplesPerPixel, int dimensionCount) {
    setSamplesPerPixel(samplesPerPixel);

    for (int i = 0; i < dimensionCount; i++) {
        m_1dSamples.push_back(std::vector<math::real>(samplesPerPixel));
        m_2dSamples.push_back(std::vector<math::Vector2>(samplesPerPixel));
    }
}

manta::math::real manta::PixelBasedSampler::generate1d() {
    if (m_current1dDimension < m_1dSamples.size()) {
        return m_1dSamples[m_current1dDimension++][m_currentPixelSample];
    }
    else return uniformRandom();
}

manta::math::Vector2 manta::PixelBasedSampler::generate2d() {
    if (m_current2dDimension < m_2dSamples.size()) {
        return m_2dSamples[m_current2dDimension++][m_currentPixelSample];
    }
    else {
        return math::Vector2(
            uniformRandom(),
            uniformRandom());
    }
}

bool manta::PixelBasedSampler::startNextSample() {
    m_current1dDimension = m_current2dDimension = 0;

    return Sampler::startNextSample();
}
