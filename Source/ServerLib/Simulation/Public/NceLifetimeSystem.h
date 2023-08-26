#pragma once

#include "DynamicObjectInitRequest.h"
#include "EntityDelete.h"
#include "QueuedEvents.h"
#include <queue>

namespace sol
{
class state;
}

namespace AM
{
namespace Server
{

class World;
class Network;
class SpriteData;
class ISimulationExtension;

/**
 * Manages creation and destruction of non-client entities.
 *
 * Non-client entities are any entity not controlled by a client (dynamic 
 * objects, NPCs, etc).
 */
class NceLifetimeSystem
{
public:
    NceLifetimeSystem(World& inWorld, Network& inNetwork,
                      SpriteData& inSpriteData, sol::state& inLua,
                      const ISimulationExtension* inExtension);

    /**
     * Processes any waiting EntityCreateRequest or EntityDelete messages.
     */
    void processUpdates();

private:
    /**
     * Either creates the given object and initializes it, or re-creates it 
     * and queues an init for next tick.
     */
    void createDynamicObject(
        const DynamicObjectInitRequest& objectInitRequest);

    /**
     * Creates and sets the given object's components and adds it to the locator.
     */
    void initDynamicObject(entt::entity newEntity,
        const DynamicObjectInitRequest& objectInitRequest);

    /** Used to add/remove entities. */
    World& world;

    /** Used to get sprite data when adding an entity. */
    SpriteData& spriteData;

    /** Used to run entity init scripts. */
    sol::state& lua;

    /** If non-nullptr, contains the project's simulation extension functions.
        Used for checking if entity creation requests are valid. */
    const ISimulationExtension* extension;

    /** Holds objects that need to be re-initialized on the next tick. */
    std::queue<DynamicObjectInitRequest> objectReInitQueue;

    EventQueue<DynamicObjectInitRequest> objectInitRequestQueue;
    EventQueue<EntityDelete> deleteQueue;
};

} // End namespace Server
} // End namespace AM
