#include "NetworkOutputSystem.h"
#include "Game.h"
#include "World.h"
#include "Network.h"
#include "MessageUtil.h"
#include "Debug.h"

namespace AM
{
namespace Client
{

NetworkOutputSystem::NetworkOutputSystem(Game& inGame, World& inWorld, Network& inNetwork)
: game(inGame)
, world(inWorld)
, network(inNetwork)
, builder(BUILDER_BUFFER_SIZE)
, accumulatedTime(0.0f)
{
}

void NetworkOutputSystem::updateServer(float deltaSeconds)
{
    accumulatedTime += deltaSeconds;

    // Process as many network ticks as have accumulated.
    if (accumulatedTime >= NETWORK_OUTPUT_TICK_INTERVAL_S) {
        sendInputState();

        accumulatedTime -= NETWORK_OUTPUT_TICK_INTERVAL_S;
        if (accumulatedTime >= NETWORK_OUTPUT_TICK_INTERVAL_S) {
            // If we've accumulated enough time to send more, something
            // happened to delay us.
            // We still only want to send the latest data, but it's worth giving
            // debug output that we detected this.
            DebugInfo(
                "Detected a delayed network send. accumulatedTime: %f. Setting to 0.",
                accumulatedTime);
            accumulatedTime = 0;
        }
    }
}

void NetworkOutputSystem::sendInputState()
{
    if (!world.playerIsDirty) {
        // If there's no change, don't send anything.
        return;
    }

    /* Send the updated state to the server. */
    // Prep the builder for a new message.
    builder.Clear();

    EntityID playerID = world.playerID;

    // Create the vector of entity data.
    std::vector<flatbuffers::Offset<fb::Entity>> entityVector;
    entityVector.push_back(serializeEntity(playerID));
    auto serializedEntity = builder.CreateVector(entityVector);

    // Build an EntityUpdate.
    flatbuffers::Offset<fb::EntityUpdate> entityUpdate = fb::CreateEntityUpdate(builder, game.getCurrentTick()
    , serializedEntity);

    // Build a Message.
    fb::MessageBuilder messageBuilder(builder);
    messageBuilder.add_content_type(fb::MessageContent::EntityUpdate);
    messageBuilder.add_content(entityUpdate.Union());
    flatbuffers::Offset<fb::Message> message = messageBuilder.Finish();
    builder.Finish(message);

    // Send the message.
    Uint8* buffer = builder.GetBufferPointer();
    network.send(
        std::make_shared<std::vector<Uint8>>(buffer, (buffer + builder.GetSize())));

    world.playerIsDirty = false;
}

flatbuffers::Offset<AM::fb::Entity> NetworkOutputSystem::serializeEntity(
EntityID playerID)
{
    // Translate the inputs to fb's enum.
    fb::InputState fbInputStates[Input::Type::NumTypes];
    std::array<Input::State, Input::NumTypes>& playerInputStates =
        world.inputs[playerID].inputStates;
    for (uint8_t i = 0; i < Input::Type::NumTypes; ++i) {
        // Translate the Input::State enum to fb::InputState.
        fbInputStates[i] = MessageUtil::convertToFbInputState(playerInputStates[i]);
    }
    flatbuffers::Offset<flatbuffers::Vector<fb::InputState>> inputVector =
        builder.CreateVector(fbInputStates, Input::Type::NumTypes);

    // Build the inputComponent.
    flatbuffers::Offset<fb::InputComponent> inputComponent = fb::CreateInputComponent(
        builder, inputVector);

    // Build the Entity.
    auto entityName = builder.CreateString(world.entityNames[playerID]);
    fb::EntityBuilder entityBuilder(builder);
    entityBuilder.add_id(playerID);
    entityBuilder.add_name(entityName);

    // Mark that we only are sending the InputComponent.
    entityBuilder.add_flags(ComponentFlag::Input);
    entityBuilder.add_inputComponent(inputComponent);

    return entityBuilder.Finish();
}

} // namespace Client
} // namespace AM