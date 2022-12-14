#include <pch.h>

#include "../include/obj_file_loader.h"
#include "../include/mesh.h"
#include "../include/light_ray.h"
#include "../include/coarse_intersection.h"
#include "../include/intersection_point.h"

#include <chrono>
#include <fstream>

using namespace manta;

TEST(MeshIntersectionTests, MeshIntersectionSanityCheck) {
    ObjFileLoader singleTriangleObj;
    bool result = singleTriangleObj.loadObjFile("../../../test/geometry/single_triangle.obj");

    Mesh mesh;
    mesh.loadObjFileData(&singleTriangleObj);
    mesh.setFastIntersectEnabled(false);

    LightRay ray;
    ray.setDirection(math::loadVector(0.0, 0.0, -1.0));
    ray.setSource(math::loadVector(0.5, 0.0, 1.0));
    ray.calculateTransformations();

    CoarseIntersection intersection;
    bool found = mesh.findClosestIntersection(&ray, &intersection, 0.0, math::constants::REAL_MAX, nullptr /**/ STATISTICS_NULL_INPUT);

    EXPECT_EQ(found, true);
    EXPECT_FLOAT_EQ(intersection.depth, 1.0);

    singleTriangleObj.destroy();
    mesh.destroy();
}

TEST(MeshIntersectionTests, MeshIntersectionSanityCheckOppositeSide) {
    ObjFileLoader singleTriangleObj;
    bool result = singleTriangleObj.loadObjFile("../../../test/geometry/single_triangle.obj");

    Mesh mesh;
    mesh.loadObjFileData(&singleTriangleObj);
    mesh.setFastIntersectEnabled(false);

    LightRay ray;
    ray.setDirection(math::loadVector(0.0, 0.0, 1.0));
    ray.setSource(math::loadVector(0.5, 0.0, -1.0));
    ray.calculateTransformations();

    CoarseIntersection intersection;
    bool found = mesh.findClosestIntersection(&ray, &intersection, 0.0, math::constants::REAL_MAX, nullptr /**/ STATISTICS_NULL_INPUT);

    EXPECT_EQ(found, true);
    EXPECT_FLOAT_EQ(intersection.depth, 1.0);

    singleTriangleObj.destroy();
    mesh.destroy();
}

TEST(MeshIntersectionTests, MeshIntersectionSanityCheckNoHit) {
    ObjFileLoader singleTriangleObj;
    bool result = singleTriangleObj.loadObjFile("../../../test/geometry/single_triangle.obj");

    Mesh mesh;
    mesh.loadObjFileData(&singleTriangleObj);
    mesh.setFastIntersectEnabled(false);

    LightRay ray;
    ray.setDirection(math::loadVector(0.0, 0.0, 1.0));
    ray.setSource(math::loadVector(5.5, 0.0, -1.0));
    ray.calculateTransformations();

    CoarseIntersection intersection;
    bool found = mesh.findClosestIntersection(&ray, &intersection, 0.0, math::constants::REAL_MAX, nullptr /**/ STATISTICS_NULL_INPUT);

    EXPECT_EQ(found, false);

    singleTriangleObj.destroy();
    mesh.destroy();
}

TEST(MeshIntersectionTests, MeshIntersectionCheckVertex) {
    ObjFileLoader singleTriangleObj;
    bool result = singleTriangleObj.loadObjFile("../../../test/geometry/single_triangle.obj");

    Mesh mesh;
    mesh.loadObjFileData(&singleTriangleObj);
    mesh.setFastIntersectEnabled(false);

    LightRay ray;
    ray.setDirection(math::loadVector(0.0, 0.0, -1.0));
    ray.setSource(math::loadVector(1.0, 1.0, 1.0));
    ray.calculateTransformations();

    CoarseIntersection intersection;
    bool found = mesh.findClosestIntersection(&ray, &intersection, 0.0, math::constants::REAL_MAX, nullptr /**/ STATISTICS_NULL_INPUT);

    EXPECT_EQ(found, true);
    EXPECT_FLOAT_EQ(intersection.depth, 1.0);

    singleTriangleObj.destroy();
    mesh.destroy();
}

TEST(MeshIntersectionTests, MeshIntersectionSideIntersection) {
    ObjFileLoader singleTriangleObj;
    bool result = singleTriangleObj.loadObjFile("../../../test/geometry/single_triangle.obj");

    Mesh mesh;
    mesh.loadObjFileData(&singleTriangleObj);
    mesh.setFastIntersectEnabled(false);

    LightRay ray;
    ray.setDirection(math::loadVector(1.0, 0.0, 0.0));
    ray.setSource(math::loadVector(1.0, 0.0, 0.0));
    ray.calculateTransformations();

    CoarseIntersection intersection;
    bool found = mesh.findClosestIntersection(&ray, &intersection, 0.0, math::constants::REAL_MAX, nullptr /**/ STATISTICS_NULL_INPUT);

    EXPECT_EQ(found, false);

    singleTriangleObj.destroy();
    mesh.destroy();
}

TEST(MeshIntersectionTests, PlaneWithQuads) {
    ObjFileLoader planeObj;
    bool result = planeObj.loadObjFile("../../../test/geometry/plane.obj");

    Mesh mesh;
    mesh.loadObjFileData(&planeObj);
    mesh.findQuads();
    mesh.setFastIntersectEnabled(false);

    LightRay ray;
    ray.setDirection(math::loadVector(0.0f, -1.0f, 0.0f));
    ray.setSource(math::loadVector(0.0f, 1.0f, 0.0f));
    ray.calculateTransformations();

    LightRay ray1;
    ray1.setDirection(math::loadVector(0.0f, -1.0f, 0.0f));
    ray1.setSource(math::loadVector(-0.1f, 1.0f, 0.1f));
    ray1.calculateTransformations();

    LightRay ray2;
    ray2.setDirection(math::loadVector(0.0f, -1.0f, 0.0f));
    ray2.setSource(math::loadVector(0.1f, 1.0f, -0.1f));
    ray2.calculateTransformations();

    LightRay ray1u;
    ray1u.setDirection(math::loadVector(0.0f, 1.0f, 0.0f));
    ray1u.setSource(math::loadVector(-0.1f, -1.0f, 0.1f));
    ray1u.calculateTransformations();

    LightRay ray2u;
    ray2u.setDirection(math::loadVector(0.0f, 1.0f, 0.0f));
    ray2u.setSource(math::loadVector(0.1f, -1.0f, -0.1f));
    ray2u.calculateTransformations();

    LightRay rayOut;
    rayOut.setDirection(math::loadVector(0.0f, 1.0f, 0.0f));
    rayOut.setSource(math::loadVector(2.f, -1.0f, 2.f));
    rayOut.calculateTransformations();

    CoarseCollisionOutput intersection;
    bool found = mesh.detectQuadIntersection(0, 0.0f, math::constants::REAL_MAX, &ray, &intersection);
    EXPECT_EQ(found, true);

    found = mesh.detectQuadIntersection(0, 0.0f, math::constants::REAL_MAX, &ray1, &intersection);
    EXPECT_EQ(found, true);
    EXPECT_EQ(intersection.subdivisionHint, 0);

    found = mesh.detectQuadIntersection(0, 0.0f, math::constants::REAL_MAX, &ray2, &intersection);
    EXPECT_EQ(found, true);
    EXPECT_EQ(intersection.subdivisionHint, 1);

    found = mesh.detectQuadIntersection(0, 0.0f, math::constants::REAL_MAX, &ray1u, &intersection);
    EXPECT_EQ(found, true);
    EXPECT_EQ(intersection.subdivisionHint, 0);

    found = mesh.detectQuadIntersection(0, 0.0f, math::constants::REAL_MAX, &ray2u, &intersection);
    EXPECT_EQ(found, true);
    EXPECT_EQ(intersection.subdivisionHint, 1);

    found = mesh.detectQuadIntersection(0, 0.0f, math::constants::REAL_MAX, &rayOut, &intersection);
    EXPECT_EQ(found, false);

    planeObj.destroy();
    mesh.destroy();
}

TEST(MeshIntersectionTests, MeshIntersectionQuadsSanityCheck) {
    ObjFileLoader singleTriangleObj;
    bool result = singleTriangleObj.loadObjFile("../../../test/geometry/plane.obj");

    Mesh mesh;
    mesh.loadObjFileData(&singleTriangleObj);
    mesh.setFastIntersectEnabled(false);
    mesh.findQuads();

    LightRay ray1;
    ray1.setDirection(math::loadVector(0.0f, -1.0f, 0.0f));
    ray1.setSource(math::loadVector(0.1f, 1.0f, -0.1f));
    ray1.calculateTransformations();

    LightRay ray2;
    ray2.setDirection(math::loadVector(0.0f, -1.0f, 0.0f));
    ray2.setSource(math::loadVector(-0.1f, 1.0f, 0.1f));
    ray2.calculateTransformations();

    LightRay ray3;
    ray3.setDirection(math::loadVector(0.0f, 1.0f, 0.0f));
    ray3.setSource(math::loadVector(0.1f, -1.0f, -0.1f));
    ray3.calculateTransformations();

    LightRay ray4;
    ray4.setDirection(math::loadVector(0.0f, 1.0f, 0.0f));
    ray4.setSource(math::loadVector(-0.1f, -1.0f, 0.1f));
    ray4.calculateTransformations();

    CoarseIntersection intersection;
    bool found = mesh.findClosestIntersection(&ray1, &intersection, 0.0f, math::constants::REAL_MAX, nullptr /**/ STATISTICS_NULL_INPUT);

    EXPECT_EQ(found, true);
    EXPECT_EQ(intersection.faceHint, 0);
    EXPECT_EQ(intersection.subdivisionHint, 1);
    EXPECT_FLOAT_EQ(intersection.depth, 1.0f);

    found = mesh.findClosestIntersection(&ray2, &intersection, 0.0f, math::constants::REAL_MAX, nullptr /**/ STATISTICS_NULL_INPUT);

    EXPECT_EQ(found, true);
    EXPECT_EQ(intersection.faceHint, 0);
    EXPECT_EQ(intersection.subdivisionHint, 0);
    EXPECT_FLOAT_EQ(intersection.depth, 1.0f);

    found = mesh.findClosestIntersection(&ray3, &intersection, 0.0f, math::constants::REAL_MAX, nullptr /**/ STATISTICS_NULL_INPUT);

    EXPECT_EQ(found, true);
    EXPECT_EQ(intersection.faceHint, 0);
    EXPECT_EQ(intersection.subdivisionHint, 1);
    EXPECT_FLOAT_EQ(intersection.depth, 1.0);

    found = mesh.findClosestIntersection(&ray4, &intersection, 0.0f, math::constants::REAL_MAX, nullptr /**/ STATISTICS_NULL_INPUT);

    EXPECT_EQ(found, true);
    EXPECT_EQ(intersection.faceHint, 0);
    EXPECT_EQ(intersection.subdivisionHint, 0);
    EXPECT_FLOAT_EQ(intersection.depth, 1.0);

    IntersectionPoint p;
    mesh.fineIntersection(ray1.getDirection(), &p, &intersection);

    singleTriangleObj.destroy();
    mesh.destroy();
}

TEST(MeshIntersectionTests, MeshIntersectionQuadsObliqueCheck) {
    ObjFileLoader singleTriangleObj;
    bool result = singleTriangleObj.loadObjFile("../../../test/geometry/plane.obj");

    Mesh mesh;
    mesh.loadObjFileData(&singleTriangleObj);
    mesh.setFastIntersectEnabled(false);
    mesh.findQuads();

    LightRay ray1;
    ray1.setDirection(math::loadVector(0.0f, 1.0f, 0.0f));
    ray1.setSource(math::loadVector(-0.1f, -1.0f, 0.1f));
    ray1.calculateTransformations();

    math::Vector obliqueTarget = math::loadVector(-0.1f, 0.0f, 0.1f);
    math::Vector obliqueSource = math::loadVector(5.f, -5.f, 0.f);
    math::Vector dir = math::sub(obliqueTarget, obliqueSource);
    dir = math::normalize(dir);

    LightRay ray2;
    ray2.setDirection(dir);
    ray2.setSource(obliqueSource);
    ray2.calculateTransformations();

    CoarseIntersection intersection;
    bool found = mesh.findClosestIntersection(&ray1, &intersection, 0.0, math::constants::REAL_MAX, nullptr /**/ STATISTICS_NULL_INPUT);

    EXPECT_EQ(found, true);
    EXPECT_EQ(intersection.faceHint, 0);
    EXPECT_EQ(intersection.subdivisionHint, 0);
    EXPECT_FLOAT_EQ(intersection.depth, 1.0f);

    found = mesh.findClosestIntersection(&ray2, &intersection, 0.0, math::constants::REAL_MAX, nullptr /**/ STATISTICS_NULL_INPUT);

    EXPECT_EQ(found, true);
    EXPECT_EQ(intersection.faceHint, 0);
    EXPECT_EQ(intersection.subdivisionHint, 0);

    IntersectionPoint p;
    mesh.fineIntersection(ray1.getDirection(), &p, &intersection);

    singleTriangleObj.destroy();
    mesh.destroy();
}
