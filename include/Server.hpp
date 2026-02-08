/**
 * @file Server.hpp
 * @brief TCP Server class for handling incoming connections
 */
#ifndef SERVER_HPP
#define SERVER_HPP

#include <netinet/in.h>
#include <string>

class Server {
  public:
    /**
     * @brief Construct a server on specified port
     * @param port Port number to listen on
     * @param backlog Maximum pending connections (default: 10)
     */
    explicit Server(int port, int backlog = 10);

    /**
     * @brief Destructor - cleans up socket resources
     */
    ~Server();

    // Prevent copying
    Server(const Server&)            = delete;
    Server& operator=(const Server&) = delete;

    /**
     * @brief Initialize and bind the server socket
     * @return true if successful, false otherwise
     */
    bool start();

    /**
     * @brief Main server loop - accepts and handles connections
     */
    void run();

    /**
     * @brief Stop the server gracefully
     */
    void stop();

    /**
     * @brief Get the port the server is actually bound to
     * @return Port number
     */
    int getPort() const {
        return m_actualPort;
    }

  private:
    /**
     * @brief Create and configure the server socket
     * @return true if successful
     */
    bool createSocket();

    /**
     * @brief Bind socket to address with retry logic
     * @return true if successful
     */
    bool bindSocket();

    /**
     * @brief Start listening for connections
     * @return true if successful
     */
    bool listenSocket();

    /**
     * @brief Handle a connected client
     * @param clientSocket File descriptor for client connection
     * @param clientAddr Client's address information
     */
    void handleClient(int clientSocket, const sockaddr_in& clientAddr);

    int         m_serverSocket;  ///< Server socket file descriptor
    int         m_port;          ///< Requested port
    int         m_actualPort;    ///< Actual bound port
    int         m_backlog;       ///< Connection backlog size
    bool        m_running;       ///< Server running state
    sockaddr_in m_serverAddress; ///< Server address structure
};

#endif // SERVER_HPP
