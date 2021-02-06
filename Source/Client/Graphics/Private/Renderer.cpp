#include "Renderer.h"
#include "World.h"
#include "Sim.h"
#include "Position.h"
#include "PreviousPosition.h"
#include "Sprite.h"
#include "Camera.h"
#include "MovementHelpers.h"
#include "ScreenRect.h"
#include "SDL2pp/Rect.hh"
#include "Log.h"
#include "Ignore.h"
#include <cmath>

namespace AM
{
namespace Client
{
Renderer::Renderer(SDL2pp::Renderer& inSdlRenderer, Sim& inSim,
                           SDL2pp::Window& window)
: sdlRenderer(inSdlRenderer)
, sim(inSim)
, world(sim.getWorld())
, accumulatedTime(0.0)
{
    // Init the groups that we'll be using.
    auto group
        = world.registry.group<Sprite>(entt::get<Position, PreviousPosition>);
    ignore(group);

    // TODO: This will eventually be used when we get to variable window sizes.
    ignore(window);
}

void Renderer::tick()
{
    accumulatedTime += frameTimer.getDeltaSeconds(true);

    if (accumulatedTime >= RENDER_TICK_TIMESTEP_S) {
        // Process the rendering for this frame.
        render();

        accumulatedTime -= RENDER_TICK_TIMESTEP_S;
        if (accumulatedTime >= RENDER_TICK_TIMESTEP_S) {
            // If we've accumulated enough time to render again, something
            // happened (probably a window event that stopped app execution.)
            // We still only want to render the latest data, but it's worth
            // giving debug output that we detected this.
            LOG_INFO("Detected a request for two renders in the same frame. "
                     "Render must have been massively delayed. Render was "
                     "delayed by: %.8fs. Setting to 0.",
                     accumulatedTime);
            accumulatedTime = 0;
        }
    }
}

void Renderer::initTimer()
{
    frameTimer.updateSavedTime();
}

double Renderer::getTimeTillNextFrame()
{
    // The time since accumulatedTime was last updated.
    double timeSinceIteration = frameTimer.getDeltaSeconds(false);
    return (RENDER_TICK_TIMESTEP_S - (accumulatedTime + timeSinceIteration));
}

void Renderer::render()
{
    /* Set up the camera for this frame. */
    // Get how far we are between game ticks in decimal percent.
    double alpha = sim.getIterationProgress();

    // Get the lerped camera position based on the alpha.
    auto [playerCamera, playerSprite, playerPosition, playerPreviousPos]
        = world.registry.get<Camera, Sprite, Position, PreviousPosition>(
            world.playerEntity);
    Position cameraLerp = MovementHelpers::interpolatePosition(
        playerCamera.prevPosition, playerCamera.position, alpha);

    // Set the camera at the lerped position.
    Camera lerpedCamera = playerCamera;
    lerpedCamera.position.x = cameraLerp.x;
    lerpedCamera.position.y = cameraLerp.y;

    // Get iso screen coords for the center point of camera.
    ScreenPoint lerpedCameraCenter = worldToScreen(lerpedCamera.position.x, lerpedCamera.position.y, lerpedCamera.zoomFactor);

    // Calc where the top left of the lerpedCamera is in screen space.
    lerpedCamera.extent.x = lerpedCameraCenter.x - (lerpedCamera.extent.width / 2);
    lerpedCamera.extent.y = lerpedCameraCenter.y - (lerpedCamera.extent.height / 2);

    /* Render. */
    // Clear the screen to prepare for drawing.
    sdlRenderer.Clear();

    // Render the world tiles first.
    renderTiles(lerpedCamera);

    // Render all entities, lerping between their previous and next positions.
    renderEntities(lerpedCamera, alpha);

    sdlRenderer.Present();
}

void Renderer::renderTiles(Camera& camera)
{
    for (unsigned int y = 0; y < WORLD_HEIGHT; ++y) {
        for (unsigned int x = 0; x < WORLD_WIDTH; ++x) {
            // Get iso screen coords for this tile.
            ScreenPoint tileScreenPos = tileToScreen(x, y, camera.zoomFactor);

            // Get the sprite's vertical offset (iso sprites may have extra
            // vertical space to show depth, we just want the tile.)
            Sprite& sprite{world.terrainMap[y * WORLD_WIDTH + x]};
            float spriteOffsetY = (sprite.height - TILE_SCREEN_HEIGHT
                                - TILE_SCREEN_EDGE_HEIGHT) * camera.zoomFactor;

            // Apply the camera adjustment.
            int adjustedX = static_cast<int>(round(tileScreenPos.x - camera.extent.x));
            int adjustedY = static_cast<int>(round(tileScreenPos.y - spriteOffsetY - camera.extent.y));

            // Apply the camera's zoom.
            int zoomedWidth = static_cast<int>(round(sprite.width * camera.zoomFactor));
            int zoomedHeight = static_cast<int>(round(sprite.height * camera.zoomFactor));

            // Draw the tile.
            SDL2pp::Rect spriteScreenExtent =
                {adjustedX, adjustedY, zoomedWidth, zoomedHeight};
            sdlRenderer.Copy(*(sprite.texturePtr), sprite.texExtent, spriteScreenExtent);
        }
    }
}

void Renderer::renderEntities(Camera& camera, double alpha)
{
    // Render all entities with a Sprite, PreviousPosition and Position.
    auto group
        = world.registry.group<Sprite>(entt::get<Position, PreviousPosition>);
    for (entt::entity entity : group) {
        auto [sprite, position, previousPos]
            = group.get<Sprite, Position, PreviousPosition>(entity);

        // Get the lerp'd world position and translate it to iso screen space.
        Position lerp = MovementHelpers::interpolatePosition(previousPos,
                                                             position, alpha);
        ScreenPoint entityScreenPos = worldToScreen(lerp.x, lerp.y, camera.zoomFactor);

        // Apply the camera adjustment to the entity.
        float adjustedX = entityScreenPos.x - camera.extent.x;
        float adjustedY = entityScreenPos.y - camera.extent.y;

        // Prep the sprite data, accounting for camera zoom.
        float zoomedWidth = sprite.width * camera.zoomFactor;
        float zoomedHeight = sprite.height * camera.zoomFactor;

        // If the entity's sprite is within the camera bounds, render it.
        if (isWithinCameraBounds(adjustedX, adjustedY, zoomedWidth, zoomedHeight, camera)) {
            SDL2pp::Rect spriteScreenExtent {static_cast<int>(round(adjustedX)),
                    static_cast<int>(round(adjustedY)), static_cast<int>(round(zoomedWidth)),
                    static_cast<int>(round(zoomedHeight))};
            sdlRenderer.Copy(*(sprite.texturePtr), sprite.texExtent, spriteScreenExtent);
        }
    }
}

ScreenPoint Renderer::tileToScreen(int xIndex, int yIndex, float zoom) {
    // Convert tile index to isometric screen position.
    float screenX = (xIndex - yIndex) * (TILE_SCREEN_WIDTH / 2.f);
    float screenY = (xIndex + yIndex) * (TILE_SCREEN_HEIGHT / 2.f);

    // Apply the camera zoom.
    screenX *= zoom;
    screenY *= zoom;

    return {screenX, screenY};
}

ScreenPoint Renderer::worldToScreen(float x, float y, float zoom) {
    // Calc the scaling factor between world tile and screen tile sizes.
    static const float TILE_WIDTH_SCALE = static_cast<float>(TILE_SCREEN_WIDTH) / TILE_WORLD_WIDTH;
    static const float TILE_HEIGHT_SCALE = static_cast<float>(TILE_SCREEN_HEIGHT) / TILE_WORLD_HEIGHT;

    // Convert cartesian world point to isometric screen point.
    float screenX = (x - y) * (TILE_WIDTH_SCALE / 2.f);
    float screenY = (x + y)  * (TILE_HEIGHT_SCALE / 2.f);

    // Add a half tile X-axis offset, since (0, 0) starts at the midpoint in a
    // tile sprite, but doesn't in a non-tile sprite.
    screenX += (TILE_SCREEN_WIDTH / 2);

    // Apply the camera zoom.
    screenX *= zoom;
    screenY *= zoom;

    return {screenX, screenY};
}

bool Renderer::isWithinCameraBounds(float x, float y, float width, float height, Camera& camera)
{
    // If the sprite is within view of the camera, return true.
    if ((x + width) < 0 || (camera.extent.width < x) || ((y + height) < 0)
        || (camera.extent.height < y)) {
        return false;
    }
    else {
        return true;
    }
}

} // namespace Client
} // namespace AM
