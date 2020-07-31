#include "RenderSystem.h"
#include "World.h"
#include "Game.h"
#include "Debug.h"
#include "Ignore.h"

namespace AM
{
namespace Client
{

RenderSystem::RenderSystem(SDL2pp::Renderer& inRenderer, Game& inGame,
                           SDL2pp::Window& window)
: renderer(inRenderer), game(inGame), world(game.getWorld()), accumulatedTime(0.0f)
{
    // TODO: This will eventually be used when we get to variable window sizes.
    ignore(window);
}

void RenderSystem::render(double deltaSeconds)
{
    accumulatedTime += deltaSeconds;

    // Process the rendering for this frame.
    if (accumulatedTime >= RENDER_INTERVAL_S) {
        renderer.Clear();

        // How far we are between game ticks in decimal percent.
        float alpha = game.getAccumulatedTime() / GAME_TICK_INTERVAL_S;
        for (size_t entityID = 0; entityID < MAX_ENTITIES; ++entityID) {
            if (world.entityExists(entityID)) {
                const SpriteComponent& sprite = world.sprites[entityID];
                const PositionComponent& position = world.positions[entityID];
                const PositionComponent& oldPosition = world.oldPositions[entityID];

                // Lerp'd position based on how far we are between game ticks.
                int lerpX = std::round((position.x * alpha) + (oldPosition.x * (1.0 - alpha)));
                int lerpY = std::round((position.y * alpha) + (oldPosition.y * (1.0 - alpha)));
                SDL2pp::Rect spriteWorldData = { lerpX, lerpY, sprite.width,
                        sprite.height };

                renderer.Copy(*(world.sprites[entityID].texturePtr),
                    world.sprites[entityID].posInTexture, spriteWorldData);
            }
        }

        renderer.Present();

        accumulatedTime -= RENDER_INTERVAL_S;
        if (accumulatedTime >= RENDER_INTERVAL_S) {
            // If we've accumulated enough time to render again, something
            // happened (probably a window event that stopped app execution.)
            // We still only want to render the latest data, but it's worth giving
            // debug output that we detected this.
            DebugInfo(
                "Detected a request for two renders in the same frame. Render must have"
                "been massively delayed. Render was delayed by: %.8fs. Setting to 0.",
                accumulatedTime);
            accumulatedTime = 0;
        }
        else if (accumulatedTime >= RENDER_DELAYED_TIME_S) {
            // Delayed render could be caused by the sim taking too long, or too much printing.
            DebugInfo("Detected a delayed render. Render was delayed by: %.8fs.",
                accumulatedTime);
        }
    }

}

float RenderSystem::getAccumulatedTime()
{
    return accumulatedTime;
}

} // namespace Client
} // namespace AM
