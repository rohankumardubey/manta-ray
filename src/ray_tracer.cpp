#include "../include/ray_tracer.h"

#include "../include/material.h"
#include "../include/light_ray.h"
#include "../include/camera_ray_emitter.h"
#include "../include/camera_ray_emitter_group.h"
#include "../include/scene.h"
#include "../include/scene_object.h"
#include "../include/intersection_point.h"
#include "../include/scene_geometry.h"
#include "../include/stack_allocator.h"
#include "../include/memory_management.h"
#include "../include/worker.h"
#include "../include/camera_ray_emitter_group.h"
#include "../include/path_recorder.h"
#include "../include/standard_allocator.h"
#include "../include/stack_list.h"
#include "../include/coarse_intersection.h"
#include "../include/image_plane.h"
#include "../include/os_utilities.h"
#include "../include/material_library.h"
#include "../include/vector_node_output.h"
#include "../include/bsdf.h"
#include "../include/sampler.h"
#include "../include/console.h"
#include "../include/light.h"
#include "../include/render_pattern.h"
#include "../include/spiral_render_pattern.h"

#include <iostream>
#include <thread>
#include <chrono>
#include <sstream>
#include <stack>

manta::RayTracer::RayTracer() {
    m_multithreadedInput = nullptr;
    m_threadCountInput = nullptr;
    m_renderPatternInput = nullptr;
    m_backgroundColorInput = nullptr;
    m_deterministicSeedInput = nullptr;
    m_materialLibraryInput = nullptr;
    m_sceneInput = nullptr;
    m_cameraInput = nullptr;
    m_samplerInput = nullptr;
    m_outputImage = nullptr;
    m_workers = nullptr;
    m_renderPattern = nullptr;
    m_directLightSamplingEnableInput = nullptr;

    m_directLightSampling = true;
    m_deterministicSeed = false;
    m_pathRecordingOutputDirectory = "";
    m_backgroundColor = math::constants::Zero;
    m_currentRay = 0;
    m_lastRayPrint = 0;

    m_threadCount = 0;
}

manta::RayTracer::~RayTracer() {
    /* void */
}

void manta::RayTracer::traceAll(const Scene *scene, CameraRayEmitterGroup *group, ImagePlane *target) {
    // Simple performance metrics for now
    auto startTime = std::chrono::system_clock::now();

    // Reset counter
    m_currentRay = 0;

    // Set up the emitter group
    group->configure();

    // Create jobs
    RenderPattern::PatternParameters params;
    params.group = group;
    params.scene = scene;
    params.target = target;

    if (m_renderPattern == nullptr) {
        SpiralRenderPattern renderPattern;
        renderPattern.setBlockWidth(64);
        renderPattern.setBlockHeight(64);
        renderPattern.generateJobs(params, &m_jobQueue);
    }
    else {
        m_renderPattern->generateJobs(params, &m_jobQueue);
    }

    // Hide the cursor to avoid annoying blinking artifact
    showConsoleCursor(false);

    // Create and start all threads
    createWorkers();
    startWorkers();
    waitForWorkers();

    target->normalize();

    // Print a single new line to terminate progress display
    // See RayTracer::incrementRayCompletion
    Session::get().getConsole()->out("\n");

    auto endTime = std::chrono::system_clock::now();
    std::chrono::duration<double> diff = endTime - startTime;

    std::stringstream ss_out;

#if ENABLE_DETAILED_STATISTICS
    RuntimeStatistics combinedStatistics;
    combinedStatistics.reset();

    // Add all statistics from workers
    for (int i = 0; i < m_threadCount; i++) {
        combinedStatistics.add(m_workers[i].getStatistics());
    }

    // Display statistics
    ss_out <<        "================================================" << std::endl;
    ss_out <<        "Run-time Statistics" << std::endl;
    ss_out <<        "------------------------------------------------" << std::endl;
    const int counterCount = (int)RuntimeStatistics::Counter::Count;
    for (int i = 0; i < counterCount; i++) {
        unsigned __int64 counter = combinedStatistics.counters[i];
        const char *counterName = combinedStatistics.getCounterName((RuntimeStatistics::Counter)i);
        std::stringstream ss;
        ss << counterName << ":";
        ss_out << ss.str();
        for (int j = 0; j < (37 - ss.str().length()); j++) {
            ss_out << " ";
        }

        ss_out << counter << std::endl;
    }
#endif /* ENABLE_DETAILED_STATISTICS */

    ss_out <<        "================================================" << std::endl;
    ss_out <<        "Total processing time:               " << diff.count() << " s" << std::endl;
    ss_out <<        "------------------------------------------------" << std::endl;
    ss_out <<        "Standard allocator peak usage:       " << StandardAllocator::Global()->getMaxUsage() / (double)MB << " MB" << std::endl;
    ss_out <<        "Main allocator peak usage:           " << m_stack.getMaxUsage() / (double)MB << " MB" << std::endl;
    unsigned __int64 totalUsage = m_stack.getMaxUsage() + StandardAllocator::Global()->getMaxUsage();
    for (int i = 0; i < m_threadCount; i++) {
        std::stringstream ss;
        ss <<            "Worker " << i << " peak memory usage:";
        ss_out << ss.str();
        for (int j = 0; j < (37 - ss.str().length()); j++) {
            ss_out << " ";
        }
        ss_out << m_workers[i].getMaxMemoryUsage() / (double)MB << " MB" << std::endl;
        totalUsage += m_workers[i].getMaxMemoryUsage();
    }
    ss_out <<        "                                     -----------" << std::endl;
    ss_out <<        "Total memory usage:                  " << totalUsage / (double)MB << " MB" << std::endl;
    ss_out <<        "================================================" << std::endl;

    Session::get().getConsole()->out(ss_out.str());

    // Bring back the cursor
    showConsoleCursor(true);
}

void manta::RayTracer::tracePixel(int px, int py, const Scene *scene, CameraRayEmitterGroup *group, ImagePlane *target) {
    group->initialize();
    target->initialize(group->getResolutionX(), group->getResolutionY());

    // Create the singular job for the pixel
    Job job;
    job.scene = scene;
    job.group = group;
    job.target = target;
    job.startX = px;
    job.endX = px;
    job.startY = py;
    job.endY = py;

    m_jobQueue.push(job);

    // Create and start all threads
    createWorkers();
    startWorkers();
    waitForWorkers();
}

void manta::RayTracer::configure(mem_size stackSize, mem_size workerStackSize, int threadCount, bool multithreaded) {
    m_stack.initialize(stackSize);
    m_multithreaded = multithreaded;
    m_threadCount = threadCount;
    m_workerStackSize = workerStackSize;
}

void manta::RayTracer::destroy() {
    if (m_outputImage != nullptr) {
        m_outputImage->destroy();
        delete m_outputImage;
    }

    destroyWorkers();
}

void manta::RayTracer::incrementRayCompletion(const Job *job, int increment) {
    m_outputLock.lock();
    
    const int emitterCount = job->group->getResolutionX() * job->group->getResolutionY();
    m_currentRay += increment;

    // Print in increments of 1000 or the last 1000 one by one
    if ((m_currentRay - m_lastRayPrint) > 1000 || m_currentRay >= (emitterCount - 1000)) {
        std::stringstream ss;
        ss << "Pixel " << m_currentRay << "/" << emitterCount << "                      \r";
        Session::get().getConsole()->out(ss.str());

        m_lastRayPrint = m_currentRay;
    }

    m_outputLock.unlock();
}

manta::math::Vector manta::RayTracer::uniformSampleOneLight(IntersectionPoint *point, const Scene *scene, Sampler *sampler, IntersectionPointManager *manager, StackAllocator *stackAllocator) const {
    const int lightCount = scene->getLightCount();
    if (lightCount == 0) return math::constants::Zero;
    const int light_i = std::min((int)(sampler->generate1d() * lightCount), lightCount - 1);

    Light *light = scene->getLight(light_i);

    const math::Vector2 uLight = sampler->generate2d();
    const math::Vector2 uScattering = sampler->generate2d();

    const math::Vector l_n = math::loadScalar((math::real)lightCount);
    return math::mul(
        l_n,
        estimateDirect(point, uScattering, light, uLight, scene, sampler, manager, stackAllocator)
    );
}

manta::math::Vector manta::RayTracer::estimateDirect(
    IntersectionPoint *point,
    const math::Vector2 &uScattering,
    const Light *light,
    const math::Vector2 &uLight,
    const Scene *scene,
    Sampler *sampler,
    IntersectionPointManager *manager,
    StackAllocator *stackAllocator /**/
    STATISTICS_PROTOTYPE) const
{
    math::Vector wi;
    math::real lightPdf = 0, scatteringPdf = 0;
    math::Vector Ld = math::constants::Zero;
    math::real depth;
    math::Vector Li = light->sampleIncoming(*point, uLight, &wi, &lightPdf, &depth);

    math::Vector f;
    if (lightPdf > 0) {
        f = point->m_bsdf->f(point, point->m_lightRay->getDirection(), math::negate(wi), true);
        scatteringPdf = point->m_bsdf->pdf(point, point->m_lightRay->getDirection(), math::negate(wi));
        if (scatteringPdf != 0) {
            const math::Vector p0 = (math::getScalar(math::dot(wi, point->m_vertexNormal)) > 0)
                ? point->m_outside
                : point->m_inside;
            const bool occluded = this->occluded(scene, p0, wi, depth /**/ STATISTICS_PARAM_INPUT);
            if (occluded) {
                Li = math::constants::Zero;
            }
        }
        else {
            Li = math::constants::Zero;
        }

        const math::real w = powerHeuristic(1, lightPdf, 1, scatteringPdf);
        Ld = math::add(Ld, math::mul(f, math::mul(Li, math::loadScalar(w / lightPdf))));
    }

    RayFlags flags = RayFlag::None;
    f = point->m_bsdf->sampleF(point, uScattering, math::negate(point->m_lightRay->getDirection()), &wi, &scatteringPdf, &flags, stackAllocator, true);
    f = math::mask(f, math::constants::MaskOffW);
    if (scatteringPdf > 0 && (flags & RayFlag::Delta) == 0) {
        lightPdf = light->pdfIncoming(*point, wi);
        if (lightPdf == 0) {
            return Ld;
        }

        const math::real weight = powerHeuristic(1, scatteringPdf, 1, lightPdf);
        depth = math::constants::REAL_MAX;
        if (light->intersect(point->m_position, wi, &depth)) {
            const math::Vector p0 = ((flags & RayFlag::Transmission) > 0)
                ? point->m_inside
                : point->m_outside;
            const bool occluded = this->occluded(scene, p0, wi, depth /**/ STATISTICS_PARAM_INPUT);

            if (!occluded) {
                // TODO: inputs are technically wrong
                IntersectionPoint p;
                p.m_position = p0;
                Li = light->L(p, wi);
            }
            else {
                Li = math::constants::Zero;
            }

            Ld = math::add(
                Ld,
                math::mul(math::mul(f, Li), math::loadScalar(weight / scatteringPdf)));
        }
    }

    return Ld;
}

manta::math::real manta::RayTracer::powerHeuristic(int nf, math::real f_pdf, int ng, math::real g_pdf) {
    const math::real f = nf * f_pdf;
    const math::real g = ng * g_pdf;
    return (f * f) / (f * f + g * g);
}

void manta::RayTracer::_evaluate() {
    piranha::native_int threadCount;
    piranha::native_bool multithreaded;
    piranha::native_bool deterministicSeed;
    piranha::native_bool enableDirectLightSampling;
    CameraRayEmitterGroup *camera;
    Scene *scene;

    static_cast<piranha::NodeOutput *>(m_multithreadedInput)->fullCompute((void *)&multithreaded);
    static_cast<piranha::NodeOutput *>(m_threadCountInput)->fullCompute((void *)&threadCount);
    static_cast<piranha::NodeOutput *>(m_deterministicSeedInput)->fullCompute((void *)&deterministicSeed);
    static_cast<piranha::NodeOutput *>(m_directLightSamplingEnableInput)->fullCompute((void *)&enableDirectLightSampling);
    static_cast<VectorNodeOutput *>(m_backgroundColorInput)->sample(nullptr, (void *)&m_backgroundColor);

    m_directLightSampling = enableDirectLightSampling;

    m_materialManager = getObject<MaterialLibrary>(m_materialLibraryInput);
    m_sampler = getObject<Sampler>(m_samplerInput);
    m_renderPattern = getObject<RenderPattern>(m_renderPatternInput);
    camera = getObject<CameraRayEmitterGroup>(m_cameraInput);
    scene = getObject<Scene>(m_sceneInput);

    configure(200 * MB, 50 * MB, threadCount, multithreaded);
    setDeterministicSeedMode(deterministicSeed);

    traceAll(scene, camera, camera->getImagePlane());

    m_outputImage = new VectorMap2D();
    m_outputImage->copy(camera->getImagePlane());

    m_output.setMap(m_outputImage);
}

void manta::RayTracer::_initialize() {
    /* void */
}

void manta::RayTracer::_destroy() {
    destroy();
}

void manta::RayTracer::registerInputs() {
    registerInput(&m_multithreadedInput, "multithreaded");
    registerInput(&m_threadCountInput, "threads");
    registerInput(&m_renderPatternInput, "render_pattern");
    registerInput(&m_backgroundColorInput, "background");
    registerInput(&m_deterministicSeedInput, "deterministic_seed");
    registerInput(&m_materialLibraryInput, "materials");
    registerInput(&m_sceneInput, "scene");
    registerInput(&m_cameraInput, "camera");
    registerInput(&m_samplerInput, "sampler");
    registerInput(&m_directLightSamplingEnableInput, "direct_light_sampling");
}

void manta::RayTracer::registerOutputs() {
    registerOutput(&m_output, "image");
}

void manta::RayTracer::createWorkers() {
    m_workers = new Worker[m_threadCount];

    std::mt19937 seedGenerator;
    for (int i = 0; i < m_threadCount; i++) {
        m_workers[i].initialize(m_workerStackSize, this, i, m_deterministicSeed, m_pathRecordingOutputDirectory, seedGenerator());
    }
}

void manta::RayTracer::startWorkers() {
    const int workerCount = m_threadCount;
    std::stringstream ss;
    ss << "Starting " << workerCount << " workers" << std::endl;
    Session::get().getConsole()->out(ss.str());

    for (int i = 0; i < workerCount; i++) {
        m_workers[i].start(m_multithreaded);
    }
}

void manta::RayTracer::waitForWorkers() {
    const int workerCount = m_threadCount;
    for (int i = 0; i < workerCount; i++) {
        m_workers[i].join();
    }
}

void manta::RayTracer::destroyWorkers() {
    const int workerCount = m_threadCount;
    for (int i = 0; i < workerCount; i++) {
        m_workers[i].destroy();
    }

    if (m_workers != nullptr) {
        delete[] m_workers;
        m_workers = nullptr;
    }
}

void manta::RayTracer::depthCull(
    const Scene *scene,
    LightRay *ray,
    SceneObject **closestObject,
    IntersectionPoint *point,
    StackAllocator *s,
    math::real startingDepth
    /**/ STATISTICS_PROTOTYPE) const
{
    CoarseIntersection closestIntersection;
    closestIntersection.sceneObject = nullptr;

    Light *light = nullptr;
    math::real closestDepth = startingDepth;
    bool found = false;

    const int lightCount = scene->getLightCount();
    for (int i = 0; i < lightCount; ++i) {
        const bool intersectLight = scene->getLight(i)->intersect(ray->getSource(), ray->getDirection(), &closestDepth);

        if (intersectLight) {
            light = scene->getLight(i);
            found = true;
        }
    }

    // Find the closest intersection
    const int objectCount = scene->getSceneObjectCount();
    for (int i = 0; i < objectCount; i++) {
        SceneObject *object = scene->getSceneObject(i);
        SceneGeometry *geometry = object->getGeometry();

        if (!geometry->fastIntersection(ray)) continue;
        if (geometry->findClosestIntersection(ray, &closestIntersection, (math::real)0.0, closestDepth, s /**/ STATISTICS_PARAM_INPUT)) {
            found = true;
            closestIntersection.sceneObject = object;
            light = nullptr;
            closestDepth = closestIntersection.depth;
        }
    }

    if (found) {
        point->m_valid = true;

        if (closestIntersection.sceneObject != nullptr) {
            const math::Vector position = math::add(ray->getSource(), math::mul(ray->getDirection(), math::loadScalar(closestIntersection.depth)));
            closestIntersection.sceneGeometry->fineIntersection(position, point, &closestIntersection);
            *closestObject = closestIntersection.sceneObject;

            refineContact(ray, closestIntersection.depth, point, closestObject, s);
        }
        else if (light != nullptr) {
            point->m_light = light;
            *closestObject = nullptr;
        }
    }
    else {
        point->m_valid = false;
        *closestObject = nullptr;
    }
}

void manta::RayTracer::refineContact(
    const LightRay *ray,
    math::real depth, 
    IntersectionPoint *point,
    SceneObject **closestObject,
    StackAllocator *s) const 
{
    if (math::getScalar(math::dot(point->m_faceNormal, ray->getDirection())) > 0) {
        point->m_faceNormal = math::negate(point->m_faceNormal);
        point->m_vertexNormal = math::negate(point->m_vertexNormal);
        point->m_direction = MediaInterface::Direction::Out;
        const math::Vector temp = point->m_inside;
        point->m_inside = point->m_outside;
        point->m_outside = temp;
    }
    else {
        point->m_direction = MediaInterface::Direction::In;
    }
}

bool manta::RayTracer::occluded(const Scene *scene, const math::Vector &p0, const math::Vector &d, math::real maxDepth STATISTICS_PROTOTYPE) const {
    const int objectCount = scene->getSceneObjectCount();
    for (int i = 0; i < objectCount; i++) {
        SceneObject *object = scene->getSceneObject(i);
        SceneGeometry *geometry = object->getGeometry();

        if (geometry->occluded(p0, d, maxDepth /**/ STATISTICS_PARAM_INPUT)) {
            return true;
        }
    }

    return false;
}

manta::math::Vector manta::RayTracer::traceRay(
    const Scene *scene,
    LightRay *ray,
    int degree,
    IntersectionPointManager *manager,
    Sampler *sampler,
    StackAllocator *s /**/
    PATH_RECORDER_DECL /**/
    STATISTICS_PROTOTYPE) const 
{
    int maxBounces = 4;

    LightRay *currentRay = ray;
    LightRay localRay;
    math::Vector beta = math::constants::One;
    math::Vector L = math::constants::Zero;

    RayFlags flags = RayFlag::None;
    for (int bounces = 0; bounces < maxBounces; bounces++) {
        currentRay->resetCache();

        SceneObject *sceneObject = nullptr;
        Material *material = nullptr;
        IntersectionPoint point;
        point.m_lightRay = currentRay;
        point.m_id = manager->generateId();
        point.m_threadId = manager->getThreadId();
        point.m_manager = manager;

        depthCull(scene, currentRay, &sceneObject, &point, s, math::constants::REAL_MAX /**/ STATISTICS_PARAM_INPUT);

        const bool geometryIntersection = (sceneObject != nullptr);
        const bool lightIntersection = (point.m_light != nullptr);
        if (geometryIntersection) {
            material = (point.m_material == -1) 
                ? sceneObject->getDefaultMaterial()
                : m_materialManager->getMaterial(point.m_material);

            const math::Vector emission = material->getEmission(point);

            L = math::add(
                L,
                math::mul(beta, emission)
            );
        }
        else {
            if (point.m_light != nullptr) {
                L = math::add(
                    L,
                    math::mul(beta, m_backgroundColor)
                );
            }

            if (bounces == 0 || (flags & RayFlag::Delta) > 0 || !m_directLightSampling) {
                if (point.m_light != nullptr) {
                    L = math::add(
                        L,
                        math::mul(beta, point.m_light->L(point, point.m_lightRay->getDirection()))
                    );
                }
            }

            break;
        }

        // Get the BSDF associated with this material
        BSDF *bsdf = material->getBSDF();
        if (bsdf == nullptr) break;
        else point.m_bsdf = bsdf;

        if (m_directLightSampling) {
            L = math::add(
                L,
                math::mul(beta, uniformSampleOneLight(&point, scene, sampler, manager, s)));
        }

        // Generate a new path
        const math::Vector outgoingDir = math::negate(currentRay->getDirection());
        math::Vector incomingDir;

        math::Vector2 s_u = (sampler != nullptr)
            ? sampler->generate2d()
            : math::Vector2(math::uniformRandom(), math::uniformRandom());
        math::real pdf;
        flags = RayFlag::None;
        math::Vector f = bsdf->sampleF(&point, s_u, outgoingDir, &incomingDir, &pdf, &flags, s, true);
        f = math::mask(f, math::constants::MaskOffW);

        if ((flags & RayFlag::Transmission) > 0) {
            maxBounces = std::max(maxBounces, 16);
        }

        if (pdf == (math::real)0.0) break;
        
        beta = math::mul(beta, f);
        beta = math::div(
            beta,
            math::loadScalar(pdf)
        );

        assert(!std::isnan(math::getX(beta)) && !std::isnan(math::getY(beta)) && !std::isnan(math::getZ(beta)));
        
        localRay.setDirection(incomingDir);
        if ((flags & RayFlag::Transmission) > 0) localRay.setSource(point.m_inside);
        else localRay.setSource(point.m_outside);
        localRay.calculateTransformations();
        currentRay = &localRay;

        if (bounces > 3) {
            const math::real q = std::max((math::real)0.05, 1 - math::getScalar(math::maxComponent(beta)));
            const math::real d = (sampler != nullptr)
                ? sampler->generate1d()
                : math::uniformRandom();
            if (d < q) break;
            beta = math::div(beta, math::loadScalar(1 - q));
        }
    }

    return L;
}
