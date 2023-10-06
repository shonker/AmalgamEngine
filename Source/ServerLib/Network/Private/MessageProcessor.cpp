#include "MessageProcessor.h"
#include "QueuedEvents.h"
#include "Deserialize.h"
#include "DispatchMessage.h"
#include "IMessageProcessorExtension.h"
#include "ServerNetworkDefs.h"
#include "Heartbeat.h"
#include "InputChangeRequest.h"
#include "ChunkUpdateRequest.h"
#include "EntityInitRequest.h"
#include "EntityDeleteRequest.h"
#include "NameChangeRequest.h"
#include "AnimationStateChangeRequest.h"
#include "InitScriptRequest.h"
#include "InteractionRequest.h"
#include "TileAddLayer.h"
#include "TileRemoveLayer.h"
#include "TileClearLayers.h"
#include "TileExtentClearLayers.h"
#include "EntityDelete.h"
#include "Log.h"
#include <span>

namespace AM
{
namespace Server
{
template<typename T>
void dispatchWithNetID(NetworkID netID, std::span<Uint8> messageBuffer,
                       EventDispatcher& dispatcher)
{
    // Deserialize the message.
    T message{};
    Deserialize::fromBuffer(messageBuffer.data(), messageBuffer.size(), message);

    // Fill in the network ID that we assigned to this client.
    message.netID = netID;

    // Push the message into any subscribed queues.
    dispatcher.push<T>(message);
}

MessageProcessor::MessageProcessor(EventDispatcher& inNetworkEventDispatcher)
: networkEventDispatcher{inNetworkEventDispatcher}
{
}

Sint64 MessageProcessor::processReceivedMessage(NetworkID netID,
                                                Uint8 messageType,
                                                Uint8* messageBuffer,
                                                std::size_t messageSize)
{
    // The tick that the received message corresponds to.
    // Will be -1 if the message doesn't correspond to any tick.
    Sint64 messageTick{-1};

    // Match the enum values to their event types.
    EngineMessageType engineMessageType{
        static_cast<EngineMessageType>(messageType)};
    switch (engineMessageType) {
        case EngineMessageType::Heartbeat: {
            messageTick = static_cast<Sint64>(
                handleHeartbeat(messageBuffer, messageSize));
            break;
        }
        case EngineMessageType::InputChangeRequest: {
            messageTick = static_cast<Sint64>(
                handleInputChangeRequest(netID, messageBuffer, messageSize));
            break;
        }
        case EngineMessageType::ChunkUpdateRequest: {
            dispatchWithNetID<ChunkUpdateRequest>(
                netID, {messageBuffer, messageSize}, networkEventDispatcher);
            break;
        }
        case EngineMessageType::EntityInitRequest: {
            dispatchWithNetID<EntityInitRequest>(
                netID, {messageBuffer, messageSize}, networkEventDispatcher);
            break;
        }
        case EngineMessageType::EntityDeleteRequest: {
            dispatchMessage<EntityDeleteRequest>(
                messageBuffer, messageSize, networkEventDispatcher);
            break;
        }
        case EngineMessageType::NameChangeRequest: {
            dispatchMessage<NameChangeRequest>(messageBuffer, messageSize,
                                               networkEventDispatcher);
            break;
        }
        case EngineMessageType::AnimationStateChangeRequest: {
            dispatchMessage<AnimationStateChangeRequest>(
                messageBuffer, messageSize, networkEventDispatcher);
            break;
        }
        case EngineMessageType::InitScriptRequest: {
            dispatchWithNetID<InitScriptRequest>(
                netID, {messageBuffer, messageSize}, networkEventDispatcher);
            break;
        }
        case EngineMessageType::InteractionRequest: {
            dispatchWithNetID<InteractionRequest>(
                netID, {messageBuffer, messageSize}, networkEventDispatcher);
            break;
        }
        case EngineMessageType::TileAddLayer: {
            dispatchMessage<TileAddLayer>(messageBuffer, messageSize,
                                        networkEventDispatcher);
            break;
        }
        case EngineMessageType::TileRemoveLayer: {
            dispatchMessage<TileRemoveLayer>(messageBuffer, messageSize,
                                        networkEventDispatcher);
            break;
        }
        case EngineMessageType::TileClearLayers: {
            dispatchMessage<TileClearLayers>(messageBuffer, messageSize,
                                        networkEventDispatcher);
            break;
        }
        case EngineMessageType::TileExtentClearLayers: {
            dispatchMessage<TileExtentClearLayers>(messageBuffer, messageSize,
                                        networkEventDispatcher);
            break;
        }
        case EngineMessageType::EntityDelete: {
            dispatchMessage<EntityDelete>(messageBuffer, messageSize,
                                        networkEventDispatcher);
            break;
        }
        default: {
            // If we don't have a handler for this message type, pass it to
            // the project.
            if (extension != nullptr) {
                extension->processReceivedMessage(netID, messageType,
                                                  messageBuffer, messageSize);
            }
            break;
        }
    }

    return messageTick;
}

void MessageProcessor::setExtension(
    std::unique_ptr<IMessageProcessorExtension> inExtension)
{
    extension = std::move(inExtension);
}

Uint32 MessageProcessor::handleHeartbeat(Uint8* messageBuffer,
                                         std::size_t messageSize)
{
    // Deserialize the message.
    Heartbeat heartbeat{};
    Deserialize::fromBuffer(messageBuffer, messageSize, heartbeat);

    // Return the tick number associated with this message.
    return heartbeat.tickNum;
}

Uint32 MessageProcessor::handleInputChangeRequest(NetworkID netID,
                                                  Uint8* messageBuffer,
                                                  std::size_t messageSize)
{
    // Deserialize the message.
    InputChangeRequest inputChangeRequest{};
    Deserialize::fromBuffer(messageBuffer, messageSize, inputChangeRequest);

    // Fill in the network ID that we assigned to this client.
    inputChangeRequest.netID = netID;

    // Push the message into any subscribed queues.
    networkEventDispatcher.push<InputChangeRequest>(inputChangeRequest);

    // Return the tick number associated with this message.
    return inputChangeRequest.tickNum;
}

} // End namespace Server
} // End namespace AM
