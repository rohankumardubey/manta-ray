#ifndef MANTARAY_OPAQUE_MEDIA_INTERFACE_H
#define MANTARAY_OPAQUE_MEDIA_INTERFACE_H

#include "media_interface.h"

namespace manta {

    class OpaqueMediaInterface {
    public:
        OpaqueMediaInterface();
        virtual ~OpaqueMediaInterface();

        virtual math::real fresnelTerm(const math::Vector &i, const math::Vector &m) const 
            { return (math::real)1.0; /* Perfect reflection */ }
    };

} /* namespace manta */

#endif /* MANTARAY_OPAQUE_MEDIA_INTERFACE */
