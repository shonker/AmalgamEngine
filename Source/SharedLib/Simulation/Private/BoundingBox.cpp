#include "BoundingBox.h"
#include "Position.h"
#include "Cylinder.h"
#include "Ray.h"
#include "TileExtent.h"
#include <cmath>

namespace AM
{

bool BoundingBox::operator==(const BoundingBox& other)
{
    return (minX == other.minX) && (maxX == other.maxX) && (minY == other.minY)
           && (maxY == other.maxY) && (minZ == other.minZ)
           && (maxZ == other.maxZ);
}

float BoundingBox::getXLength() const
{
    return (maxX - minX);
}

float BoundingBox::getYLength() const
{
    return (maxY - minY);
}

float BoundingBox::getZLength() const
{
    return (maxZ - minZ);
}

Position BoundingBox::getMinPosition() const
{
    return {minX, minY, minZ};
}

Position BoundingBox::getMaxPosition() const
{
    return {maxX, maxY, maxZ};
}

Position BoundingBox::get3dCenter() const
{
    Position centerPosition{};
    centerPosition.x = minX + ((maxX - minX) / 2);
    centerPosition.y = minY + ((maxY - minY) / 2);
    centerPosition.z = minZ + ((maxZ - minZ) / 2);

    return centerPosition;
}

bool BoundingBox::isEmpty() const
{
    return ((minX == maxX) || (minY == maxY) || (minZ == maxZ));
}

bool BoundingBox::intersects(const BoundingBox& other) const
{
    return ((minX < other.maxX) && (maxX > other.minX) && (minY < other.maxY)
            && (maxY > other.minY) && (minZ < other.maxZ)
            && (maxZ > other.minZ));
}

bool BoundingBox::intersects(const Cylinder& cylinder) const
{
    // Reference: https://stackoverflow.com/a/402010/4258629

    // TODO: Consider Z
    Position boxCenter{get3dCenter()};
    float xLength{getXLength()};
    float yLength{getYLength()};

    // Get the X and Y distances between the centers.
    float circleDistanceX{std::abs(cylinder.center.x - boxCenter.x)};
    float circleDistanceY{std::abs(cylinder.center.y - boxCenter.y)};

    // If the circle is far enough away that no intersection is possible,
    // return false.
    if (circleDistanceX > ((xLength / 2) + cylinder.radius)) {
        return false;
    }
    if (circleDistanceY > ((yLength / 2) + cylinder.radius)) {
        return false;
    }

    // If the circle is close enough that an intersection is guaranteed,
    // return true.
    if (circleDistanceX <= (xLength / 2)) {
        return true;
    }
    if (circleDistanceY <= (yLength / 2)) {
        return true;
    }

    // Calculate the distance from the center of the circle to the corner
    // of the box.
    float xDif{circleDistanceX - (xLength / 2)};
    float yDif{circleDistanceY - (yLength / 2)};
    float cornerDistanceSquared{(xDif * xDif) + (yDif * yDif)};

    // If the distance is less than the radius, return true.
    return (cornerDistanceSquared <= (cylinder.radius * cylinder.radius));
}

bool BoundingBox::intersects(const Ray& ray) const
{
    auto [tMin, tMax] = getIntersections(ray);

    // If tMax is negative, the ray would have to go in the negative direction
    // to intersect the rect.
    // If tMin > tMax, no intersection.
    if ((tMax < 0) || (tMin > tMax)) {
        return false;
    }

    return true;
}

bool BoundingBox::intersects(const TileExtent& tileExtent) const
{
    static constexpr int TILE_WORLD_WIDTH{
        static_cast<int>(SharedConfig::TILE_WORLD_WIDTH)};
    static constexpr int TILE_WORLD_HEIGHT{
        static_cast<int>(SharedConfig::TILE_WORLD_HEIGHT)};

    float tileMinX{static_cast<float>(tileExtent.x * TILE_WORLD_WIDTH)};
    float tileMaxX{static_cast<float>((tileExtent.x + tileExtent.xLength)
                                      * TILE_WORLD_WIDTH)};
    float tileMinY{static_cast<float>(tileExtent.y) * TILE_WORLD_WIDTH};
    float tileMaxY{static_cast<float>((tileExtent.y + tileExtent.yLength)
                                      * TILE_WORLD_WIDTH)};
    float tileMinZ{static_cast<float>(tileExtent.z) * TILE_WORLD_HEIGHT};
    float tileMaxZ{static_cast<float>((tileExtent.z + tileExtent.zLength)
                                      * TILE_WORLD_HEIGHT)};

    return ((maxX >= tileMinX) && (tileMaxX >= minX) && (maxY >= tileMinY)
            && (tileMaxY >= minY) && (maxZ >= tileMinZ) && (tileMaxZ >= minZ));
}

float BoundingBox::getMinIntersection(const Ray& ray) const
{
    auto [tMin, tMax] = getIntersections(ray);

    // If tMax is negative, the ray would have to go in the negative direction
    // to intersect the rect.
    // If tMin > tMax, no intersection.
    if ((tMax < 0) || (tMin > tMax)) {
        return -1;
    }

    // If tMin doesn't intersect in the forward direction, return tMax.
    if (tMin < 0) {
        return tMax;
    }

    return tMin;
}

float BoundingBox::getMaxIntersection(const Ray& ray) const
{
    auto [tMin, tMax] = getIntersections(ray);

    // If tMax is negative, the ray would have to go in the negative direction
    // to intersect the rect.
    // If tMin > tMax, no intersection.
    if ((tMax < 0) || (tMin > tMax)) {
        return -1;
    }

    return tMax;
}

TileExtent BoundingBox::asTileExtent() const
{
    static constexpr float TILE_WORLD_WIDTH{
        static_cast<float>(SharedConfig::TILE_WORLD_WIDTH)};
    static constexpr float TILE_WORLD_HEIGHT{
        static_cast<float>(SharedConfig::TILE_WORLD_HEIGHT)};

    TileExtent tileExtent{};
    tileExtent.x = static_cast<int>(std::floor(minX / TILE_WORLD_WIDTH));
    tileExtent.y = static_cast<int>(std::floor(minY / TILE_WORLD_WIDTH));
    tileExtent.z = static_cast<int>(std::floor(minZ / TILE_WORLD_HEIGHT));
    tileExtent.xLength
        = (static_cast<int>(std::ceil(maxX / TILE_WORLD_WIDTH)) - tileExtent.x);
    tileExtent.yLength
        = (static_cast<int>(std::ceil(maxY / TILE_WORLD_WIDTH)) - tileExtent.y);
    tileExtent.zLength = (static_cast<int>(std::ceil(maxZ / TILE_WORLD_HEIGHT))
                          - tileExtent.z);

    return tileExtent;
}

std::array<float, 2> BoundingBox::getIntersections(const Ray& ray) const
{
    // Find the constant t where intersection occurs for each direction.
    float tX1{(minX - ray.origin.x) / ray.directionX};
    float tX2{(maxX - ray.origin.x) / ray.directionX};
    float tY1{(minY - ray.origin.y) / ray.directionY};
    float tY2{(maxY - ray.origin.y) / ray.directionY};
    float tZ1{(minZ - ray.origin.z) / ray.directionZ};
    float tZ2{(maxZ - ray.origin.z) / ray.directionZ};

    // Find the min t in each direction, then find the max of those.
    // This gives us the t where the ray first intersects the rect.
    float tMin{std::max(std::max(std::min(tX1, tX2), std::min(tY1, tY2)),
                        std::min(tZ1, tZ2))};

    // Find the max t in each direction, then find the min of those.
    // This gives us the t where the ray last intersects the rect.
    float tMax{std::min(std::min(std::max(tX1, tX2), std::max(tY1, tY2)),
                        std::max(tZ1, tZ2))};

    return {tMin, tMax};
}

} // End namespace AM
