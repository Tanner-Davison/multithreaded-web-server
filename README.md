# IPv4 TCP Server Development Guide

### written and developed by: Tanner Davison

## Table of Contents

- [Introduction](#introduction)
- [Core Concepts](#core-concepts)
- [Socket Programming Fundamentals](#socket-programming-fundamentals)
- [Server Implementation Breakdown](#server-implementation-breakdown)
- [Your Server Architecture](#your-server-architecture)
- [Best Practices & Patterns](#best-practices--patterns)
- [Common Pitfalls & Solutions](#common-pitfalls--solutions)
- [Quick Reference](#quick-reference)

---

## Introduction

This guide explains how to build a robust IPv4 TCP server in C++. It covers the fundamental concepts, walks through my implementation, and provides best practices for production-ready servers.

### What is a TCP Server?

A TCP server is a program that:

1. **Listens** on a specific port for incoming connections
2. **Accepts** client connections
3. **Handles** data exchange with clients
4. **Manages** multiple concurrent connections

---

## Core Concepts

### The Socket

A **socket** is an endpoint for network communication. Think of it as a "phone number" for your program that allows it to send and receive data over a network.

```cpp
int socket_fd = socket(AF_INET, SOCK_STREAM, 0);
```

### All Server Domain(s) #### ( Address Families )

what kind of **addresses** the socket can communicate with:

- `AF_INET`: IPv4 Internet protocols
- `AF_INET6 IPv6`: IPv6 Internet protocols
- `AF_UNIX/AF_LOCALLocal`: Local communication (same machine)
- `AF_PACKETLow`: low-level packet interface

### Types (aka: Socket Types):

specifies the **communication** semantics;

- `SOCK_STREAM`: _TCP_- reliable, connection-based, ordered byte stream
- `SOCK_DGRAM`: _UDP_ - unreliable, connectionless datagrams
- `SOCK_RAW`: Raw network protocol access
- `SOCK_SEQPACKET`: Reliable, ordered, connection-based datagrams

## Our Implementation

- `AF_INET`: IPv4 address family
- `SOCK_STREAM`: TCP (reliable, connection-oriented)
- `0`: Default protocol (TCP for SOCK_STREAM)

### The Address Structure

The `sockaddr_in` structure defines where your server listens:

```cpp
struct sockaddr_in server_addr;
server_addr.sin_family = AF_INET;           // IPv4
server_addr.sin_addr.s_addr = INADDR_ANY;   // All network interfaces
server_addr.sin_port = htons(8080);         // Port in network byte order
```

**Why `htons()`?** Network protocols use big-endian byte order. `htons()` (host-to-network-short) converts your system's byte order to network byte order.

**Why `INADDR_ANY`?** Allows the server to accept connections on any available network interface (localhost, ethernet, wifi, etc.).

---

## Socket Programming Fundamentals

### The Server Lifecycle

```
┌─────────────────────────────────────────┐
│  1. CREATE SOCKET                       │
│     socket() → file descriptor          │
└──────────────┬──────────────────────────┘
               │
┌──────────────▼──────────────────────────┐
│  2. BIND TO ADDRESS                     │
│     bind() → associate socket with port │
└──────────────┬──────────────────────────┘
               │
┌──────────────▼──────────────────────────┐
│  3. LISTEN FOR CONNECTIONS              │
│     listen() → mark as passive socket   │
└──────────────┬──────────────────────────┘
               │
┌──────────────▼──────────────────────────┐
│  4. ACCEPT CONNECTIONS (loop)           │
│     accept() → create new socket/client │
└──────────────┬──────────────────────────┘
               │
┌──────────────▼──────────────────────────┐
│  5. HANDLE CLIENT                       │
│     read()/write() → communicate        │
└──────────────┬──────────────────────────┘
               │
┌──────────────▼──────────────────────────┐
│  6. CLOSE CONNECTION                    │
│     close() → cleanup resources         │
└─────────────────────────────────────────┘
```

### Step-by-Step Explanation

#### 1. **Create Socket**

```cpp
int server_fd = socket(AF_INET, SOCK_STREAM, 0);
if (server_fd < 0) {
    perror("socket failed");
    exit(EXIT_FAILURE);
}
```

- Creates a socket file descriptor
- Returns -1 on failure
- The socket is not yet associated with any address

#### 2. **Bind to Address**

```cpp
struct sockaddr_in address;
address.sin_family = AF_INET;
address.sin_addr.s_addr = INADDR_ANY;
address.sin_port = htons(PORT);

if (bind(server_fd, (struct sockaddr*)&address, sizeof(address)) < 0) {
    perror("bind failed");
    exit(EXIT_FAILURE);
}
```

- Associates socket with specific IP:port
- `INADDR_ANY` (0.0.0.0) means "accept on all interfaces"
- Can fail if port is already in use or privileged (<1024 without root)

#### 3. **Listen for Connections**

```cpp
if (listen(server_fd, BACKLOG) < 0) {
    perror("listen failed");
    exit(EXIT_FAILURE);
}
```

- Marks socket as passive (ready to accept connections)
- `BACKLOG`: Maximum pending connections queue (typically 5-128)
- Connections beyond backlog are refused

#### 4. **Accept Connections**

```cpp
struct sockaddr_in client_addr;
socklen_t client_len = sizeof(client_addr);

int client_fd = accept(server_fd,
                      (struct sockaddr*)&client_addr,
                      &client_len);
if (client_fd < 0) {
    perror("accept failed");
    // Handle error but keep server running
}
```

- **Blocks** until a client connects
- Returns new socket for that specific client
- Original `server_fd` continues listening

#### 5. **Handle Client Communication**

```cpp
char buffer[1024] = {0};
ssize_t bytes_read = read(client_fd, buffer, sizeof(buffer));
if (bytes_read > 0) {
    // Process data
    const char* response = "HTTP/1.1 200 OK\r\n\r\nHello World";
    write(client_fd, response, strlen(response));
}
```

#### 6. **Close Connection**

```cpp
close(client_fd);  // Close client socket
// Server socket stays open for new connections
```

---

## Server Implementation Breakdown

### The Basic Server Loop

```cpp
// 1. Create and configure socket
int server_fd = socket(AF_INET, SOCK_STREAM, 0);

// 2. Set socket options
int opt = 1;
setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

// 3. Bind to address
struct sockaddr_in address;
address.sin_family = AF_INET;
address.sin_addr.s_addr = INADDR_ANY;
address.sin_port = htons(8080);
bind(server_fd, (struct sockaddr*)&address, sizeof(address));

// 4. Listen
listen(server_fd, 10);

// 5. Accept loop
while (running) {
    struct sockaddr_in client_addr;
    socklen_t len = sizeof(client_addr);
    int client_fd = accept(server_fd, (struct sockaddr*)&client_addr, &len);

    if (client_fd >= 0) {
        handleClient(client_fd, client_addr);
        close(client_fd);
    }
}

// 6. Cleanup
close(server_fd);
```

---

## Your Server Architecture

### Class Structure

```cpp
class Server {
private:
    int         m_serverSocket;   // Main listening socket
    int         m_port;           // Requested port
    int         m_actualPort;     // Port actually bound to
    int         m_backlog;        // Connection queue size
    bool        m_running;        // Server state flag
    sockaddr_in m_serverAddress;  // Server address config

public:
    Server(int port, int backlog = 10);
    ~Server();

    bool start();    // Initialize and bind
    void run();      // Main accept loop
    void stop();     // Graceful shutdown
};
```

### Why This Design?

**Encapsulation**: Socket management is hidden inside the class. Users only need:

```cpp
Server server(8080);
server.start();
server.run();
```

**RAII Pattern**: The destructor ensures socket cleanup even if exceptions occur.

**State Management**: `m_running` flag allows graceful shutdown via `stop()`.

### Implementation Flow

```cpp
// main.cpp
Server server(8080);        // 1. Construct
if (!server.start()) {      // 2. Initialize socket, bind, listen
    return EXIT_FAILURE;
}
server.run();               // 3. Accept loop (blocks here)
```

Inside `start()`:

```cpp
bool Server::start() {
    if (!createSocket()) return false;   // socket()
    if (!bindSocket())   return false;   // bind() with retry
    if (!listenSocket()) return false;   // listen()
    m_running = true;
    return true;
}
```

Inside `run()`:

```cpp
void Server::run() {
    while (m_running) {
        sockaddr_in client_addr;
        socklen_t len = sizeof(client_addr);

        int client_fd = accept(m_serverSocket,
                              (struct sockaddr*)&client_addr,
                              &len);

        if (client_fd < 0) continue; // Error handling

        handleClient(client_fd, client_addr); // Process request
        close(client_fd);                      // Cleanup
    }
}
```

---

## Best Practices & Patterns

### 1. Socket Options: SO_REUSEADDR & SO_REUSEPORT

**Problem**: After server crashes or restart, `bind()` fails with "Address already in use".

**Cause**: Socket in `TIME_WAIT` state (TCP's 2-minute cooldown after close).

**Solution**: Set `SO_REUSEADDR` before binding:

```cpp
bool Server::createSocket() {
    m_serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (m_serverSocket < 0) return false;

    // Allow immediate reuse of address
    int opt = 1;
    if (setsockopt(m_serverSocket, SOL_SOCKET, SO_REUSEADDR,
                   &opt, sizeof(opt)) < 0) {
        perror("setsockopt SO_REUSEADDR");
        return false;
    }

    return true;
}
```

**When to Use Each:**

| Option         | Purpose                       | Use Case                        |
| -------------- | ----------------------------- | ------------------------------- |
| `SO_REUSEADDR` | Reuse address in TIME_WAIT    | Server restart without waiting  |
| `SO_REUSEPORT` | Multiple sockets on same port | Load balancing across processes |

**SO_REUSEPORT Example** (Linux 3.9+):

```cpp
int opt = 1;
setsockopt(sock, SOL_SOCKET, SO_REUSEPORT, &opt, sizeof(opt));
// Now multiple server processes can bind to same port
// Kernel load-balances incoming connections
```

### 2. Error Handling Strategies

**Always check return values:**

```cpp
// BAD - ignores errors
socket(AF_INET, SOCK_STREAM, 0);
bind(server_fd, ...);

// GOOD - handles each step
int server_fd = socket(AF_INET, SOCK_STREAM, 0);
if (server_fd < 0) {
    std::cerr << "Socket creation failed: " << strerror(errno) << "\n";
    return false;
}

if (bind(server_fd, ...) < 0) {
    std::cerr << "Bind failed: " << strerror(errno) << "\n";
    close(server_fd);
    return false;
}
```

**Handle `accept()` errors gracefully:**

```cpp
int client_fd = accept(server_fd, ...);
if (client_fd < 0) {
    if (errno == EINTR) {
        // Interrupted by signal, retry
        continue;
    }
    perror("accept failed");
    continue; // Keep server running
}
```

### 3. Bind with Retry Logic

Ports may be transiently unavailable. Your code should implement:

```cpp
bool Server::bindSocket() {
    const int MAX_RETRIES = 5;
    const int RETRY_DELAY_SEC = 2;

    for (int attempt = 0; attempt < MAX_RETRIES; ++attempt) {
        if (bind(m_serverSocket,
                (struct sockaddr*)&m_serverAddress,
                sizeof(m_serverAddress)) == 0) {

            // Get actual port (useful if port 0 was requested)
            socklen_t len = sizeof(m_serverAddress);
            getsockname(m_serverSocket,
                       (struct sockaddr*)&m_serverAddress,
                       &len);
            m_actualPort = ntohs(m_serverAddress.sin_port);
            return true;
        }

        if (attempt < MAX_RETRIES - 1) {
            std::cerr << "Bind attempt " << (attempt + 1)
                     << " failed, retrying in " << RETRY_DELAY_SEC
                     << "s...\n";
            sleep(RETRY_DELAY_SEC);
        }
    }

    std::cerr << "Failed to bind after " << MAX_RETRIES << " attempts\n";
    return false;
}
```

### 4. Proper Resource Cleanup (RAII)

```cpp
Server::~Server() {
    stop();

    if (m_serverSocket >= 0) {
        shutdown(m_serverSocket, SHUT_RDWR); // Stop ongoing I/O
        close(m_serverSocket);                // Release socket
        m_serverSocket = -1;
    }
}
```

**Why `shutdown()` before `close()`?**

- `shutdown()`: Stops read/write operations immediately
- `close()`: Decrements reference count, socket may linger

### 5. Signal Handling for Graceful Shutdown

```cpp
#include <csignal>

// Global pointer for signal handler access
Server* g_server = nullptr;

void signalHandler(int signum) {
    if (g_server) {
        std::cout << "\nShutting down gracefully...\n";
        g_server->stop();
    }
}

int main() {
    Server server(8080);
    g_server = &server;

    // Register signal handlers
    signal(SIGINT, signalHandler);   // Ctrl+C
    signal(SIGTERM, signalHandler);  // kill command

    server.start();
    server.run();

    g_server = nullptr;
    return 0;
}
```

### 6. Concurrency Patterns

Your server currently handles one client at a time (blocking). For production:

**Option A: Thread-per-Connection**

```cpp
void Server::run() {
    while (m_running) {
        int client_fd = accept(m_serverSocket, ...);
        if (client_fd < 0) continue;

        // Create new thread for each client
        std::thread client_thread([this, client_fd, client_addr]() {
            handleClient(client_fd, client_addr);
            close(client_fd);
        });
        client_thread.detach(); // Or store and join later
    }
}
```

**Pros**: Simple, each client isolated  
**Cons**: Thread overhead, scalability limited

**Option B: Thread Pool**

```cpp
#include <queue>
#include <condition_variable>

class ThreadPool {
    std::vector<std::thread> workers;
    std::queue<std::function<void()>> tasks;
    std::mutex queue_mutex;
    std::condition_variable cv;
    bool stop = false;

public:
    ThreadPool(size_t threads) {
        for (size_t i = 0; i < threads; ++i) {
            workers.emplace_back([this] {
                while (true) {
                    std::function<void()> task;
                    {
                        std::unique_lock<std::mutex> lock(queue_mutex);
                        cv.wait(lock, [this] {
                            return stop || !tasks.empty();
                        });
                        if (stop && tasks.empty()) return;
                        task = std::move(tasks.front());
                        tasks.pop();
                    }
                    task();
                }
            });
        }
    }

    void enqueue(std::function<void()> task) {
        {
            std::unique_lock<std::mutex> lock(queue_mutex);
            tasks.push(std::move(task));
        }
        cv.notify_one();
    }
};
```

Usage:

```cpp
ThreadPool pool(10); // 10 worker threads

void Server::run() {
    while (m_running) {
        int client_fd = accept(m_serverSocket, ...);
        if (client_fd < 0) continue;

        pool.enqueue([this, client_fd, client_addr]() {
            handleClient(client_fd, client_addr);
            close(client_fd);
        });
    }
}
```

**Pros**: Controlled resource usage, better scalability  
**Cons**: More complex implementation

**Option C: Non-blocking I/O with epoll/select**

```cpp
#include <sys/epoll.h>

int epoll_fd = epoll_create1(0);
struct epoll_event event, events[MAX_EVENTS];

event.events = EPOLLIN;
event.data.fd = m_serverSocket;
epoll_ctl(epoll_fd, EPOLL_CTL_ADD, m_serverSocket, &event);

while (m_running) {
    int n = epoll_wait(epoll_fd, events, MAX_EVENTS, -1);

    for (int i = 0; i < n; ++i) {
        if (events[i].data.fd == m_serverSocket) {
            // New connection
            int client_fd = accept(m_serverSocket, ...);
            // Add to epoll for monitoring
        } else {
            // Existing client has data
            handleClient(events[i].data.fd, ...);
        }
    }
}
```

**Pros**: Single-threaded, handles thousands of connections  
**Cons**: Most complex, requires non-blocking socket configuration

---

## Common Pitfalls & Solutions

### 1. **Forgetting Network Byte Order**

```cpp
// WRONG
server_addr.sin_port = 8080;

// CORRECT
server_addr.sin_port = htons(8080); // Host to Network Short
```

### 2. **Not Closing Client Sockets**

```cpp
// Memory leak - socket file descriptors exhaust
while (true) {
    int client = accept(server_fd, ...);
    handleClient(client);
    // FORGOT: close(client);
}

// CORRECT
int client = accept(server_fd, ...);
handleClient(client);
close(client); // Always close
```

### 3. **Ignoring Partial Reads/Writes**

```cpp
// WRONG - read() may return less than buffer size
char buffer[1024];
read(client_fd, buffer, sizeof(buffer));
// What if only 100 bytes were read?

// CORRECT - loop until complete
ssize_t total = 0;
ssize_t expected = 1024;
while (total < expected) {
    ssize_t n = read(client_fd, buffer + total, expected - total);
    if (n <= 0) break; // Error or EOF
    total += n;
}
```

### 4. **Buffer Overflows**

```cpp
// DANGEROUS
char buffer[256];
read(client_fd, buffer, 1024); // Can write beyond buffer!

// SAFE
char buffer[256];
ssize_t n = read(client_fd, buffer, sizeof(buffer) - 1);
if (n > 0) {
    buffer[n] = '\0'; // Null-terminate
}
```

### 5. **Not Handling SIGPIPE**

When a client disconnects, writing to socket causes `SIGPIPE`, killing your server.

```cpp
// Ignore SIGPIPE globally
signal(SIGPIPE, SIG_IGN);

// Or handle per-write
ssize_t n = send(client_fd, data, len, MSG_NOSIGNAL);
if (n < 0) {
    if (errno == EPIPE) {
        // Client disconnected
    }
}
```

---

## Quick Reference

### Essential Headers

```cpp
#include <sys/socket.h>  // socket, bind, listen, accept
#include <netinet/in.h>  // sockaddr_in, INADDR_ANY
#include <arpa/inet.h>   // htons, ntohs, inet_ntoa
#include <unistd.h>      // close, read, write
#include <cstring>       // memset
#include <cerrno>        // errno
```

### Key Functions

| Function     | Purpose                       | Returns                    |
| ------------ | ----------------------------- | -------------------------- |
| `socket()`   | Create socket                 | File descriptor or -1      |
| `bind()`     | Associate socket with address | 0 or -1                    |
| `listen()`   | Mark socket as passive        | 0 or -1                    |
| `accept()`   | Accept incoming connection    | New socket FD or -1        |
| `read()`     | Receive data                  | Bytes read, 0 (EOF), or -1 |
| `write()`    | Send data                     | Bytes written or -1        |
| `close()`    | Close socket                  | 0 or -1                    |
| `shutdown()` | Stop I/O operations           | 0 or -1                    |

### Socket Options

```cpp
int opt = 1;

// Reuse address (essential for server restart)
setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

// Reuse port (load balancing)
setsockopt(sock, SOL_SOCKET, SO_REUSEPORT, &opt, sizeof(opt));

// Set receive timeout
struct timeval timeout;
timeout.tv_sec = 5;
timeout.tv_usec = 0;
setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));

// Keep-alive
setsockopt(sock, SOL_SOCKET, SO_KEEPALIVE, &opt, sizeof(opt));
```

### Error Checking Pattern

```cpp
if (result < 0) {
    std::cerr << "Operation failed: " << strerror(errno) << "\n";
    // Cleanup and handle error
}
```

### Getting Client Info

```cpp
struct sockaddr_in client_addr;
socklen_t len = sizeof(client_addr);
int client_fd = accept(server_fd, (struct sockaddr*)&client_addr, &len);

// Convert IP to string
char client_ip[INET_ADDRSTRLEN];
inet_ntop(AF_INET, &client_addr.sin_addr, client_ip, sizeof(client_ip));

// Get port
int client_port = ntohs(client_addr.sin_port);

std::cout << "Client connected from " << client_ip
          << ":" << client_port << "\n";
```

### Compile Command

```bash
# Basic compilation
g++ -std=c++23 -o server src/main.cpp src/Server.cpp -I include

# With warnings and debug symbols
g++ -std=c++23 -Wall -Wextra -g -o server src/main.cpp src/Server.cpp -I include

# With pthread for multithreading
g++ -std=c++23 -Wall -Wextra -pthread -o server src/main.cpp src/Server.cpp -I include
```

---

## Debugging Tips

### 1. Test with netcat

```bash
# Start your server
./server

# In another terminal, connect as client
nc localhost 8080
# Type messages and press Enter
```

### 2. Check what's listening

```bash
# See all listening sockets
netstat -tuln | grep 8080

# Or with ss (faster)
ss -tuln | grep 8080
```

### 3. Monitor connections

```bash
# See established connections
netstat -antp | grep 8080
```

### 4. Use strace

```bash
# See all system calls
strace ./server

# Filter for socket operations
strace -e socket,bind,listen,accept ./server
```

### 5. Check for port already in use

```bash
# Find process using port 8080
lsof -i :8080

# Kill it if needed
kill -9 <PID>
```

---

## Your Implementation Checklist

Based on your code, here's what to implement or verify:

- [x] Socket creation
- [x] Bind with retry logic
- [x] Listen for connections
- [x] Accept loop
- [x] RAII cleanup (destructor)
- [x] Error handling structure
- [x] Port management (m_actualPort)
- [ ] **Add `SO_REUSEADDR` in `createSocket()`**
- [ ] **Implement signal handling (SIGINT, SIGTERM)**
- [ ] **Add threading/async handling in `run()`**
- [ ] **Implement `handleClient()` logic**
- [ ] **Add logging/metrics**
- [ ] **Handle `SIGPIPE`**
- [ ] **Add graceful shutdown in `stop()`**

---

## Next Steps

1. **Implement SO_REUSEADDR**: Add to `createSocket()` method
2. **Build handleClient()**: Define your protocol (HTTP? Custom?)
3. **Add Concurrency**: Choose thread-pool or async I/O
4. **Error Logging**: Replace prints with proper logging framework
5. **Testing**: Write unit tests for edge cases
6. **Security**: Add request validation, rate limiting

---

## Further Reading

- **Beej's Guide to Network Programming**: Classic resource for socket programming
- **Unix Network Programming** by W. Richard Stevens: The definitive book
- **man pages**: `man 2 socket`, `man 2 bind`, `man 7 socket`
- **RFC 793**: TCP specification

---

_This documentation is based on your Server class implementation and current best practices as of 2026._
