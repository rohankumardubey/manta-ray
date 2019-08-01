#ifndef MANTARAY_LIGHT_RAY_H
#define MANTARAY_LIGHT_RAY_H

#include "manta_math.h"

namespace manta {

    class LightRay {
    public:
        LightRay();
        ~LightRay();

        math::Vector getDirection() const { return m_direction; }
        void setDirection(const math::Vector &dir) { m_direction = dir; }

        math::Vector getSource() const { return m_source; }
        void setSource(const math::Vector &source) { m_source = source; }

        math::Vector getIntensity() const { return m_intensity; }
        void setIntensity(const math::Vector &intensity) { m_intensity = intensity; }

        math::Vector getWeightedIntensity() const { return math::mul(m_intensity, m_weight); }

        void setWeight(math::Vector weight) { m_weight = weight; }
        math::Vector getWeight() const { return m_weight; }

        void setPdf(math::real pdf) { m_pdf = pdf; }
        math::real getPdf() const { return m_pdf; }

        void setAperturePoint(const math::Vector2 &aperturePoint) { m_aperturePoint = aperturePoint; }
        math::Vector2 getAperturePoint() const { return m_aperturePoint; }

        void calculateTransformations();

        int getKX() const { return m_kx; }
        int getKY() const { return m_ky; }
        int getKZ() const { return m_kz; }
        const math::Vector3 &getShear() const { return m_shear; }
        math::Vector getPermutedDirection() const { return m_permutedDirection; }
        math::Vector getInverseDirection() const { return m_inverseDirection; }

        void setMeta(int meta) { m_meta = meta; }
        int getMeta() const { return m_meta; }

    protected:
        int m_meta;

        math::Vector2 m_aperturePoint;

        math::Vector m_direction;
        math::Vector m_source;

        math::Vector m_intensity;
        math::Vector m_weight;

        math::real m_pdf;

        int m_kx, m_ky, m_kz;
        math::Vector3 m_shear;
        math::Vector m_permutedDirection;
        math::Vector m_inverseDirection;
    };

} /* namespace manta */

#endif /* MANTARAY_LIGHT_RAY_H */
