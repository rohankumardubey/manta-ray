#include "../include/microfacet_brdf.h"

#include "../include/microfacet_distribution.h"
#include "../include/vector_node_output.h"

manta::MicrofacetBRDF::MicrofacetBRDF() {
    m_distribution = nullptr;
    m_distributionInput = nullptr;
    m_mediaInterface = nullptr;
    m_mediaInterfaceInput = nullptr;
}

manta::MicrofacetBRDF::~MicrofacetBRDF() {
    /* void */
}

manta::math::Vector manta::MicrofacetBRDF::sampleF(
    const IntersectionPoint *surfaceInteraction,
    const math::Vector2 &u,
    const math::Vector &i,
    math::Vector *o,
    math::real *pdf,
    RayFlags *flags,
    StackAllocator *stackAllocator) 
{
    const math::Vector m = m_distribution->generateMicrosurfaceNormal(surfaceInteraction, u);
    const math::Vector ri = math::reflect(i, m);
    const math::real o_dot_m = math::getScalar(math::dot(ri, m));

    *o = ri;

    if (!sameHemisphere(*o, i)) {
        *pdf = (math::real)0.0;
        return math::constants::Zero;
    }

    if (!m_distribution->isDelta(surfaceInteraction)) {
        *pdf = m_distribution->calculatePDF(m, surfaceInteraction) / ::abs(4 * o_dot_m);
        *flags = RayFlag::Reflection;
        return MicrofacetBRDF::f(surfaceInteraction, i, *o, stackAllocator);
    }
    else {
        *pdf = 1.0;
        *flags = RayFlag::Reflection | RayFlag::Delta;

        const math::real F = (m_mediaInterface == nullptr)
            ? (math::real)1.0
            : m_mediaInterface->fresnelTerm(o_dot_m, surfaceInteraction->m_direction);

        return math::mul(
            getReflectivity(surfaceInteraction),
            math::loadScalar(F));
    }    
}

manta::math::Vector manta::MicrofacetBRDF::f(
    const IntersectionPoint *surfaceInteraction,
    const math::Vector &i,
    const math::Vector &o,
    StackAllocator *stackAllocator)
{
    if (m_distribution->isDelta(surfaceInteraction)) return math::constants::Zero;

    const math::real cosThetaO = std::abs(math::getZ(o));
    const math::real cosThetaI = std::abs(math::getZ(i));

    math::Vector wh = math::add(i, o);

    if (cosThetaO == 0 || cosThetaI == 0) return math::constants::Zero;
    if (math::getX(wh) == 0 && math::getY(wh) == 0 && math::getZ(wh) == 0) return math::constants::Zero;

    wh = math::normalize(wh);

    const math::real o_dot_wh = math::getScalar(math::dot(wh, o));

    const math::real F = (m_mediaInterface == nullptr)
        ? (math::real)1.0
        : m_mediaInterface->fresnelTerm(o_dot_wh, surfaceInteraction->m_direction);

    const math::Vector reflectivity = math::loadScalar(
        m_distribution->calculateDistribution(wh, surfaceInteraction) *
        m_distribution->bidirectionalShadowMasking(i, o, wh, surfaceInteraction) *
        F / (4 * cosThetaI * cosThetaO));

    return math::mul(
        reflectivity,
        getReflectivity(surfaceInteraction));
}

manta::math::real manta::MicrofacetBRDF::pdf(
    const IntersectionPoint *surfaceInteraction,
    const math::Vector &i,
    const math::Vector &o) 
{
    if (m_distribution->isDelta(surfaceInteraction)) {
        return 0;
    }

    if (!sameHemisphere(i, o)) {
        return 0;
    }

    const math::Vector wh = math::normalize(math::add(i, o));
    const math::real o_dot_wh = math::getScalar(math::dot(wh, o));

    const math::real cosThetaO = math::getZ(o);
    const math::real cosThetaI = math::getZ(i);

    const math::real pdf = m_distribution->calculatePDF(wh, surfaceInteraction) / ::abs(4 * o_dot_wh);
    return pdf;
}

void manta::MicrofacetBRDF::setBaseReflectivity(const math::Vector &reflectivity) {
    m_reflectivity.setDefault(reflectivity);
}

manta::math::Vector manta::MicrofacetBRDF::getReflectivity(
    const IntersectionPoint *surfaceInteraction) 
{
    return m_reflectivity.sample(surfaceInteraction);
}

void manta::MicrofacetBRDF::_evaluate() {
    BXDF::_evaluate();
    m_distribution = getObject<MicrofacetDistribution>(m_distributionInput);
    m_mediaInterface = getObject<MediaInterface>(m_mediaInterfaceInput);
}

piranha::Node * manta::MicrofacetBRDF::_optimize(piranha::NodeAllocator *nodeAllocator) {
    m_reflectivity.optimize();

    return this;
}

void manta::MicrofacetBRDF::registerInputs() {
    BXDF::registerInputs();

    registerInput(&m_distributionInput, "distribution");
    registerInput(m_reflectivity.getPortAddress(), "reflectivity");
    registerInput(&m_mediaInterfaceInput, "media_interface");
}
