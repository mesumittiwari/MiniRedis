# Mini-Redis

<p align="center">
  <strong>A Persistent Multithreaded In-Memory Key-Value Store in C++</strong>
</p>

<p align="center">
  A lightweight Redis-inspired TCP server built from scratch using C++17,
  POSIX sockets, multithreading, synchronization, snapshot persistence,
  Docker, and GitHub Actions.
</p>

<p align="center">
  <a href="https://github.com/mesumittiwari/MiniRedis">GitHub Repository</a>
</p>

---

## Table of Contents

- [Overview](#overview)
- [Features](#features)
- [Architecture](#architecture)
- [Project Structure](#project-structure)
- [Technology Stack](#technology-stack)
- [How It Works](#how-it-works)
  - [Server Lifecycle](#server-lifecycle)
  - [Client Connection Lifecycle](#client-connection-lifecycle)
  - [Command Processing](#command-processing)
- [Supported Commands](#supported-commands)
  - [SET](#set)
  - [GET](#get)
  - [DEL](#del)
  - [EXISTS](#exists)
  - [EXPIRE](#expire)
  - [TTL](#ttl)
  - [KEYS](#keys)
  - [QUIT / EXIT](#quit--exit)
- [Data Model](#data-model)
- [Concurrency Model](#concurrency-model)
- [Thread Safety](#thread-safety)
- [Locking Strategy](#locking-strategy)
- [Lazy Expiration](#lazy-expiration)
- [Persistence](#persistence)
- [Persistence Format](#persistence-format)
- [Graceful Shutdown](#graceful-shutdown)
- [Networking](#networking)
- [Protocol](#protocol)
- [Building Locally](#building-locally)
- [Running Locally](#running-locally)
- [Testing With Netcat](#testing-with-netcat)
- [Testing TTL](#testing-ttl)
- [Testing Persistence](#testing-persistence)
- [Docker](#docker)
- [Docker Compose](#docker-compose)
- [Persistent Storage](#persistent-storage)
- [CI/CD](#cicd)
- [Design Decisions](#design-decisions)
- [Trade-offs](#trade-offs)
- [Limitations](#limitations)
- [Future Improvements](#future-improvements)
- [Learning Outcomes](#learning-outcomes)
- [Project Roadmap](#project-roadmap)
- [Build and Run Cheat Sheet](#build-and-run-cheat-sheet)
- [Author](#author)
- [License](#license)

---

# Overview

**Mini-Redis** is a lightweight Redis-inspired in-memory key-value store implemented from scratch in C++17.

The project provides a standalone TCP server that allows multiple clients to connect concurrently and perform basic key-value operations such as storing, retrieving, deleting, and checking keys.

Unlike a simple command-line key-value store, Mini-Redis introduces several systems-programming concepts:

- TCP networking
- Client/server architecture
- Concurrent client handling
- Thread synchronization
- Shared-memory access
- Key expiration
- Disk persistence
- Graceful shutdown
- Docker containerization
- Continuous integration

The goal of the project is not to reproduce the complete Redis implementation.

Instead, Mini-Redis focuses on understanding the fundamental engineering concepts involved in building a small networked in-memory database from first principles.

---

# Features

## Core Storage

- In-memory key-value storage
- `std::unordered_map` based implementation
- String keys and values
- Basic CRUD-style operations

## Networking

- POSIX TCP sockets
- Server listens on port `6380`
- Multiple clients can connect simultaneously
- Line-oriented command interface
- Client/server communication over TCP

## Concurrency

- Thread-per-client architecture
- `std::thread` for concurrent client handling
- `std::shared_mutex` for shared store synchronization
- Thread-safe access to the underlying data structure

## Key Expiration

- Per-key TTL
- `EXPIRE` command
- `TTL` command
- Lazy deletion of expired keys
- Expiration state restored from snapshots

## Persistence

- Disk snapshotting
- Snapshot loaded during startup
- Snapshot written during graceful shutdown
- Expiration metadata persisted
- Expired entries skipped during restoration

## Deployment

- Multi-stage Docker build
- Docker Compose configuration
- Persistent volume mapping
- GitHub Actions CI workflow

---

# Architecture

Mini-Redis follows a simple layered architecture.

```text
                         ┌──────────────────────┐
                         │        Client        │
                         │                      │
                         │  netcat / TCP client │
                         └──────────┬───────────┘
                                    │
                                    │ TCP
                                    ▼
                         ┌──────────────────────┐
                         │      TCP Server      │
                         │                      │
                         │ socket()             │
                         │ bind()               │
                         │ listen()             │
                         │ accept()             │
                         └──────────┬───────────┘
                                    │
                                    │ client socket
                                    ▼
                         ┌──────────────────────┐
                         │    Client Thread     │
                         │                      │
                         │ recv()               │
                         │ parse command        │
                         │ execute command      │
                         │ send()               │
                         └──────────┬───────────┘
                                    │
                                    ▼
                         ┌──────────────────────┐
                         │        Store         │
                         │                      │
                         │ unordered_map        │
                         │ shared_mutex         │
                         │ TTL / expiration     │
                         └──────────┬───────────┘
                                    │
                                    │ graceful shutdown
                                    ▼
                         ┌──────────────────────┐
                         │   Snapshot File      │
                         │                      │
                         │ miniredis_dump.txt   │
                         └──────────────────────┘
```

The system can be viewed as two primary components:

```text
Networking Layer
       │
       ▼
   TCP Server
       │
       ▼
Command Handling
       │
       ▼
 Storage Layer
       │
       ▼
    Store
```

The networking layer is responsible for accepting connections and communicating with clients.

The storage layer is responsible for maintaining data, enforcing synchronization, handling expiration, and managing persistence.

---

# Project Structure

```text
MiniRedis/
│
├── main.cpp
├── store.cpp
├── store.h
│
├── Dockerfile
├── docker-compose.yml
├── .gitignore
│
├── .github/
│   └── workflows/
│       └── ci.yml
│
└── data/
    └── miniredis_dump.txt
```

## File Responsibilities

### `main.cpp`

Contains the server-side functionality:

- TCP socket creation
- Address binding
- Listening for connections
- Client acceptance
- Client thread creation
- Command parsing
- Command dispatch
- Response handling
- Server lifecycle management

### `store.h`

Defines the storage abstraction.

The `Store` class owns the in-memory data and exposes operations for:

- setting values
- getting values
- deleting keys
- checking key existence
- configuring expiration
- retrieving TTL
- listing keys
- saving snapshots
- loading snapshots

### `store.cpp`

Contains the implementation of the `Store` class.

It handles:

- `unordered_map` operations
- synchronization
- lazy expiration
- TTL calculations
- snapshot serialization
- snapshot restoration

### `Dockerfile`

Provides a multi-stage Docker build.

The first stage compiles the application.

The runtime stage contains the resulting executable and required runtime environment.

### `docker-compose.yml`

Provides a convenient containerized deployment configuration with:

- port mapping
- persistent storage
- container lifecycle management

### `.github/workflows/ci.yml`

Provides automated CI validation for:

- C++ compilation
- Docker image creation
- Docker image verification

---

# Technology Stack

| Technology | Purpose |
|---|---|
| C++17 | Core implementation |
| `std::unordered_map` | In-memory key-value storage |
| POSIX Sockets | TCP networking |
| `std::thread` | Concurrent client handling |
| `std::shared_mutex` | Thread synchronization |
| `std::chrono` | TTL and expiration management |
| File Streams | Snapshot persistence |
| Linux/POSIX | Runtime environment |
| Docker | Containerization |
| Docker Compose | Container orchestration |
| Bash | Development and operational commands |
| GitHub Actions | Continuous integration |

---

# How It Works

## Server Lifecycle

The server follows the traditional TCP server lifecycle:

```text
                    ┌──────────────┐
                    │    Start     │
                    └──────┬───────┘
                           │
                           ▼
                    Load Snapshot
                           │
                           ▼
                     socket()
                           │
                           ▼
                       bind()
                           │
                           ▼
                      listen()
                           │
                           ▼
                    ┌──────────────┐
                    │    accept()  │◄─────────┐
                    └──────┬───────┘          │
                           │                  │
                           ▼                  │
                    Create Thread             │
                           │                  │
                           ▼                  │
                    Handle Client             │
                           │                  │
                           └──────────────────┘
```

At startup, the server attempts to restore previously persisted state.

It then creates a TCP socket, binds it to port `6380`, starts listening, and continuously accepts incoming connections.

---

# Client Connection Lifecycle

For every accepted connection:

```text
Client
   │
   │ connect()
   ▼
Server
   │
   │ accept()
   ▼
Client Socket
   │
   │ create thread
   ▼
Client Handler
   │
   ├── recv()
   ├── parse
   ├── execute
   ├── send()
   │
   └── repeat
```

The client thread continues processing commands until:

- the client disconnects
- the client sends `QUIT`
- the client sends `EXIT`
- an I/O error occurs

---

# Command Processing

Commands follow a simple text-based structure:

```text
COMMAND ARGUMENTS
```

For example:

```text
SET name Sumit
```

The server:

1. Receives the command.
2. Parses the command into tokens.
3. Normalizes the command name.
4. Validates the number of arguments.
5. Calls the appropriate `Store` operation.
6. Generates a response.
7. Sends the response to the client.

Conceptually:

```text
TCP Input
    │
    ▼
Receive Command
    │
    ▼
Parse
    │
    ▼
Validate
    │
    ▼
Dispatch
    │
    ├── SET
    ├── GET
    ├── DEL
    ├── EXISTS
    ├── EXPIRE
    ├── TTL
    └── KEYS
    │
    ▼
Store Operation
    │
    ▼
Response
    │
    ▼
TCP Client
```

---

# Supported Commands

## SET

Stores a value against a key.

### Syntax

```text
SET <key> <value>
```

### Example

```text
SET name Sumit
```

Response:

```text
+OK
```

---

## GET

Retrieves the value associated with a key.

### Syntax

```text
GET <key>
```

### Example

```text
SET name Sumit
GET name
```

Response:

```text
Sumit
```

If the key does not exist:

```text
(nil)
```

---

## DEL

Deletes a key from the store.

### Syntax

```text
DEL <key>
```

### Example

```text
SET name Sumit
DEL name
```

Response:

```text
1
```

If the key does not exist, the command returns:

```text
0
```

---

## EXISTS

Checks whether a key currently exists.

### Syntax

```text
EXISTS <key>
```

### Example

```text
SET name Sumit
EXISTS name
```

Response:

```text
1
```

For a missing key:

```text
0
```

Expired keys are treated as non-existent.

---

## EXPIRE

Sets a time-to-live for a key.

### Syntax

```text
EXPIRE <key> <seconds>
```

### Example

```text
SET session abc123
EXPIRE session 60
```

Response:

```text
1
```

The key will become logically expired after the specified duration.

---

## TTL

Returns the remaining lifetime of a key.

### Syntax

```text
TTL <key>
```

### Example

```text
SET session abc123
EXPIRE session 60
TTL session
```

Example response:

```text
59
```

The implementation uses the following return values:

```text
-2  → key does not exist
-1  → key exists but has no expiration
>=0 → remaining TTL in seconds
```

---

## KEYS

Lists keys currently present in the store.

### Syntax

```text
KEYS
```

### Example

```text
SET name Sumit
SET language C++
KEYS
```

Possible response:

```text
language
name
```

Expired entries encountered during the operation are removed lazily.

---

## QUIT / EXIT

Closes the current client connection.

### Example

```text
QUIT
```

or:

```text
EXIT
```

The server closes the connection without shutting down the entire server.

---

# Data Model

Mini-Redis stores each key using a `CacheItem`.

Conceptually:

```cpp
struct CacheItem {
    std::string value;
    bool has_expiry;
    TimePoint expiry_time;
};
```

The primary container is:

```cpp
std::unordered_map<std::string, CacheItem> data;
```

This allows each key to contain:

```text
Key
 │
 ├── Value
 │
 ├── Has Expiry?
 │
 └── Expiry Timestamp
```

For example:

```text
"name"
   │
   ├── value: "Sumit"
   └── expiry: none


"session"
   │
   ├── value: "abc123"
   └── expiry: timestamp
```

---

# Concurrency Model

Mini-Redis uses a **thread-per-client** concurrency model.

Instead of processing all clients sequentially:

```text
Client A ──┐
Client B ──┼──► Single Server Thread
Client C ──┘
```

the server creates a separate thread for each client:

```text
Client A ─────► Thread A
Client B ─────► Thread B
Client C ─────► Thread C
Client D ─────► Thread D
```

All of these threads share the same `Store` instance.

Therefore, access to the shared store must be synchronized.

---

# Thread Safety

The underlying `std::unordered_map` is shared by multiple client threads.

Concurrent access without synchronization could result in:

- data races
- undefined behavior
- container corruption
- inconsistent reads
- crashes

Mini-Redis protects the shared store using:

```cpp
std::shared_mutex rw_lock;
```

Operations acquire the appropriate lock before accessing shared state.

---

# Locking Strategy

Mini-Redis uses `std::unique_lock` for operations that require exclusive access.

Conceptually:

```cpp
std::unique_lock<std::shared_mutex> lock(rw_lock);
```

This provides mutual exclusion between concurrent operations that access or modify the store.

Operations such as:

```text
SET
DEL
EXPIRE
```

naturally require exclusive access.

However, operations such as:

```text
GET
EXISTS
TTL
```

can also modify the map because of lazy expiration.

For example:

```text
GET expired_key
        │
        ▼
Check expiration
        │
        ├───────────────┐
        │               │
    Not expired       Expired
        │               │
        ▼               ▼
 Return value       Erase key
                        │
                        ▼
                    Return nil
```

Therefore these operations also require exclusive access in the current design.

---

# Locking Contract

The store contains an internal expiration helper:

```cpp
bool is_expired(const std::string& key);
```

The helper assumes that the caller already holds the required lock.

This design avoids attempting to acquire the same `shared_mutex` again inside an operation that already owns it.

For example:

```text
GET
 │
 ├── acquire unique_lock
 │
 ├── is_expired()
 │
 ├── possibly erase expired key
 │
 └── release lock
```

Instead of:

```text
GET
 │
 ├── acquire lock
 │
 └── is_expired()
        │
        └── acquire same lock again  ← problematic
```

This makes the locking responsibility explicit and avoids nested locking of the same synchronization primitive.

---

# Lazy Expiration

Mini-Redis uses **lazy expiration**.

When a key receives a TTL:

```text
SET session abc123
EXPIRE session 10
```

the store records the expiration timestamp.

It does not continuously scan the entire store looking for expired keys.

Instead, expiration is checked when a relevant operation accesses the key.

For example:

```text
             Key Access
                  │
                  ▼
          Check expiration
                  │
          ┌───────┴────────┐
          │                │
       Valid             Expired
          │                │
          ▼                ▼
    Return value        Erase key
```

---

# Why Lazy Expiration?

A background expiration mechanism would require an additional worker thread or periodic scanning mechanism.

Lazy expiration keeps the implementation smaller and easier to reason about.

### Advantages

- Simple implementation
- No dedicated expiration thread
- No periodic full-store scan
- Fewer synchronization requirements
- Expiration work occurs only when needed

### Trade-off

An expired key that is never accessed may remain physically present in the underlying map until an operation encounters it.

The key is nevertheless treated as expired by operations that perform expiration checks.

---

# Persistence

Mini-Redis uses **snapshot-based persistence**.

The in-memory state can be serialized to disk and restored when the server starts again.

The persistence lifecycle is:

```text
                  Server Startup
                       │
                       ▼
                 Load Snapshot
                       │
                       ▼
                Restore Store
                       │
                       ▼
                 Accept Clients
                       │
                       ▼
                  Normal Runtime
                       │
                       ▼
              Graceful Termination
                       │
                       ▼
                 Save Snapshot
                       │
                       ▼
                  Server Stops
```

---

# Snapshot Persistence

The snapshot contains the current state of the in-memory store.

For each entry, the snapshot records:

```text
KEY VALUE HAS_EXPIRY EXPIRY_TIMESTAMP
```

Example:

```text
username Sumit 0 0
language cpp 0 0
session abc123 1 1790000000
```

Where:

```text
HAS_EXPIRY = 0
```

means the key has no expiration.

And:

```text
HAS_EXPIRY = 1
```

means an expiration timestamp is stored.

---

# Persistence Restoration

When Mini-Redis starts, it attempts to load the snapshot.

The server reconstructs the in-memory state from the persisted representation.

For keys that have an expiration timestamp, the server checks whether the key has already expired.

Expired entries are not restored.

Conceptually:

```text
Snapshot
   │
   ▼
Read Entry
   │
   ▼
Has Expiry?
   │
   ├── No ──────► Restore
   │
   └── Yes
        │
        ▼
   Already Expired?
        │
        ├── No ──► Restore
        │
        └── Yes ─► Skip
```

---

# Graceful Shutdown

Mini-Redis supports graceful server termination through termination signals such as:

```text
SIGINT
SIGTERM
```

A normal graceful shutdown performs the persistence step before the process exits.

For example:

```text
Ctrl+C
   │
   ▼
Graceful Shutdown
   │
   ▼
Save Store Snapshot
   │
   ▼
Exit
```

This allows the current in-memory state to survive a normal server shutdown.

> **Important:** Snapshot persistence protects state across graceful shutdown. It is not crash-consistent write-ahead logging. A sudden process crash, machine failure, or termination before the snapshot is written can still result in lost changes.

---

# Networking

Mini-Redis uses the POSIX socket API to implement a TCP server.

The basic server lifecycle is:

```text
socket()
   │
   ▼
bind()
   │
   ▼
listen()
   │
   ▼
accept()
   │
   ▼
recv()
   │
   ▼
Process Command
   │
   ▼
send()
```

The server listens on:

```text
Host: 0.0.0.0
Port: 6380
Protocol: TCP
```

---

# TCP Client/Server Model

A TCP client connects to the server:

```text
Client
   │
   │ TCP Connection
   ▼
Mini-Redis
```

Once connected, the client can send multiple commands.

The server processes commands and sends responses over the same TCP connection.

This makes the server stateful at the connection level while keeping the actual key-value state shared across all clients.

---

# Protocol

Mini-Redis currently uses a simple line-oriented text protocol rather than Redis's RESP protocol.

Commands are represented as:

```text
COMMAND ARGUMENTS\n
```

Examples:

```text
SET name Sumit
GET name
DEL name
EXISTS name
EXPIRE name 60
TTL name
KEYS
```

The protocol was intentionally kept simple to focus on the underlying concepts of:

- TCP networking
- server lifecycle
- concurrent connections
- synchronization
- storage

rather than implementing the complete Redis wire protocol.

---

# Protocol Example

A client can communicate with Mini-Redis using:

```text
SET name Sumit
```

The server responds:

```text
+OK
```

Then:

```text
GET name
```

returns:

```text
Sumit
```

A complete interaction may look like:

```text
Client                         Server
  │                              │
  │──── SET name Sumit ─────────►│
  │                              │
  │◄────────── +OK ──────────────│
  │                              │
  │──── GET name ───────────────►│
  │                              │
  │◄────────── Sumit ────────────│
  │                              │
  │──── EXISTS name ────────────►│
  │                              │
  │◄──────────── 1 ──────────────│
```

---

# Building Locally

## Requirements

A POSIX-compatible environment such as:

- Linux
- WSL
- another Unix-like development environment

is recommended.

Required tools:

- C++17-compatible compiler
- POSIX socket support
- pthread support

Recommended tools:

- `netcat`
- Git
- Docker
- Docker Compose

---

# Compiler Check

Verify your compiler:

```bash
g++ --version
```

Mini-Redis requires C++17.

---

# Build

Compile the project with:

```bash
g++ -std=c++17 main.cpp store.cpp -pthread -o miniredis
```

For a stricter development build:

```bash
g++ -std=c++17 \
    -Wall \
    -Wextra \
    -Wpedantic \
    -Wconversion \
    -Wsign-conversion \
    main.cpp store.cpp \
    -pthread \
    -o miniredis
```

---

# Running Locally

Start the server:

```bash
./miniredis
```

The server listens on:

```text
0.0.0.0:6380
```

---

# Testing With Netcat

Open another terminal and connect using:

```bash
nc localhost 6380
```

You can now issue commands directly.

Example:

```text
SET name Sumit
+OK

GET name
Sumit

EXISTS name
1

DEL name
1

GET name
(nil)
```

---

# Testing Multiple Clients

Open multiple terminals.

### Terminal 1

```bash
nc localhost 6380
```

### Terminal 2

```bash
nc localhost 6380
```

### Terminal 3

```bash
nc localhost 6380
```

Each connection is handled by its own server thread.

Because all clients operate on the same `Store` instance, changes made from one client are visible to the others.

Example:

### Client 1

```text
SET shared hello
```

### Client 2

```text
GET shared
```

Result:

```text
hello
```

---

# Testing TTL

Connect to the server:

```bash
nc localhost 6380
```

Create a key:

```text
SET temporary hello
```

Set an expiration:

```text
EXPIRE temporary 10
```

Check the TTL:

```text
TTL temporary
```

The result should be approximately:

```text
9
```

After the TTL expires:

```text
GET temporary
```

returns:

```text
(nil)
```

The expired key is removed lazily.

---

# Testing Persistence

Start Mini-Redis:

```bash
./miniredis
```

Store some values:

```text
SET username Sumit
SET language cpp
SET project MiniRedis
```

Gracefully terminate the server:

```text
Ctrl+C
```

The server writes its current state to the snapshot file.

Start Mini-Redis again:

```bash
./miniredis
```

The state is loaded from the snapshot.

Verify:

```text
GET username
```

Expected:

```text
Sumit
```

Then:

```text
GET language
```

Expected:

```text
cpp
```

And:

```text
GET project
```

Expected:

```text
MiniRedis
```

---

# Docker

Mini-Redis includes a multi-stage Dockerfile.

The build process separates:

```text
Build Environment
        │
        │ compile
        ▼
C++ Executable
        │
        │ copy
        ▼
Runtime Image
```

This avoids requiring the complete compiler toolchain in the final runtime image.

---

# Building the Docker Image

Run:

```bash
docker build -t miniredis .
```

Verify:

```bash
docker image ls
```

---

# Running With Docker

Run the container:

```bash
docker run --rm -p 6380:6380 miniredis
```

Then connect:

```bash
nc localhost 6380
```

The application is now running inside the container while the TCP port is exposed to the host.

---

# Docker Compose

Mini-Redis also provides a Docker Compose configuration.

Start the application:

```bash
docker compose up --build
```

Run in detached mode:

```bash
docker compose up --build -d
```

Check the service:

```bash
docker compose ps
```

View logs:

```bash
docker compose logs -f
```

Stop the service:

```bash
docker compose down
```

---

# Persistent Storage

The Docker Compose configuration maps the host data directory to the container's data directory.

Conceptually:

```text
Host
│
└── ./data
       │
       │ volume mapping
       ▼
Container
│
└── /data
```

This means persistence is not tied exclusively to the lifetime of a container.

The snapshot can remain available on the host and be reused when the service is recreated.

---

# Docker Deployment Flow

```text
Source Code
     │
     ▼
Docker Build
     │
     ▼
Compiled Binary
     │
     ▼
Runtime Container
     │
     ├──────── TCP :6380
     │
     └──────── /data
                   │
                   ▼
              Persistent
                Storage
```

---

# CI/CD

Mini-Redis uses GitHub Actions for automated build validation.

The CI workflow runs when changes are pushed to the repository's main branch or when pull requests target the main branch.

The workflow performs:

```text
Checkout Repository
        │
        ▼
Compile C++ Application
        │
        ▼
Build Docker Image
        │
        ▼
Verify Docker Image
```

This provides an automated check that the project remains buildable and that the container image can be created successfully.

---

# GitHub Actions

The workflow uses:

```text
GitHub Actions
        │
        ├── Checkout
        │
        ├── C++ Build
        │
        ├── Docker Build
        │
        └── Docker Image Verification
```

This prevents basic compilation and containerization failures from going unnoticed.

---

# Design Decisions

## Why `std::unordered_map`?

Mini-Redis is primarily a key-based lookup system.

`std::unordered_map` provides average constant-time complexity for:

- lookup
- insertion
- deletion

This makes it a natural data structure for a basic in-memory key-value store.

Conceptually:

```text
Key
 │
 ▼
Hash Function
 │
 ▼
Bucket
 │
 ▼
Value
```

---

# Why C++?

C++ was selected because the project is intended to explore systems-level concepts.

It provides direct access to:

- threads
- synchronization primitives
- file I/O
- socket APIs
- memory-oriented data structures
- low-level operating-system interfaces

This makes the language particularly suitable for understanding the mechanics behind a networked in-memory server.

---

# Why POSIX Sockets?

POSIX sockets provide a direct interface to TCP networking.

Using them exposes the core lifecycle of a network server:

```text
socket
bind
listen
accept
recv
send
```

Instead of hiding these details behind a high-level networking framework, Mini-Redis implements the basic server lifecycle directly.

---

# Why Thread-per-Client?

The thread-per-client model was selected because it is straightforward to understand and implement.

Each connection gets its own execution context:

```text
Connection 1 → Thread 1
Connection 2 → Thread 2
Connection 3 → Thread 3
```

### Advantages

- Simple mental model
- Easy client isolation
- Straightforward blocking I/O
- Natural mapping between client and thread

### Disadvantages

- One thread per connection
- Thread creation overhead
- Large numbers of clients can create resource pressure
- Less scalable than event-driven I/O for very high connection counts

The architecture is appropriate for the scope of Mini-Redis.

---

# Why `std::shared_mutex`?

A `std::shared_mutex` allows a design to distinguish between shared and exclusive access.

Conceptually:

```text
Shared Lock
    │
    ├── Reader A
    ├── Reader B
    └── Reader C

Exclusive Lock
    │
    └── Writer
```

However, because Mini-Redis performs lazy deletion during operations such as `GET`, `EXISTS`, and `TTL`, those operations may modify the map.

Therefore the current implementation uses exclusive locking where lazy expiration can occur.

This prioritizes correctness and clear synchronization semantics over attempting to maximize read parallelism prematurely.

---

# Why Lazy Expiration Instead of a Background Thread?

A background expiration worker could periodically scan for expired keys.

However, that introduces another concurrent component:

```text
Client Threads
      │
      ├──────────────┐
      │              │
      ▼              ▼
    Store       Expiration
                 Worker
```

That would require additional synchronization and lifecycle management.

Lazy expiration avoids this additional complexity.

For the scope of Mini-Redis, the trade-off is appropriate.

---

# Why Snapshot Persistence?

Snapshot persistence was selected instead of AOF because it provides a simple mechanism for restoring the in-memory state without requiring every write command to be logged.

### Snapshot

```text
Current Memory
      │
      ▼
Snapshot File
```

### AOF

```text
SET ...
SET ...
DEL ...
SET ...
...
```

Mini-Redis focuses on snapshot persistence to keep the system compact and understandable.

---

# Why Not RESP?

Redis uses the Redis Serialization Protocol (RESP).

Mini-Redis currently uses a simpler line-oriented protocol.

Implementing RESP would provide greater compatibility with Redis clients, but would also add protocol parsing and serialization complexity.

The current project prioritizes understanding:

- TCP
- concurrency
- synchronization
- storage
- persistence

over Redis protocol compatibility.

---

# Trade-offs

Every architectural decision in Mini-Redis involves a trade-off.

| Decision | Advantage | Trade-off |
|---|---|---|
| `unordered_map` | Fast average key lookup | No ordering |
| Thread-per-client | Simple concurrency model | Thread overhead |
| `shared_mutex` | Supports shared/exclusive semantics | More synchronization complexity |
| Lazy expiration | Simple, no background worker | Expired entries may remain until encountered |
| Snapshot persistence | Simple restoration | Recent changes can be lost after unexpected failure |
| Text protocol | Easy to debug | Not Redis-compatible RESP |
| Docker | Reproducible environment | Adds containerization overhead |

---

# Error Handling

The server validates command names and argument counts before executing operations.

Malformed commands should result in an error response rather than terminating the server.

Examples:

```text
UNKNOWN
SET
GET
EXPIRE key
```

may result in an error response similar to:

```text
-ERR unknown command or wrong number of arguments
```

The goal is to keep malformed client input isolated to the individual request.

---

# Limitations

Mini-Redis is intentionally not a production Redis replacement.

Current limitations include:

- Simple text-based protocol
- No RESP support
- No AOF
- No replication
- No clustering
- No transactions
- No authentication
- No advanced Redis data types
- No LRU eviction
- No memory limit
- No distributed operation
- Thread-per-client architecture
- Snapshot-based rather than crash-consistent persistence
- Simple command parsing
- String-only values

These limitations are intentional and keep the project focused on the core systems concepts.

---

# Future Improvements

The architecture provides several possible directions for future development.

## Protocol Improvements

- RESP support
- Binary-safe values
- More robust TCP stream framing
- Better command parsing
- More Redis-compatible responses

## Concurrency Improvements

- Fixed-size thread pool
- Work queue
- Asynchronous I/O
- Event-driven architecture
- Linux `epoll`

## Persistence Improvements

- Append-Only File (AOF)
- Periodic snapshots
- Atomic snapshot replacement
- Crash recovery
- Snapshot versioning
- Corruption detection

## Memory Management

- Configurable memory limits
- LRU eviction
- Memory usage statistics

## Data Types

Potential additional data structures:

```text
Lists
Sets
Hashes
Sorted Sets
```

## Observability

Potential additions:

- Request counters
- Connected client count
- Operation latency
- Throughput metrics
- Server statistics
- Logging

---

# Project Roadmap

Mini-Redis was developed incrementally around several systems concepts.

## Phase 1 — Core Storage

- In-memory hash map
- Basic CRUD operations
- Command-line interaction

## Phase 2 — TCP Server

- POSIX socket server
- TCP client connections
- Command/response protocol

## Phase 3 — Command Processing

- SET
- GET
- DEL
- EXISTS
- EXPIRE
- TTL
- KEYS
- Error handling
- Lazy expiration

## Phase 4 — Concurrency

- Multiple simultaneous clients
- `std::thread`
- Shared store
- Synchronization
- `std::shared_mutex`

## Phase 5 — Persistence

- Snapshot serialization
- Startup restoration
- Expiration restoration
- Graceful shutdown persistence

## Deployment

- Docker
- Docker Compose
- Persistent volumes
- GitHub Actions CI

The project deliberately stops short of implementing advanced Redis functionality so that each implemented component remains understandable and discussable.

---

# Build and Run Cheat Sheet

## Local Build

```bash
g++ -std=c++17 main.cpp store.cpp -pthread -o miniredis
```

## Run

```bash
./miniredis
```

## Connect

```bash
nc localhost 6380
```

---

## Docker Build

```bash
docker build -t miniredis .
```

## Docker Run

```bash
docker run --rm -p 6380:6380 miniredis
```

---

## Docker Compose

Start:

```bash
docker compose up --build
```

Detached:

```bash
docker compose up --build -d
```

Status:

```bash
docker compose ps
```

Logs:

```bash
docker compose logs -f
```

Stop:

```bash
docker compose down
```

---

# Example End-to-End Session

Start the server:

```bash
./miniredis
```

Connect:

```bash
nc localhost 6380
```

Store data:

```text
SET name Sumit
+OK

SET language C++
+OK
```

Retrieve data:

```text
GET name
Sumit

GET language
C++
```

Check existence:

```text
EXISTS name
1
```

Set an expiration:

```text
EXPIRE name 30
1
```

Check TTL:

```text
TTL name
29
```

List keys:

```text
KEYS
language
name
```

Delete a key:

```text
DEL name
1
```

Verify deletion:

```text
GET name
(nil)
```

Close the client:

```text
QUIT
```

---

# Systems Concepts Demonstrated

Mini-Redis provides practical exposure to several core computer-science and systems concepts.

## Data Structures

- Hash tables
- Key-value storage
- Hash-based lookup
- Average O(1) lookup/insertion/deletion

## Operating Systems

- Processes
- Threads
- Synchronization
- Signals
- File I/O
- Shared state

## Networking

- TCP
- Sockets
- Client/server architecture
- Blocking I/O
- Connection lifecycle
- Network protocols

## Concurrency

- Race conditions
- Mutual exclusion
- Shared resources
- Thread synchronization
- Read/write synchronization
- Thread-per-client architecture

## Persistence

- Serialization
- Deserialization
- Snapshots
- State restoration
- Expiration timestamps

## DevOps

- Docker
- Multi-stage builds
- Docker Compose
- Persistent volumes
- CI workflows

---

# Complexity

For the underlying hash map, the expected average complexity is:

| Operation | Average Complexity |
|---|---:|
| `SET` | O(1) |
| `GET` | O(1) |
| `DEL` | O(1) |
| `EXISTS` | O(1) |
| `EXPIRE` | O(1) |
| `TTL` | O(1) |
| `KEYS` | O(n) |

`KEYS` requires traversing the store and is therefore linear in the number of stored keys.

Expiration checks performed during key-specific operations are constant-time on average because they first perform a hash-map lookup.

---

# Security Considerations

Mini-Redis is a learning and portfolio project and should not be exposed directly to an untrusted public network.

The current implementation does not provide:

- Authentication
- Authorization
- TLS
- Encryption
- Rate limiting
- Resource quotas
- Production-grade input hardening

For local development and controlled environments, the server is sufficient for demonstrating the intended systems concepts.

---

# Why This Project?

The purpose of Mini-Redis is to bridge the gap between theoretical computer-science knowledge and practical systems programming.

Rather than interacting with a database through a high-level API, the project implements the major building blocks directly:

```text
                 ┌──────────────────┐
                 │   Data Structure │
                 │   unordered_map  │
                 └────────┬─────────┘
                          │
                 ┌────────▼─────────┐
                 │   Synchronization│
                 │  shared_mutex    │
                 └────────┬─────────┘
                          │
                 ┌────────▼─────────┐
                 │    Networking    │
                 │   TCP Sockets    │
                 └────────┬─────────┘
                          │
                 ┌────────▼─────────┐
                 │   Persistence    │
                 │    Snapshots     │
                 └────────┬─────────┘
                          │
                 ┌────────▼─────────┐
                 │   Deployment     │
                 │ Docker + CI      │
                 └──────────────────┘
```

The resulting system is small enough to understand end-to-end while still exposing important concepts used in real server-side systems.

---

# Interview Discussion Points

Mini-Redis can be used to discuss several systems-programming topics in technical interviews.

### Networking

- Why TCP instead of UDP?
- What does `socket()` return?
- Why are `bind()`, `listen()`, and `accept()` separate operations?
- What is the difference between the listening socket and a client socket?
- How does a TCP connection work?

### Concurrency

- Why use one thread per client?
- What happens when multiple clients modify the same key?
- Why is synchronization necessary?
- Why use `shared_mutex`?
- What is a race condition?
- What happens if the map is accessed concurrently without synchronization?

### Expiration

- Why lazy expiration?
- Why can `GET` require an exclusive lock?
- What happens to an expired key that is never accessed?
- How would a background expiration worker change the design?

### Persistence

- Why snapshotting?
- What happens if the server crashes before a snapshot?
- How does startup restoration work?
- How could AOF improve durability?
- How could snapshots be made atomic?

### Scalability

- What are the limitations of thread-per-client?
- How would a thread pool improve the design?
- How would `epoll` change the architecture?
- How would the system behave with thousands of simultaneous clients?

---

# Project Highlights

The main engineering concepts demonstrated by Mini-Redis are:

```text
✓ C++17 systems programming
✓ POSIX TCP networking
✓ Concurrent client handling
✓ Thread synchronization
✓ Shared state management
✓ TTL-based expiration
✓ Lazy deletion
✓ Snapshot persistence
✓ Graceful shutdown
✓ Docker containerization
✓ Docker Compose
✓ GitHub Actions CI
```

---

# Repository

GitHub:

https://github.com/mesumittiwari/MiniRedis

---

# Author

## Sumit Tiwari

B.Tech — Mathematics and Computing  
Madhav Institute of Technology and Science, Gwalior

- GitHub: https://github.com/mesumittiwari
- LinkedIn: https://linkedin.com/in/mesumittiwari
- LeetCode: https://leetcode.com/mesumittiwari

---

# License

This project is intended primarily as a learning and portfolio project.

If you choose to add an open-source license, place the corresponding license text in a `LICENSE` file in the repository.

---

<p align="center">
  Built from scratch with C++17 to understand networking, concurrency, persistence, and systems programming.
</p>
