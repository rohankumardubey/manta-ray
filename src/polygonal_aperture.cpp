#include "../include/polygonal_aperture.h"

#include "../include/standard_allocator.h"

#include <assert.h>

manta::PolygonalAperture::PolygonalAperture() {
    m_radius = (math::real)0.0;
    m_bladeCurvature = (math::real)0.0;

    m_edges = nullptr;
    m_edgeCount = 0;

    m_edgeCountInput = nullptr;
    m_angleInput = nullptr;
    m_bladeCurvatureInput = nullptr;
}

manta::PolygonalAperture::~PolygonalAperture() {
    assert(m_edges == nullptr);
}

void manta::PolygonalAperture::configure(int edges, math::real angle, bool halfOffset) {
    static const math::real DEFAULT_ANGLE = math::constants::PI / 2;

    m_edgeCount = edges;
    m_edges = StandardAllocator::Global()->allocate<Edge>(edges);

    math::real dtheta = math::constants::TWO_PI / m_edgeCount;
    math::real offsetAngle = angle + DEFAULT_ANGLE;

    if (halfOffset) offsetAngle += dtheta / 2;

    // Generate vertices
    for (int i = 0; i < m_edgeCount; i++) {
        math::real theta = dtheta * i + offsetAngle;

        math::real x = ::cos(theta);
        math::real y = ::sin(theta);

        m_edges[i].reference.x = x;
        m_edges[i].reference.y = y;
    }

    // Generate edge directions
    for (int i = 0; i < m_edgeCount; i++) {
        int nextEdge = (i + 1) % m_edgeCount;

        math::Vector2 d;

        d.x = m_edges[nextEdge].reference.x - m_edges[i].reference.x;
        d.y = m_edges[nextEdge].reference.y - m_edges[i].reference.y;

        math::real magnitude = ::sqrt(d.x * d.x + d.y * d.y);
        d.x /= magnitude;
        d.y /= magnitude;

        m_edges[i].direction = d;
    }

    // Precompute cache
    for (int i = 0; i < m_edgeCount; i++) {
        Edge &edge = m_edges[i];
        edge.cache = edge.direction.y * edge.reference.x - edge.direction.x * edge.reference.y;
    }
}

void manta::PolygonalAperture::destroy() {
    StandardAllocator::Global()->free(m_edges, m_edgeCount);

    // Reset parameters
    m_edges = nullptr;
    m_edgeCount = 0;
}

bool manta::PolygonalAperture::filter(math::real x, math::real y) const {
    enum class Sign {
        None,
        Negative,
        Positive
    };

    Sign s = Sign::None;
    bool insidePolygon = true;
    for (int i = 0; i < m_edgeCount; i++) {
        const Edge &edge = m_edges[i];

        math::real dx = x - edge.reference.x * m_radius;
        math::real dy = y - edge.reference.y * m_radius;

        // Do a simplified cross product (we're only interested in sign)
        math::real c = edge.direction.y * dx - edge.direction.x * dy;

        if (c == 0) continue;
        else if (c < 0) {
            if (s == Sign::Positive) { insidePolygon = false; break;  }
            else s = Sign::Negative;
        }
        else if (c > 0) {
            if (s == Sign::Negative) { insidePolygon = false; break;  }
            else s = Sign::Positive;
        }
    }

    if (insidePolygon) return true;

    // Early exit if curvature is disabled
    if (m_bladeCurvature == (math::real)0.0) return false;

    math::real mag_2 = x * x + y * y;
    if (mag_2 > m_radius * m_radius) return false;

    // Early exit if the aperture is perfectly circular
    if (m_bladeCurvature == (math::real)1.0) return true;

    const math::real mag_inv = 1 / ::sqrt(mag_2);
    const math::real radial_x = x * mag_inv;
    const math::real radial_y = y * mag_inv;

    math::real polygonLimitR = math::constants::REAL_MAX;
    for (int i = 0; i < m_edgeCount; i++) {
        const Edge &edge = m_edges[i];

        math::real div = edge.direction.y * radial_x - edge.direction.x * radial_y;
        
        if (div != (math::real)0.0) {
            math::real r = edge.cache / div;
            if (r > (math::real)0.0 && r < polygonLimitR) {
                polygonLimitR = r;
            }
        }
    }

    polygonLimitR *= m_radius;

    // Smoothly interpolate between polygonal limit and circular limit
    const math::real radius_mid = m_radius * m_bladeCurvature + polygonLimitR * (1 - m_bladeCurvature);
    if (mag_2 > radius_mid * radius_mid) return false;

    return true;
}

void manta::PolygonalAperture::_initialize() {
    /* void */
}

void manta::PolygonalAperture::_evaluate() {
    Aperture::_evaluate();

    piranha::native_float angle, curvature;
    piranha::native_int blades;

    m_angleInput->fullCompute((void *)&angle);
    m_bladeCurvatureInput->fullCompute((void *)&curvature);
    m_edgeCountInput->fullCompute((void *)&blades);

    m_bladeCurvature = (math::real)curvature;
    
    configure(blades, (math::real)angle);
}

void manta::PolygonalAperture::_destroy() {
    /* void */
}

void manta::PolygonalAperture::registerInputs() {
    Aperture::registerInputs();

    registerInput(&m_angleInput, "angle");
    registerInput(&m_bladeCurvatureInput, "curvature");
    registerInput(&m_edgeCountInput, "blades");
}
