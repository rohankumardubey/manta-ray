#ifndef MANTARAY_OBJECT_CHANNEL_TYPES_H
#define MANTARAY_OBJECT_CHANNEL_TYPES_H

#include <piranha.h>

namespace manta {

    struct ObjectChannel {
        static const piranha::ChannelType SceneGeometryChannel;
        static const piranha::ChannelType MeshChannel;
        static const piranha::ChannelType KdTreeChannel;
        static const piranha::ChannelType BsdfChannel;
        static const piranha::ChannelType MaterialChannel;
        static const piranha::ChannelType MicrofacetDistributionChannel;
        static const piranha::ChannelType MaterialLibraryChannel;
        static const piranha::ChannelType SceneObjectChannel;
        static const piranha::ChannelType SceneChannel;
        static const piranha::ChannelType SamplerChannel;
        static const piranha::ChannelType CameraChannel;
        static const piranha::ChannelType ApertureChannel;
        static const piranha::ChannelType LensChannel;
        static const piranha::ChannelType MediaInterfaceChannel;
    };

    template <typename Type> 
    extern inline const piranha::ChannelType *LookupChannelType() { 
        static_assert(false, "Invalid type lookup"); 
        return nullptr;  
    }

    // Forward declaration of all types
    class Mesh;
    class KDTree;
    class BSDF;
    class SimpleBSDFMaterial;
    class MicrofacetDistribution;
    class MaterialLibrary;
    class Material;
    class SceneGeometry;
    class Scene;
    class SceneObject;
    class CameraRayEmitterGroup;
    class Sampler2d;
    class Aperture;
    class Lens;
    class MediaInterface;

    // Helper macro
#define ASSIGN_CHANNEL_TYPE(type, channel) template <> extern inline const piranha::ChannelType *LookupChannelType<type>() { return &ObjectChannel::channel; }

    // Register all types
    ASSIGN_CHANNEL_TYPE(SceneGeometry, SceneGeometryChannel);
    ASSIGN_CHANNEL_TYPE(Mesh, MeshChannel);
    ASSIGN_CHANNEL_TYPE(KDTree, KdTreeChannel);
    ASSIGN_CHANNEL_TYPE(BSDF, BsdfChannel);
    ASSIGN_CHANNEL_TYPE(Material, MaterialChannel);
    ASSIGN_CHANNEL_TYPE(MicrofacetDistribution, MicrofacetDistributionChannel);
    ASSIGN_CHANNEL_TYPE(MaterialLibrary, MaterialLibraryChannel);
    ASSIGN_CHANNEL_TYPE(Scene, SceneChannel);
    ASSIGN_CHANNEL_TYPE(SceneObject, SceneObjectChannel);
    ASSIGN_CHANNEL_TYPE(CameraRayEmitterGroup, CameraChannel);
    ASSIGN_CHANNEL_TYPE(Sampler2d, SamplerChannel);
    ASSIGN_CHANNEL_TYPE(Aperture, ApertureChannel);
    ASSIGN_CHANNEL_TYPE(Lens, LensChannel);
    ASSIGN_CHANNEL_TYPE(MediaInterface, MediaInterfaceChannel);

} /* namespace manta */

#endif /* MANTARAY_OBJECT_CHANNEL_TYPES_H */
