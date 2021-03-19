#include "../include/kd_tree.h"

#include "../include/mesh.h"
#include "../include/standard_allocator.h"
#include "../include/coarse_intersection.h"
#include "../include/runtime_statistics.h"
#include "../include/os_utilities.h"
#include "../include/vector_node_output.h"
#include "../include/session.h"
#include "../include/console.h"

#include <algorithm>
#include <thread>
#include <stdlib.h>
#include <chrono>

manta::KDTree::KDTree() {
    m_nodes = nullptr;
    m_nodeVolumes = nullptr;
    m_nodeCapacity = 0;
    m_nodeCount = 0;

    m_volumeCount = 0;
    m_volumeCapacity = 0;

    m_faceLists = nullptr;
    m_faceCount = 0;

    m_progress = (math::real)0.0;
    m_complete = false;
}

manta::KDTree::~KDTree() {
    assert(m_faceLists == nullptr);
    assert(m_nodeVolumes == nullptr);
    assert(m_nodes == nullptr);
}

void manta::KDTree::_evaluate() {
    Mesh *mesh = getObject<Mesh>(m_meshInput);

    piranha::native_float width;
    piranha::native_int granularity;
    math::Vector center;

    m_widthInput->fullCompute((void *)&width);
    m_granularityInput->fullCompute((void *)&granularity);
    static_cast<VectorNodeOutput *>(m_centerInput)->sample(nullptr, (void *)&center);

    configure((math::real)width, center);

    analyzeWithProgress(mesh, granularity);

    setOutput(this);
}

void manta::KDTree::_destroy() {
    destroy();
}

void manta::KDTree::registerInputs() {
    registerInput(&m_centerInput, "center");
    registerInput(&m_meshInput, "mesh");
    registerInput(&m_widthInput, "width");
    registerInput(&m_granularityInput, "granularity");
}

void manta::KDTree::configure(math::real width, const math::Vector &position) {
    m_width = width;

    m_bounds.maxPoint = math::add(position, math::loadScalar(width));
    m_bounds.minPoint = math::sub(position, math::loadScalar(width));
}

void manta::KDTree::destroy() {
    if (m_faceLists != nullptr) {
        StandardAllocator::Global()->free(m_faceLists, m_faceCount);
        m_faceLists = nullptr;
        m_faceCount = 0;
    }

    if (m_nodes != nullptr) {
        StandardAllocator::Global()->free(m_nodes, m_nodeCapacity);
        m_nodes = nullptr;
        m_nodeCapacity = 0;
        m_nodeCount = 0;
    }

    if (m_nodeVolumes != nullptr) {
        StandardAllocator::Global()->aligned_free(m_nodeVolumes, m_volumeCapacity);
        m_nodeVolumes = nullptr;
        m_volumeCount = 0;
        m_volumeCapacity = 0;
    }
}

bool manta::KDTree::findClosestIntersection(
    const LightRay *ray,
    CoarseIntersection *intersection,
    math::real minDepth,
    math::real maxDepth,
    StackAllocator *s    
    /**/ STATISTICS_PROTOTYPE) const
{
    math::real tmin, tmax;
    if (!m_bounds.rayIntersect(*ray, &tmin, &tmax)) {
        return false;
    }

    constexpr int MAX_DEPTH = 64;
    KDJob jobs[MAX_DEPTH];
    int currentJob = 0;

    bool hit = false;
    math::real closestHit = std::min(tmax, maxDepth);
    const KDTreeNode *node = &m_nodes[0];
    while (node != nullptr) {
        if (closestHit < tmin) break;
        if (!node->isLeaf()) {
            int axis = node->getSplitAxis();
            math::real split = node->getSplit();

            math::real o_axis = math::get(ray->getSource(), axis);
            math::real tPlane = (split - o_axis) * math::get(ray->getInverseDirection(), axis);
            const bool eq = o_axis == split;
            if (eq) {
                // Edge case
                // Explanation: somtimes inverse direction can be inf. This will result in tPlane being NaN.
                tPlane = (math::real)0.0;
            }

            const KDTreeNode *firstChild, *secondChild;
            bool belowFirst = (o_axis < split) || (eq && math::get(ray->getDirection(), axis) <= 0);

            if (belowFirst) {
                firstChild = node + 1;
                secondChild = &m_nodes[node->getAboveChild()];
            }
            else {
                firstChild = &m_nodes[node->getAboveChild()];
                secondChild = node + 1;
            }

            if (tPlane > tmax || tPlane <= 0) {
                node = firstChild;
            }
            else if (tPlane < tmin) {
                node = secondChild;
            }
            else {
                // Add second child to queue
                jobs[currentJob].node = secondChild;
                jobs[currentJob].tmin = tPlane;
                jobs[currentJob].tmax = tmax;
                ++currentJob;

                node = firstChild;
                tmax = tPlane;
            }
        }
        else {
            const int primitiveCount = node->getPrimitiveCount();
            const int objectOffset = node->getObjectOffset();
            int *faceList;
            if (primitiveCount > 0) {
                const KDBoundingVolume &bv = m_nodeVolumes[objectOffset];
                faceList = &m_faceLists[bv.faceListOffset];

#if KD_TREE_WITH_BOUNDING_BOXES
                INCREMENT_COUNTER(RuntimeStatistics::Counter::TotalBvTests);
                math::real t0, t1;
                if (bv.bounds.rayIntersect(*ray, &t0, &t1)) {
#endif
                    INCREMENT_COUNTER(RuntimeStatistics::Counter::TotalBvHits);
                    if (m_mesh->findClosestIntersection(faceList, primitiveCount, ray, intersection, minDepth, closestHit /**/ STATISTICS_PARAM_INPUT)) {
                        hit = true;
                        closestHit = intersection->depth;
                    }
#if KD_TREE_WITH_BOUNDING_BOXES
                }
#endif
            }
            else {
                // Nothing in this node
            }

            if (currentJob > 0) {
                --currentJob;
                node = jobs[currentJob].node;
                tmin = jobs[currentJob].tmin;
                tmax = jobs[currentJob].tmax;
            }
            else break;
        }
    }

    return hit;
}

manta::math::Vector manta::KDTree::getClosestPoint(const CoarseIntersection *hint, const math::Vector &p) const {
    return math::Vector();
}

void manta::KDTree::fineIntersection(const math::Vector &r, IntersectionPoint *p, const CoarseIntersection *hint) const {
    hint->sceneGeometry->fineIntersection(r, p, hint);
}

bool manta::KDTree::fastIntersection(const LightRay *ray) const {
    return true;
}

void manta::KDTree::analyzeWithProgress(Mesh *mesh, int maxSize) {
    auto startTime = std::chrono::system_clock::now();

    showConsoleCursor(false);

    std::thread thread(&KDTree::analyze, this, mesh, maxSize);

    while (!isComplete()) {
        std::stringstream ss;
        ss << "Generating KD tree:..." << m_progress * (math::real)100.0 << "%                    \r";
        Session::get().getConsole()->out(ss.str());
        sleep(20);    
    }

    // Wait for the thread to finish
    thread.join();

    std::stringstream ss;
    ss << "Generating KD tree... " << (math::real)100.0 << "%                    \r" << std::endl;
    Session::get().getConsole()->out(ss.str());

    showConsoleCursor(true);

    auto endTime = std::chrono::system_clock::now();
    std::chrono::duration<double> diff = endTime - startTime;

    ss = std::stringstream();
    ss << "KD tree generation took: " << diff.count() << "s" << std::endl;
    Session::get().getConsole()->out(ss.str());
}

void manta::KDTree::analyze(Mesh *mesh, int maxSize) {
    constexpr int MAX_DEPTH = 64;

    setComplete(false);
    resetProgress();

    m_mesh = mesh;

    std::vector<int> faces;
    int nFaces = mesh->getFaceCount();
    for (int i = 0; i < nFaces; i++) {
        faces.push_back(i);
    }

    KDTreeWorkspace workspace;
    workspace.mesh = mesh;
    workspace.maxPrimitives = maxSize;
    
    for (int i = 0; i < 3; i++) {
        workspace.edges[i] = StandardAllocator::Global()->allocate<KDBoundEdge>(mesh->getFaceCount() * 2);
    }

    // Generate bounds for all faces
    workspace.allFaceBounds = StandardAllocator::Global()->allocate<AABB>(nFaces, 16);
    for (int i = 0; i < nFaces; i++) {
        mesh->calculateFaceAABB(i, &workspace.allFaceBounds[i]);
    }

    AABB topNodeBounds;
    topNodeBounds.minPoint = math::loadScalar(-m_width);
    topNodeBounds.maxPoint = math::loadScalar(m_width);

    int badRefines = 0;
    _analyze(0, &topNodeBounds, faces, badRefines, MAX_DEPTH, &workspace, (math::real)1.0);

    // Copy faces into a new array
    int totalFaces = (int)workspace.faces.size();

    m_faceLists = StandardAllocator::Global()->allocate<int>(totalFaces);
    m_faceCount = totalFaces;
    for (int i = 0; i < totalFaces; i++) {
        m_faceLists[i] = workspace.faces[i];
    }

    // Destroy all allocated memory
    for (int i = 0; i < 3; i++) {
        StandardAllocator::Global()->free(workspace.edges[i], mesh->getFaceCount() * 2);
    }

    StandardAllocator::Global()->aligned_free(workspace.allFaceBounds, nFaces);

    setProgress((math::real)1.0);
    setComplete(true);
}

void manta::KDTree::_analyze(int currentNode, AABB *nodeBounds, const std::vector<int> &faces, 
                            int badRefines, int depth, KDTreeWorkspace *workspace, math::real effort) {
    constexpr math::real intersectionCost = 80;
    constexpr math::real traversalCost = 1;
    constexpr math::real emptyBonus = 0.5;

    createNode();

    // Debug only
    //m_nodeBounds.push_back(*nodeBounds);

    int primitiveCount = (int)faces.size();
    if (primitiveCount <= workspace->maxPrimitives || depth == 0) {
        initLeaf(currentNode, faces, *nodeBounds, workspace);

        incrementProgress(effort);
        return;
    }

    int bestAxis = -1, bestOffset = -1;
    math::real bestCost = math::constants::REAL_MAX;
    math::real oldCost = intersectionCost * primitiveCount;
    math::real totalSA = nodeBounds->surfaceArea();
    math::real invTotalSA = 1 / totalSA;
    math::Vector d = math::sub(nodeBounds->maxPoint, nodeBounds->minPoint);

    int retries = 0;
    int axis = math::maxDimension3(d);

    retry_split:
    // Initialize edges
    for (int i = 0; i < primitiveCount; i++) {
        int primitiveIndex = faces[i];
        const AABB &bounds = workspace->allFaceBounds[primitiveIndex];
        workspace->edges[axis][2 * i] = KDBoundEdge(math::get(bounds.minPoint, axis), primitiveIndex, true);    // Start
        workspace->edges[axis][2 * i + 1] = KDBoundEdge(math::get(bounds.maxPoint, axis), primitiveIndex, false);    // End
    }

    std::sort(&workspace->edges[axis][0], &workspace->edges[axis][2 * primitiveCount],
        [](const KDBoundEdge &e0, const KDBoundEdge &e1) -> bool {
        if (e0.t == e1.t) return (int)e0.edgeType < (int)e1.edgeType;
        else return e0.t < e1.t;
    });

    int nBelow = 0, nAbove = primitiveCount;
    for (int i = 0; i < 2 * primitiveCount; i++) {
        if (workspace->edges[axis][i].edgeType == KDBoundEdge::EdgeType::End) --nAbove;
        math::real edgeT = workspace->edges[axis][i].t;
        if (edgeT > math::get(nodeBounds->minPoint, axis) && edgeT < math::get(nodeBounds->maxPoint, axis)) {
            // Compute split cost
            int otherAxis0 = (axis + 1) % 3, otherAxis1 = (axis + 2) % 3;
            math::real belowSA = 2 * (math::get(d, otherAxis0) * math::get(d, otherAxis1) +
                (edgeT - math::get(nodeBounds->minPoint, axis)) *
                (math::get(d, otherAxis0) + math::get(d, otherAxis1)));
            math::real aboveSA = 2 * (math::get(d, otherAxis0) * math::get(d, otherAxis1) +
                (math::get(nodeBounds->maxPoint, axis) - edgeT) *
                (math::get(d, otherAxis0) + math::get(d, otherAxis1)));

            math::real pBelow = belowSA * invTotalSA;
            math::real pAbove = aboveSA * invTotalSA;
            math::real eb = (nAbove == 0 || nBelow == 0) ? emptyBonus : 0;
            math::real cost = traversalCost + intersectionCost * (1 - eb) * (pBelow * nBelow + pAbove * nAbove);

            if (cost < bestCost) {
                bestCost = cost;
                bestAxis = axis;
                bestOffset = i;
            }
        }
        if (workspace->edges[axis][i].edgeType == KDBoundEdge::EdgeType::Start) ++nBelow;
    }

    if (bestAxis == -1 && retries < 2) {
        ++retries;
        axis = (axis + 1) % 3;
        goto retry_split;
    }

    if (bestCost > oldCost) ++badRefines;
    if (((bestCost > 4 * oldCost && primitiveCount < workspace->maxPrimitives) || bestAxis == -1 || badRefines == 3)) {
        initLeaf(currentNode, faces, *nodeBounds, workspace);
        incrementProgress(effort);
        return;
    }

    // Classify primitives
    std::vector<int> primitives0;
    std::vector<int> primitives1;
    for (int i = 0; i < bestOffset; ++i) {
        if (workspace->edges[axis][i].edgeType == KDBoundEdge::EdgeType::Start) {
            primitives0.push_back(workspace->edges[axis][i].primitiveIndex);
        }
    }
    for (int i = bestOffset + 1; i < 2 * primitiveCount; ++i) {
        if (workspace->edges[axis][i].edgeType == KDBoundEdge::EdgeType::End) {
            primitives1.push_back(workspace->edges[axis][i].primitiveIndex);
        }
    }

    // Split
    math::real split = workspace->edges[axis][bestOffset].t;
    AABB bounds0 = *nodeBounds, bounds1 = *nodeBounds;
    math::set(bounds0.maxPoint, bestAxis, split);
    math::set(bounds1.minPoint, bestAxis, split);

    int totalPrimitives = (int)primitives0.size() + (int)primitives1.size();
    math::real effort0, effort1;
    effort0 = (math::real)primitives0.size() / totalPrimitives;
    effort1 = (math::real)1.0 - effort0;

    _analyze(currentNode + 1, &bounds0, primitives0, badRefines, depth - 1, workspace, effort0 * effort);
    int aboveChild = m_nodeCount;
    m_nodes[currentNode].initInterior(bestAxis, aboveChild, split);
    _analyze(aboveChild, &bounds1, primitives1, badRefines, depth - 1, workspace, effort1 * effort);
}

int manta::KDTree::createNode() {
    if (m_nodeCount == m_nodeCapacity) {
        int newSize = 1;
        if (m_nodeCapacity > 0) newSize = m_nodeCapacity * 2;

        KDTreeNode *newNodes = StandardAllocator::Global()->allocate<KDTreeNode>(newSize);
        if (m_nodeCount > 0) {
            // Copy old nodes
            memcpy((void *)newNodes, (void *)m_nodes, sizeof(KDTreeNode) * m_nodeCount);
        }

        if (m_nodeCapacity > 0) {
            StandardAllocator::Global()->free(m_nodes, m_nodeCapacity);
        }

        m_nodes = newNodes;
        m_nodeCapacity = newSize;
    }

    int newNode = ++m_nodeCount;
    return newNode;
}

int manta::KDTree::createNodeVolume() {
    if (m_volumeCount == m_volumeCapacity) {
        int newSize = 1;
        if (m_volumeCapacity > 0) newSize = m_volumeCapacity * 2;

        KDBoundingVolume *newVolumes = StandardAllocator::Global()->allocate<KDBoundingVolume>(newSize, 16);
        if (m_nodeCount > 0) {
            // Copy old nodes
            memcpy((void *)newVolumes, (void *)m_nodeVolumes, sizeof(KDBoundingVolume) * m_volumeCount);
        }

        if (m_volumeCapacity > 0) {
            StandardAllocator::Global()->aligned_free(m_nodeVolumes, m_volumeCapacity);
        }

        m_nodeVolumes = newVolumes;
        m_volumeCapacity = newSize;
    }

    return m_volumeCount++;
}

void manta::KDTree::initLeaf(int node, const std::vector<int> &faces, const AABB &bounds, KDTreeWorkspace *workspace) {
    int primitiveCount = (int)faces.size();
    int filteredPrimitiveCount = 0;
    int newNodeVolume = createNodeVolume();

    constexpr bool ENABLE_FILTERING = true;

    bool *filtered = (primitiveCount > 0)
        ? new bool[primitiveCount]
        : nullptr;

    // Filter faces
    if (primitiveCount > 0) {
        for (int i = 0; i < primitiveCount; i++) {
            if (ENABLE_FILTERING && !m_mesh->checkFaceAABB(faces[i], bounds)) {
                filtered[i] = true;
            }
            else {
                filtered[i] = false;
                filteredPrimitiveCount++;
            }
        }
    }

    m_nodes[node].initLeaf(filteredPrimitiveCount, newNodeVolume);

    if (filteredPrimitiveCount > 0) {
        // Initialize the new volume
        KDBoundingVolume &volume = m_nodeVolumes[newNodeVolume];
        volume.faceListOffset = (int)workspace->faces.size();

        // Add all primitives to the list
        for (int i = 0; i < primitiveCount; i++) {
            if (!filtered[i]) {
                workspace->faces.push_back(faces[i]);
            }
        }

#if KD_TREE_WITH_BOUNDING_BOXES
        // Merge all bounds
        volume.bounds = workspace->allFaceBounds[faces[0]];
        for (int i = 1; i < primitiveCount; i++) {
            if (!filtered[i]) {
                volume.bounds.merge(workspace->allFaceBounds[faces[i]]);
            }
        }
#endif /* KD_TREE_WITH_BOUNDING_BOXES */
    }

    if (filtered != nullptr) delete[] filtered;
}

void manta::KDTree::writeToObjFile(const char *fname) const {
    if (m_nodeBounds.empty()) return; // Check if debug info is present

    std::ofstream f(fname);

    int width = 0;
    int nodes = m_nodeCount;
    while (nodes > 0) {
        width++; 
        nodes /= 10;
    }
    
    int nodeCount = m_nodeCount;
    for (int i = 0; i < nodeCount; i++) {
        const AABB &bounds = m_nodeBounds[i];
        KDTreeNode &node = m_nodes[i];

        int faceCount = 0;
        if (node.getSplitAxis() == 0x3) faceCount = node.getPrimitiveCount();

        f << "o " << "LEAF_";
        f.fill('0');
        f.width(width);
        f << std::right << i;
        f.width(0);
        f << "_N" << faceCount << std::endl;

        math::Vector minPoint = bounds.minPoint;
        math::Vector maxPoint = bounds.maxPoint;

        int vertexOffset = i * 8 + 1;
        f << "v " << math::getX(minPoint) << " " << math::getY(minPoint) << " " << math::getZ(minPoint) << std::endl;
        f << "v " << math::getX(maxPoint) << " " << math::getY(minPoint) << " " << math::getZ(minPoint) << std::endl;
        f << "v " << math::getX(minPoint) << " " << math::getY(minPoint) << " " << math::getZ(maxPoint) << std::endl;
        f << "v " << math::getX(maxPoint) << " " << math::getY(minPoint) << " " << math::getZ(maxPoint) << std::endl;
        f << "v " << math::getX(minPoint) << " " << math::getY(maxPoint) << " " << math::getZ(minPoint) << std::endl;
        f << "v " << math::getX(maxPoint) << " " << math::getY(maxPoint) << " " << math::getZ(minPoint) << std::endl;
        f << "v " << math::getX(minPoint) << " " << math::getY(maxPoint) << " " << math::getZ(maxPoint) << std::endl;
        f << "v " << math::getX(maxPoint) << " " << math::getY(maxPoint) << " " << math::getZ(maxPoint) << std::endl;

        f << "f " << vertexOffset << " " << vertexOffset + 1 << " " << vertexOffset + 3 << " " << vertexOffset + 2 << std::endl;
        f << "f " << vertexOffset << " " << vertexOffset + 4 << " " << vertexOffset + 6 << " " << vertexOffset + 2 << std::endl;
        f << "f " << vertexOffset << " " << vertexOffset + 4 << " " << vertexOffset + 5 << " " << vertexOffset + 1 << std::endl;
        f << "f " << vertexOffset + 7 << " " << vertexOffset + 5 << " " << vertexOffset + 1 << " " << vertexOffset + 3 << std::endl;
        f << "f " << vertexOffset + 7 << " " << vertexOffset + 6 << " " << vertexOffset + 2 << " " << vertexOffset + 3 << std::endl;
        f << "f " << vertexOffset + 7 << " " << vertexOffset + 6 << " " << vertexOffset + 4 << " " << vertexOffset + 5 << std::endl;
    }

    f.close();
}
