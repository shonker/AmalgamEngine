#pragma once

#include <SDL_stdinc.h>
#include <memory>
#include <string>

// Forward declaration
struct _TCPsocket;
typedef struct _TCPsocket* TCPsocket;

namespace AM
{
/**
 * Represents a single TCP socket.
 * Wraps SDLNet's TCPsocket in a C++ object interface.
 */
class TcpSocket
{
public:
    TcpSocket();

    /**
     * Takes ownership over the given SDLNet socket connection.
     *
     * @param inSdlSocket A connected socket.
     */
    TcpSocket(TCPsocket inSdlSocket);

    // Moveable.
    TcpSocket(TcpSocket&& otherSocket) noexcept;

    // Not copyable.
    TcpSocket(const TcpSocket& otherSocket) = delete;
    TcpSocket& operator=(const TcpSocket& otherSocket) = delete;

    /**
     * Closes this socket, if it's open.
     * Note: Will not remove the socket from any sets that it might belong to.
     */
    ~TcpSocket();

    /**
     * Opens this socket as a listener.
     *
     * @param portToListenOn  The port to listen on.
     * @return true if successful, else false.
     */
    bool openAsListener(Uint16 portToListenOn);

    /**
     * Opens a socket connection to the given host.
     *
     * @param ip  The IP to connect to.
     * @param port  The port to connect to.
     * @return true if successful, else false.
     */
    bool openConnectionTo(std::string ip, Uint16 port);

    /**
     * Closes this socket, if it's open.
     * Note: Will not remove this socket from any sets that it might belong to.
     */
    void close();

    /**
     * @return true if this socket has an open connection, else false.
     */
    bool isOpen();

    /**
     * @return true if this socket has been marked as active, else false.
     *
     * Note: Only call this on a socket in a set, after calling checkSockets()
     *       on that set.
     */
    bool isReady();

    /**
     * Sends len bytes from the given dataBuffer over this socket.
     * @return The number of bytes sent. If the number returned is less than
     *         len, an error occurred, such as the client disconnecting.
     */
    int send(const void* dataBuffer, int len);

    /**
     * Receives data from this socket.
     *
     * Note: This call will wait indefinitely for bytes to be available on this
     *       socket. Once bytes are available, it will immediately return them,
     *       even if the amount available is less than maxLen.
     *
     * Note: This function is not used for server (listener) sockets.
     *
     * @param dataBuffer  The buffer to use.
     * @param maxLen  The maximum number of bytes to receive.
     * @return The number of bytes received.
     *         If the number returned is <= 0, an error occurred, or the remote
     *         host has closed the connection.
     */
    int receive(void* dataBuffer, int maxLen);

    /**
     * Accepts an incoming connection on this socket.
     *
     * Note: Only call this on a server (listener) socket.
     *
     * @return A TcpSocket. If the accept was successful, socket.isOpen() will
     *         == true. The socket will be closed if an error occurs, such as
     *         failure to create a socket, failure to finish connecting, or if
     *         there is no waiting connection.
     */
    TcpSocket accept();

    /**
     * Gets the address of the peer at the other side of this socket.
     *
     * Note: Only call this on a connected socket, not a listener.
     *
     * @return An address in the form "IP:port".
     */
    std::string getAddress();

    /**
     * Returns the transport library's underlying socket type.
     */
    TCPsocket getUnderlyingSocket() const;

private:
    TCPsocket socket;

    /** This socket's IP. Empty if this is a listener socket. */
    std::string ip;

    /** This socket's port. */
    Uint16 port;
};

} // End namespace AM
