#include "ClientAOISystem.h"
#include "Simulation.h"
#include "World.h"
#include "Network.h"
#include "Serialize.h"
#include "ClientSimData.h"
#include "PositionHasChanged.h"
#include "BoundingBox.h"
#include "Name.h"
#include "Sprite.h"
#include "EntityDelete.h"
#include "EntityInit.h"
#include "SharedConfig.h"
#include "Log.h"
#include <algorithm>

namespace AM
{
namespace Server
{
ClientAOISystem::ClientAOISystem(Simulation& inSim, World& inWorld,
                                       Network& inNetwork)
: sim{inSim}
, world(inWorld)
, network(inNetwork)
{
}

void ClientAOISystem::updateAOILists()
{
    // Process the entities that have recently moved.
    auto view{world.registry.view<ClientSimData, PositionHasChanged, BoundingBox>()};
    for (entt::entity entityThatMoved : view) {
        auto [clientThatMoved, boundingBox] = view.get<ClientSimData, BoundingBox>(entityThatMoved);

        // Clear our lists.
        entitiesThatLeft.clear();
        clientThatMoved.entitiesThatEnteredAOI.clear();

        // Get the list of entities that are in entityThatMoved's AOI.
        std::vector<entt::entity>& currentAOIEntities {
                world.entityLocator.getEntitiesFine(boundingBox.getCenterPosition(),
                    SharedConfig::AOI_RADIUS) };

        // Remove entityThatMoved from the list, if it's in there.
        // (We don't want to add it to its own list.)
        auto entityIt { std::find(currentAOIEntities.begin(), currentAOIEntities.end(),
            entityThatMoved) };
        if (entityIt != currentAOIEntities.end()) {
            currentAOIEntities.erase(entityIt);
        }

        // Sort the list.
        std::sort(currentAOIEntities.begin(), currentAOIEntities.end());

        // Fill entitiesThatLeft with the entities that left entityThatMoved's AOI.
        std::vector<entt::entity>& oldAOIEntities{clientThatMoved.entitiesInAOI};
        std::set_difference(oldAOIEntities.begin(), oldAOIEntities.end(),
            currentAOIEntities.begin(), currentAOIEntities.end()
            , std::back_inserter(entitiesThatLeft));

        // Process the entities that left entityThatMoved's AOI.
        if (entitiesThatLeft.size() > 0) {
            processEntitiesThatLeft(entityThatMoved, clientThatMoved);
        }

        // Fill entitiesThatEntered with entities that entered entityThatMoved's AOI.
        std::set_difference(currentAOIEntities.begin(), currentAOIEntities.end(),
            oldAOIEntities.begin(), oldAOIEntities.end()
            , std::back_inserter(clientThatMoved.entitiesThatEnteredAOI));
//
//        std::string priorList{};
//        for (entt::entity entity : oldAOIEntities) {
//            priorList += (std::to_string(static_cast<unsigned int>(entity)) + ", ");
//        }
//        std::string currentList{};
//        for (entt::entity entity : currentAOIEntities) {
//            currentList += (std::to_string(static_cast<unsigned int>(entity)) + ", ");
//        }
//        std::string leftList{};
//        for (entt::entity entity : entitiesThatLeft) {
//            leftList += (std::to_string(static_cast<unsigned int>(entity)) + ", ");
//        }
//        std::string enteredList{};
//        for (entt::entity entity : clientThatMoved.entitiesThatEnteredAOI) {
//            enteredList += (std::to_string(static_cast<unsigned int>(entity)) + ", ");
//        }
//        LOG_INFO("List for %u: (%s) -> (%s). Left: (%s), Entered: (%s)", entityThatMoved, priorList.c_str()
//                 , currentList.c_str(), leftList.c_str(), enteredList.c_str());

        // Process the entities that entered entityThatMoved's AOI.
        if (clientThatMoved.entitiesThatEnteredAOI.size() > 0) {
            processEntitiesThatEntered(entityThatMoved, clientThatMoved);
        }

        // Save the new list.
        clientThatMoved.entitiesInAOI = currentAOIEntities;
    }

    // Mark any dirty entity positions as clean.
    world.registry.clear<PositionHasChanged>();
}

void ClientAOISystem::processEntitiesThatLeft(entt::entity entityThatMoved,
                                                    ClientSimData& clientThatMoved)
{
    auto view{world.registry.view<ClientSimData>()};

    // Process the entities that entered entityThatMoved's AOI.
    for (entt::entity entityThatLeft : entitiesThatLeft) {
        // Tell clientThatMoved that entityThatLeft is now out of range.
        network.serializeAndSend(clientThatMoved.netID, EntityDelete{sim.getCurrentTick(), entityThatLeft});
//        LOG_INFO("Sent EntityDelete to %u about %u", entityThatMoved, entityThatLeft);

        // If entityThatLeft is a client entity and hasn't moved on this tick,
        // update its list and send it a EntityDelete for entityThatMoved.
        // Note: If it moved, it's already going to update its lists so we
        //       don't have to.
        if (world.registry.any_of<ClientSimData>(entityThatLeft)
            && !(world.registry.any_of<PositionHasChanged>(entityThatLeft))) {
            // Find entityThatMoved in entityThatLeft's list.
            ClientSimData& clientThatLeft{view.get<ClientSimData>(entityThatLeft)};
            auto eraseIt { std::find(clientThatLeft.entitiesInAOI.begin(),
                clientThatLeft.entitiesInAOI.end(), entityThatMoved) };

            // Remove entityThatMoved from entityThatLeft's list.
            if (eraseIt != clientThatLeft.entitiesInAOI.end()) {
                clientThatLeft.entitiesInAOI.erase(eraseIt);
            }
            else {
                LOG_FATAL("Failed to find expected entity when erasing.");
            }

            // Tell clientThatLeft that entityThatMoved is now out of range.
            network.serializeAndSend(clientThatLeft.netID,
                EntityDelete { sim.getCurrentTick(), entityThatMoved });
//            LOG_INFO("Sent EntityDelete to %u about %u", entityThatLeft, entityThatMoved);
        }
    }
//    LOG_INFO("Processed %u entities that left the AOI of entity %u",
//        entitiesThatLeft.size(), entityThatMoved);
}

void ClientAOISystem::processEntitiesThatEntered(entt::entity entityThatMoved,
                                                    ClientSimData& clientThatMoved)
{
    auto view{world.registry.view<ClientSimData, Name, Sprite>()};

    // Process the entities that entered entityThatMoved's AOI.
    for (entt::entity entityThatEntered : clientThatMoved.entitiesThatEnteredAOI) {
        // Tell clientThatMoved that entityThatEntered is now in range.
        Name& movedName{view.get<Name>(entityThatMoved)};
        Sprite& movedSprite{view.get<Sprite>(entityThatMoved)};
        network.serializeAndSend(clientThatMoved.netID, EntityInit { sim.getCurrentTick(),
                entityThatEntered, movedName.name, movedSprite.numericID });
//        LOG_INFO("Sent EntityInit to %u about %u", entityThatMoved, entityThatEntered);

        // If entityThatEntered is a client entity and hasn't moved on this
        // tick, update its list and send it an EntityInit.
        // Note: If it moved, it's already going to update its lists so we
        //       don't have to.
        if (world.registry.any_of<ClientSimData>(entityThatEntered)
            && !(world.registry.any_of<PositionHasChanged>(entityThatEntered))) {
            // Add entityThatMoved to entityThatEntered's lists.
            ClientSimData& clientThatEntered{view.get<ClientSimData>(entityThatEntered)};
            clientThatEntered.entitiesInAOI.push_back(entityThatMoved);
            clientThatEntered.entitiesThatEnteredAOI.push_back(entityThatMoved);

            // Re-sort clientThatEntered's lists.
            std::sort(clientThatEntered.entitiesInAOI.begin(),
                clientThatEntered.entitiesInAOI.end());
            std::sort(clientThatEntered.entitiesThatEnteredAOI.begin(),
                clientThatEntered.entitiesThatEnteredAOI.end());

            // Tell clientThatEntered that entityThatMoved is now in range.
            Name& enteredName{view.get<Name>(entityThatEntered)};
            Sprite& enteredSprite{view.get<Sprite>(entityThatEntered)};
            network.serializeAndSend(clientThatEntered.netID,
                EntityInit { sim.getCurrentTick(), entityThatMoved, enteredName.name,
                        enteredSprite.numericID });
//            LOG_INFO("Sent EntityInit to %u about %u", entityThatEntered, entityThatMoved);
        }
    }
//    LOG_INFO("Processed %u entities that entered the AOI of entity %u",
//        entitiesThatEntered.size(), entityThatMoved);
}

} // namespace Server
} // namespace AM