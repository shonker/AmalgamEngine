#include "SDL.h"
#include "SDL_net.h"
#include <string>
#include <vector>
#include <array>
#include <atomic>
#include <cstdio>
#include "Timer.h"
#include "Debug.h"
#include "Ignore.h"

//const std::string SERVER_IP = "127.0.0.1";
const std::string SERVER_IP = "45.79.37.63";
static constexpr unsigned int SERVER_PORT = 41499;

using namespace AM;

int main(int argc, char* argv[])
{
    // SDL2 needs this signature for main, but we don't use the parameters.
    ignore(argc);
    ignore(argv);

    if (SDL_Init(0) == -1) {
        DebugInfo("SDL_Init: %s", SDLNet_GetError());
        return 1;
    }
    if (SDLNet_Init() == -1) {
        DebugInfo("SDLNet_Init: %s", SDLNet_GetError());
        return 2;
    }

    DebugInfo("Connecting to server.");

    IPaddress ip;
    if (SDLNet_ResolveHost(&ip, SERVER_IP.c_str(), SERVER_PORT)) {
        DebugInfo("Could not resolve host.");
        return 3;
    }

    TCPsocket serverSocket = SDLNet_TCP_Open(&ip);
    if (!serverSocket) {
        DebugInfo("Could not open serverSocket.");
        return 4;
    }
    DebugInfo("Connected.");

    /* Wait for 5s to let the connection settle. */
    Timer timer;
    timer.updateSavedTime();
    while (timer.getDeltaSeconds(false) < 5) {
        SDL_Delay(1);
    }

    /* Send the start byte, wait for the desired time, then send the end byte. */
    // Send the start byte.
    std::array<Uint8, 1> sendBuf = { 5 };
    int bytesSent = SDLNet_TCP_Send(serverSocket, &sendBuf, 1);
    if (bytesSent < 1) {
        DebugInfo("Failed to send all bytes.");
        return 5;
    }

    // Wait for the desired time.
    timer.updateSavedTime();
    while (timer.getDeltaSeconds(false) < 1200) {
        SDL_Delay(1);
    }

    // Send the end byte.
    sendBuf[0] = 6;
    bytesSent = SDLNet_TCP_Send(serverSocket, &sendBuf, 1);
    if (bytesSent < 1) {
        DebugInfo("Failed to send all bytes.");
        return 5;
    }
    DebugInfo("Sent end byte. Time passed: ~%.8f", timer.getDeltaSeconds(true));

    return 0;
}