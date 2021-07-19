#include "BoundingBoxGizmo.h"
#include "MainScreen.h"
#include "SpriteDataModel.h"
#include "TransformationHelpers.h"
#include "SharedConfig.h"
#include "Ignore.h"
#include "AUI/Core.h"
#include "AUI/ScalingHelpers.h"

#include <SDL2/SDL2_gfxPrimitives.h>

namespace AM
{
namespace SpriteEditor
{

BoundingBoxGizmo::BoundingBoxGizmo(MainScreen& inScreen)
: AUI::Component(inScreen, "", {0, 0, 1920, 1080})
, mainScreen{inScreen}
, activeSprite{nullptr}
, scaledRectSize{AUI::ScalingHelpers::logicalToActual(LOGICAL_RECT_SIZE)}
, scaledLineWidth{AUI::ScalingHelpers::logicalToActual(LOGICAL_LINE_WIDTH)}
, positionControlExtent{0, 0, scaledRectSize, scaledRectSize}
, lastRenderedPosExtent{}
, xControlExtent{0, 0, scaledRectSize, scaledRectSize}
, lastRenderedXExtent{}
, yControlExtent{0, 0, scaledRectSize, scaledRectSize}
, lastRenderedYExtent{}
, zControlExtent{0, 0, scaledRectSize, scaledRectSize}
, lastRenderedZExtent{}
, xMinPoint{}
, xMaxPoint{}
, yMinPoint{}
, yMaxPoint{}
, zMinPoint{}
, zMaxPoint{}
, planeXCoords{}
, planeYCoords{}
, currentHeldControl{Control::None}
{
    // Register for the events that we want to listen for.
    registerListener(AUI::InternalEvent::MouseButtonDown);
    registerListener(AUI::InternalEvent::MouseMove);
    registerListener(AUI::InternalEvent::MouseButtonUp);
}

void BoundingBoxGizmo::loadActiveSprite(SpriteStaticData* inActiveSprite)
{
    // Set the new active sprite.
    activeSprite = inActiveSprite;

    // Refresh the UI with the newly set sprite's data.
    refresh();
}

void BoundingBoxGizmo::refresh()
{
    if (activeSprite == nullptr) {
        LOG_ERROR("Tried to refresh with nullptr data.");
    }

    // Calculate where the sprite's model bounds are on the screen.
    // Note: The ordering of the points in this vector is listed in the comment
    //       for calcOffsetScreenPoints().
    std::vector<SDL_Point> boundsScreenPoints;
    calcOffsetScreenPoints(boundsScreenPoints);

    // Move the controls to the correct positions.
    moveControls(boundsScreenPoints);

    // Move the lines to the correct positions.
    moveLines(boundsScreenPoints);

    // Move the planes to the correct positions.
    movePlanes(boundsScreenPoints);
}

void BoundingBoxGizmo::render(const SDL_Point& parentOffset)
{
    // Keep our extent up to date.
    refreshScaling();

    // Save the extent that we're going to render at.
    lastRenderedExtent = scaledExtent;
    lastRenderedExtent.x += parentOffset.x;
    lastRenderedExtent.y += parentOffset.y;

    // If the component isn't visible, return without rendering.
    if (!isVisible) {
        return;
    }

    // Children should render at the parent's offset + this component's offset.
    SDL_Point childOffset{parentOffset};
    childOffset.x += scaledExtent.x;
    childOffset.y += scaledExtent.y;

    // Render the planes.
    renderPlanes(childOffset);

    // Render the lines.
    renderLines(childOffset);

    // Render the control rectangles.
    renderControls(childOffset);
}

bool BoundingBoxGizmo::onMouseButtonDown(SDL_MouseButtonEvent& event)
{
    // Check if the mouse press hit any of our controls.
    SDL_Point mousePress{event.x, event.y};
    if (SDL_PointInRect(&mousePress, &lastRenderedPosExtent)) {
        currentHeldControl = Control::Position;
        return true;
    }
    else if (SDL_PointInRect(&mousePress, &lastRenderedXExtent)) {
        currentHeldControl = Control::X;
        return true;
    }
    else if (SDL_PointInRect(&mousePress, &lastRenderedYExtent)) {
        currentHeldControl = Control::Y;
        return true;
    }
    else if (SDL_PointInRect(&mousePress, &lastRenderedZExtent)) {
        currentHeldControl = Control::Z;
        return true;
    }

    return false;
}

bool BoundingBoxGizmo::onMouseButtonUp(SDL_MouseButtonEvent& event)
{
    ignore(event);

    // If we were being pressed, release it.
    if (currentHeldControl != Control::None) {
        currentHeldControl = Control::None;
        return true;
    }
    else {
        // We weren't being pressed.
        return false;
    }
}

void BoundingBoxGizmo::onMouseMove(SDL_MouseMotionEvent& event)
{
    // If we aren't being pressed, ignore the event.
    if (currentHeldControl == Control::None) {
        return;
    }

    /* Translate the mouse position to world space. */
    // Account for the sprite's empty vertical space.
    int yOffset = AUI::ScalingHelpers::logicalToActual(activeSprite->yOffset);
    yOffset += lastRenderedExtent.y;

    // Account for the sprite's half-tile offset.
    int xOffset = AUI::ScalingHelpers::logicalToActual(SharedConfig::TILE_SCREEN_WIDTH / 2.f);
    xOffset += lastRenderedExtent.x;

    // Apply the offset to the mouse position and convert to logical space.
    SDL_Point offsetMousePoint{event.x - xOffset, event.y - yOffset};
    offsetMousePoint = AUI::ScalingHelpers::actualToLogical(offsetMousePoint);

    // Convert the screen-space mouse point to world space.
    ScreenPoint offsetMousePointSP{static_cast<float>(offsetMousePoint.x)
        , static_cast<float>(offsetMousePoint.y)};
    Position mouseWorldPos = TransformationHelpers::screenToWorld(offsetMousePointSP, 1);

    // Adjust the currently pressed control appropriately.
    switch (currentHeldControl) {
        case Control::Position: {
            updatePositionBounds(mouseWorldPos);
            break;
        }
        case Control::X: {
            activeSprite->modelBounds.minX = mouseWorldPos.x;
            break;
        }
        case Control::Y: {
            activeSprite->modelBounds.minY = mouseWorldPos.y;
            break;
        }
        case Control::Z: {
            updateZBounds(event.y);
            break;
        }
        default: {
            break;
        }
    }

    // Refresh the UI so it reflects the changed position.
    mainScreen.refreshActiveSpriteUi();
}

bool BoundingBoxGizmo::refreshScaling()
{
    // If scaledExtent was refreshed, do our specialized refreshing.
    if (Component::refreshScaling()) {
        // Re-calculate our control rectangle size.
        scaledRectSize = AUI::ScalingHelpers::logicalToActual(LOGICAL_RECT_SIZE);

        // Re-calculate our line width.
        scaledLineWidth = AUI::ScalingHelpers::logicalToActual(LOGICAL_LINE_WIDTH);

        return true;
    }

    return false;
}

void BoundingBoxGizmo::updatePositionBounds(const Position& mouseWorldPos)
{
    // Note: The expected behavior is to move along the x/y plane and
    //       leave minZ where it was.

    // Move the min bounds to follow the max bounds.
    float diffX{mouseWorldPos.x - activeSprite->modelBounds.maxX};
    float diffY{mouseWorldPos.y - activeSprite->modelBounds.maxY};
    activeSprite->modelBounds.minX += diffX;
    activeSprite->modelBounds.minY += diffY;

    // Move the max bounds to their new position.
    activeSprite->modelBounds.maxX = mouseWorldPos.x;
    activeSprite->modelBounds.maxY = mouseWorldPos.y;
}

void BoundingBoxGizmo::updateZBounds(int mouseScreenYPos)
{
    // Note: The screenToWorld() transformation can't handle z-axis
    // movement (not enough data from a 2d point), so we have to do it
    // using our contextual information.

    // Set maxZ relative to the distance between the mouse and the
    // position control (the position control is always our reference
    // for where z == 0 is.)
    float mouseZHeight = lastRenderedPosExtent.y
        + (scaledRectSize / 2.f) - mouseScreenYPos;

    // Convert to logical space.
    mouseZHeight = AUI::ScalingHelpers::actualToLogical(mouseZHeight);

    // Apply our screen -> world Z scaling.
    mouseZHeight = TransformationHelpers::screenYToWorldZ(mouseZHeight, 1);

    activeSprite->modelBounds.maxZ = mouseZHeight;
}

void BoundingBoxGizmo::calcOffsetScreenPoints(std::vector<SDL_Point>& boundsScreenPoints)
{
    /* Transform the world positions to screen points. */
    // Set up a vector of float points so we can maintain precision until
    // the end.
    std::array<ScreenPoint, 7> floatPoints{};

    // Push the points in the correct order.
    BoundingBox& modelBounds = activeSprite->modelBounds;
    Position position{modelBounds.minX, modelBounds.maxY, modelBounds.minZ};
    floatPoints[0] = TransformationHelpers::worldToScreen(position, 1);

    position = {modelBounds.maxX, modelBounds.maxY, modelBounds.minZ};
    floatPoints[1] = TransformationHelpers::worldToScreen(position, 1);

    position = {modelBounds.maxX, modelBounds.minY, modelBounds.minZ};
    floatPoints[2] = TransformationHelpers::worldToScreen(position, 1);

    position = {modelBounds.minX, modelBounds.maxY, modelBounds.maxZ};
    floatPoints[3] = TransformationHelpers::worldToScreen(position, 1);

    position = {modelBounds.maxX, modelBounds.maxY, modelBounds.maxZ};
    floatPoints[4] = TransformationHelpers::worldToScreen(position, 1);

    position = {modelBounds.maxX, modelBounds.minY, modelBounds.maxZ};
    floatPoints[5] = TransformationHelpers::worldToScreen(position, 1);

    position = {modelBounds.minX, modelBounds.minY, modelBounds.maxZ};
    floatPoints[6] = TransformationHelpers::worldToScreen(position, 1);

    /* Build the offsets. */
    // Account for the sprite's empty vertical space.
    int yOffset = AUI::ScalingHelpers::logicalToActual(384);

    // Account for the sprite's half-tile offset.
    int xOffset = AUI::ScalingHelpers::logicalToActual(SharedConfig::TILE_SCREEN_WIDTH / 2.f);

    /* Scale and offset each point, then push it into the return vector. */
    for (ScreenPoint& point : floatPoints)
    {
        // Scale and round the point.
        point.x = std::round(AUI::ScalingHelpers::logicalToActual(point.x));
        point.y = std::round(AUI::ScalingHelpers::logicalToActual(point.y));

        // Offset the point.
        point.x += xOffset;
        point.y += yOffset;

        // Cast to int and push into the return vector.
        boundsScreenPoints.push_back({static_cast<int>(point.x)
            , static_cast<int>(point.y)});
    }
}

void BoundingBoxGizmo::moveControls(std::vector<SDL_Point>& boundsScreenPoints)
{
    // Calc half the control rectangle size so we can center the controls.
    int halfRectSize = static_cast<int>(scaledRectSize / 2.f);

    // Move the control extents.
    positionControlExtent.x = boundsScreenPoints[1].x - halfRectSize;
    positionControlExtent.y = boundsScreenPoints[1].y - halfRectSize;

    xControlExtent.x = boundsScreenPoints[0].x - halfRectSize;
    xControlExtent.y = boundsScreenPoints[0].y - halfRectSize;

    yControlExtent.x = boundsScreenPoints[2].x - halfRectSize;
    yControlExtent.y = boundsScreenPoints[2].y - halfRectSize;

    zControlExtent.x = boundsScreenPoints[4].x - halfRectSize;
    zControlExtent.y = boundsScreenPoints[4].y - halfRectSize;
}

void BoundingBoxGizmo::moveLines(std::vector<SDL_Point>& boundsScreenPoints)
{
    // Move the lines.
    xMinPoint = {boundsScreenPoints[0].x, boundsScreenPoints[0].y};
    xMaxPoint = {boundsScreenPoints[1].x, boundsScreenPoints[1].y};

    yMinPoint = {boundsScreenPoints[2].x, boundsScreenPoints[2].y};
    yMaxPoint = {boundsScreenPoints[1].x, boundsScreenPoints[1].y};

    zMinPoint = {boundsScreenPoints[1].x, boundsScreenPoints[1].y};
    zMaxPoint = {boundsScreenPoints[4].x, boundsScreenPoints[4].y};
}

void BoundingBoxGizmo::movePlanes(std::vector<SDL_Point>& boundsScreenPoints)
{
    // Set the coords for the X-axis plane (coords 0 - 3, starting from top
    // left and going clockwise.)
    planeXCoords[0] = boundsScreenPoints[4].x;
    planeYCoords[0] = boundsScreenPoints[4].y;
    planeXCoords[1] = boundsScreenPoints[5].x;
    planeYCoords[1] = boundsScreenPoints[5].y;
    planeXCoords[2] = boundsScreenPoints[2].x;
    planeYCoords[2] = boundsScreenPoints[2].y;
    planeXCoords[3] = boundsScreenPoints[1].x;
    planeYCoords[3] = boundsScreenPoints[1].y;

    // Set the coords for the Y-axis plane (coords 4 - 7, starting from top
    // left and going clockwise.)
    planeXCoords[4] = boundsScreenPoints[3].x;
    planeYCoords[4] = boundsScreenPoints[3].y;
    planeXCoords[5] = boundsScreenPoints[4].x;
    planeYCoords[5] = boundsScreenPoints[4].y;
    planeXCoords[6] = boundsScreenPoints[1].x;
    planeYCoords[6] = boundsScreenPoints[1].y;
    planeXCoords[7] = boundsScreenPoints[0].x;
    planeYCoords[7] = boundsScreenPoints[0].y;

    // Set the coords for the Z-axis plane (coords 8 - 11, starting from top
    // left and going clockwise.)
    planeXCoords[8] = boundsScreenPoints[6].x;
    planeYCoords[8] = boundsScreenPoints[6].y;
    planeXCoords[9] = boundsScreenPoints[5].x;
    planeYCoords[9] = boundsScreenPoints[5].y;
    planeXCoords[10] = boundsScreenPoints[4].x;
    planeYCoords[10] = boundsScreenPoints[4].y;
    planeXCoords[11] = boundsScreenPoints[3].x;
    planeYCoords[11] = boundsScreenPoints[3].y;
}

void BoundingBoxGizmo::renderControls(const SDL_Point& childOffset)
{
    // Position control
    SDL_Rect offsetExtent{positionControlExtent};
    offsetExtent.x += childOffset.x;
    offsetExtent.y += childOffset.y;
    lastRenderedPosExtent = offsetExtent;

    SDL_SetRenderDrawColor(AUI::Core::GetRenderer(), 0, 0, 0, 255);
    SDL_RenderFillRect(AUI::Core::GetRenderer(), &lastRenderedPosExtent);

    // X control
    offsetExtent = xControlExtent;
    offsetExtent.x += childOffset.x;
    offsetExtent.y += childOffset.y;
    lastRenderedXExtent = offsetExtent;

    SDL_SetRenderDrawColor(AUI::Core::GetRenderer(), 148, 0, 0, 255);
    SDL_RenderFillRect(AUI::Core::GetRenderer(), &lastRenderedXExtent);

    // Y control
    offsetExtent = yControlExtent;
    offsetExtent.x += childOffset.x;
    offsetExtent.y += childOffset.y;
    lastRenderedYExtent = offsetExtent;

    SDL_SetRenderDrawColor(AUI::Core::GetRenderer(), 0, 149, 0, 255);
    SDL_RenderFillRect(AUI::Core::GetRenderer(), &lastRenderedYExtent);

    // Z control
    offsetExtent = zControlExtent;
    offsetExtent.x += childOffset.x;
    offsetExtent.y += childOffset.y;
    lastRenderedZExtent = offsetExtent;

    SDL_SetRenderDrawColor(AUI::Core::GetRenderer(), 0, 82, 240, 255);
    SDL_RenderFillRect(AUI::Core::GetRenderer(), &lastRenderedZExtent);
}

void BoundingBoxGizmo::renderLines(const SDL_Point& childOffset)
{
    // X-axis line
    SDL_Point offsetMinPoint{xMinPoint};
    SDL_Point offsetMaxPoint{xMaxPoint};
    offsetMinPoint.x += childOffset.x;
    offsetMinPoint.y += childOffset.y;
    offsetMaxPoint.x += childOffset.x;
    offsetMaxPoint.y += childOffset.y;

    thickLineRGBA(AUI::Core::GetRenderer(), offsetMinPoint.x, offsetMinPoint.y,
        offsetMaxPoint.x, offsetMaxPoint.y, scaledLineWidth, 148, 0, 0, 255);

    // Y-axis line
    offsetMinPoint = yMinPoint;
    offsetMaxPoint = yMaxPoint;
    offsetMinPoint.x += childOffset.x;
    offsetMinPoint.y += childOffset.y;
    offsetMaxPoint.x += childOffset.x;
    offsetMaxPoint.y += childOffset.y;

    thickLineRGBA(AUI::Core::GetRenderer(), offsetMinPoint.x, offsetMinPoint.y,
        offsetMaxPoint.x, offsetMaxPoint.y, scaledLineWidth, 0, 149, 0, 255);

    // Z-axis line
    offsetMinPoint = zMinPoint;
    offsetMaxPoint = zMaxPoint;
    offsetMinPoint.x += childOffset.x;
    offsetMinPoint.y += childOffset.y;
    offsetMaxPoint.x += childOffset.x;
    offsetMaxPoint.y += childOffset.y;

    thickLineRGBA(AUI::Core::GetRenderer(), offsetMinPoint.x, offsetMinPoint.y,
        offsetMaxPoint.x, offsetMaxPoint.y, scaledLineWidth, 0, 82, 240, 255);
}

void BoundingBoxGizmo::renderPlanes(const SDL_Point& childOffset)
{
    /* Offset all the points. */
    std::array<Sint16, 12> offsetXCoords{};
    for (unsigned int i = 0; i < offsetXCoords.size(); ++i) {
        offsetXCoords[i] = planeXCoords[i] + childOffset.x;
    }

    std::array<Sint16, 12> offsetYCoords{};
    for (unsigned int i = 0; i < offsetYCoords.size(); ++i) {
        offsetYCoords[i] = planeYCoords[i] + childOffset.y;
    }

    /* Draw the planes. */
    // X-axis plane
    filledPolygonRGBA(AUI::Core::GetRenderer(), &(offsetXCoords[0])
        , &(offsetYCoords[0]), 4, 148, 0, 0, 127);

    // Y-axis plane
    filledPolygonRGBA(AUI::Core::GetRenderer(), &(offsetXCoords[4])
        , &(offsetYCoords[4]), 4, 0, 149, 0, 127);

    // Z-axis plane
    filledPolygonRGBA(AUI::Core::GetRenderer(), &(offsetXCoords[8])
        , &(offsetYCoords[8]), 4, 0, 82, 240, 127);
}

} // End namespace SpriteEditor
} // End namespace AM
