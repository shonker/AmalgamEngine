#include "catch2/catch_all.hpp"
#include "Position.h"
#include "BoundingBox.h"
#include "Log.h"

using namespace AM;

TEST_CASE("TestBoundingBox")
{
    SECTION("Get center position")
    {
        BoundingBox box{300, 310, 300, 310, 0, 10};
        Position center{box.getCenterPosition()};

        REQUIRE(center.x == 305);
        REQUIRE(center.y == 305);
        REQUIRE(center.z == 5);
    }

    SECTION("Intersects cylinder")
    {
        // Using the origin.
        Position position{0, 0, 0};
        unsigned int radius{256};

        // Fully inside the cylinder.
        BoundingBox box1{1, 6, 3, 8, 0, 1};
        REQUIRE(box1.intersects(position, radius));

        // Corner inside the cylinder, center outside.
        BoundingBox box2{0, 10, 255, 265, 0, 1};
        REQUIRE(box2.intersects(position, radius));

        // Center inside the cylinder, corner outside.
        BoundingBox box3{0, 10, 250, 260, 0, 1};
        REQUIRE(box3.intersects(position, radius));

        // Edge shared with cylinder.
        BoundingBox box4{256, 266, 246, 266, 0, 1};
        REQUIRE(!(box4.intersects(position, radius)));

        // Fully outside the cylinder.
        BoundingBox box5{300, 310, 300, 310, 0, 1};
        REQUIRE(!(box5.intersects(position, radius)));
    }
}