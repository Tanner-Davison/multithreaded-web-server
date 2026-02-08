/**
 * @file Server.cpp
 * @brief Implementation of Server class
 */
#include "Server.hpp"
#include <arpa/inet.h>
#include <cstring>
#include <ctime>
#include <print>
#include <sys/socket.h>
#include <unistd.h>

Server::Server(int port, int backlog)
    : m_serverSocket(-1)
    , m_port(port)
    , m_actualPort(port)
    , m_backlog(backlog)
    , m_running(false) {
    std::memset(&m_serverAddress, 0, sizeof(m_serverAddress));
    std::srand(std::time(nullptr));
}

Server::~Server() {
    stop();
}

bool Server::start() {
    if (!createSocket()) {
        return false;
    }

    if (!bindSocket()) {
        close(m_serverSocket);
        return false;
    }

    if (!listenSocket()) {
        close(m_serverSocket);
        return false;
    }

    m_running = true;
    return true;
}

bool Server::createSocket() {
    m_serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (m_serverSocket < 0) {
        perror("socket creation failed");
        return false;
    }

    // Allow port reuse
    int opt = 1;
    if (setsockopt(m_serverSocket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        perror("setsockopt failed");
        return false;
    }

    return true;
}

bool Server::bindSocket() {
    m_serverAddress.sin_family      = AF_INET;
    m_serverAddress.sin_addr.s_addr = INADDR_ANY;
    m_serverAddress.sin_port        = htons(m_port);

    // Try binding with retry logic
    int attempts = 0;
    while (bind(m_serverSocket, (struct sockaddr*)&m_serverAddress, sizeof(m_serverAddress)) < 0) {
        if (attempts >= 10) {
            perror("bind failed after 10 attempts");
            return false;
        }

        m_actualPort             = m_port + (rand() % 10);
        m_serverAddress.sin_port = htons(m_actualPort);
        attempts++;
    }

    std::print("✓ Bound to port {}\n", m_actualPort);
    return true;
}

bool Server::listenSocket() {
    if (listen(m_serverSocket, m_backlog) < 0) {
        perror("listen failed");
        return false;
    }

    std::print("✓ Listening with backlog of {}\n", m_backlog);
    return true;
}

void Server::run() {
    std::print("Server running on port {}...\n\n", m_actualPort);

    while (m_running) {
        sockaddr_in clientAddr;
        socklen_t   clientLen = sizeof(clientAddr);

        int clientSocket = accept(m_serverSocket, (struct sockaddr*)&clientAddr, &clientLen);

        if (clientSocket < 0) {
            if (m_running) { // Only error if we're still supposed to be running
                perror("accept failed");
            }
            continue;
        }

        // Handle client (for now, in main thread - we'll add threading later)
        handleClient(clientSocket, clientAddr);
    }
}

void Server::handleClient(int clientSocket, const sockaddr_in& clientAddr) {
    // Get client info
    char clientIp[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &clientAddr.sin_addr, clientIp, INET_ADDRSTRLEN);
    int clientPort = ntohs(clientAddr.sin_port);

    std::print("✓ Client connected: {}:{}\n", clientIp, clientPort);

    // Simple HTTP response
    const char* response =
        "HTTP/1.1 200 OK\r\n"
        "Content-Type: text/plain\r\n"
        "Content-Length: 13\r\n"
        "\r\n"
        "Hello, World!";

    send(clientSocket, response, strlen(response), 0);

    close(clientSocket);
    std::print("Client disconnected\n\n");
}

void Server::stop() {
    m_running = false;
    if (m_serverSocket >= 0) {
        close(m_serverSocket);
        m_serverSocket = -1;
        std::print("Server stopped\n");
    }
}
