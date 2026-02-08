/**
 * @file  main.cpp
 * @brief Entry point for multithreaded web server
 * @author Tanner Davison
 * @date 2026-02-07
 */
#include "Server.hpp"
#include <cstdlib>
#include <print>

int main() {
    try {
        Server server(8080);

        if (!server.start()) {
            std::print(stderr, "Failed to start server\n");
            return EXIT_FAILURE;
        }

        std::print("Server started successfully\n");
        std::print("Press Ctrl+C to stop\n\n");

        server.run(); // Blocks here, accepting connections

    } catch (const std::exception& e) {
        std::print(stderr, "Server error: {}\n", e.what());
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
