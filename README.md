*This project has been created as part of the 42 curriculum by mapadron, jvizcain and roherna2.*

# treenity - Message Queue System

`treenity` is a high-performance, asynchronous message queue system implemented in C++17. Designed around event-driven architectures, it supports dynamic topic creation, publisher-subscriber messaging, prefix-based message routing, offset tracking, and multi-client concurrent processing. Communication between server and client processes is established via standard POSIX IPC mechanisms.

---

## Description

The objective of `treenity` is to provide a robust, lightweight message broker capable of handling high-throughput message publishing and real-time subscription filtering.

### Key Features
- **Publish / Subscribe Pattern**: Producers publish key-value pairs to named topics, while consumers subscribe to topics to consume messages.
- **Prefix Matching Routing**: Consumers can subscribe to specific key prefixes (or wildcard `""`) on a topic, receiving only relevant events efficiently.
- **Offset Management & Log Persistence**: Messages within a topic are stored sequentially with strictly increasing offsets (0-indexed). Subscribers can request delivery starting at a specific offset.
- **Concurrent Topic Processing**: Each topic operates on its own dedicated worker thread, isolating message distribution and lock contention.
- **Custom High-Performance Data Structures**: Implements custom header-only templated `HashMap` and a character-by-character `PrefixTree` (Trie) for maximum efficiency and memory control.

---

## Instructions

### Prerequisites
- GCC / G++ supporting C++17
- GNU Make
- POSIX-compliant operating system (Linux / macOS)

### Compilation

Build both the `server` and `client` binaries:
```bash
make
```
This compiles object files into `objs/` and places executables at the root directory.

Other Makefile rules:
```bash
make clean    # Remove build object files
make fclean   # Remove objects, binaries, and symlinks
make re       # Rebuild the entire project
```

### Execution

#### 1. Running the Server
Start the message queue server (default FIFO path: `/tmp/treenity.ipc`):
```bash
./server
```

#### 2. Running Client Commands

##### Topic Management
- **Create a Topic**:
  ```bash
  ./client create <topic_name>
  ```
- **List Active Topics**:
  ```bash
  ./client list
  ```
- **Get Topic Info** (shows topic name and total message count):
  ```bash
  ./client info <topic_name>
  ```

##### Producer (Publishing Messages)
Publish a single key-value message to a topic:
```bash
./client <ipc_identifier> produce <topic_name> [--raw]
```

##### Subscriber (Consuming Messages)
Subscribe to a topic with optional offset and prefix filters:
```bash
./client <ipc_identifier> subscribe <topic_name> <subscriber_name> [--prefix <prefix>] [--offset <offset>] [--raw]
```

---

## Architecture

The `treenity` server employs a decoupled, multi-threaded architectural model to ensure low latency and high concurrency across topics.

```
                   +------------------------------------+
                   |         Server Main Thread         |
                   |   (Listens on /tmp/treenity.ipc)   |
                   +-----------------+------------------+
                                     |
           +-------------------------+-------------------------+
           |                                                   |
           v                                                   v
+-----------------------+                           +-----------------------+
|  Topic "events" Thread |                           |  Topic "orders" Thread |
|  - Message Storage    |                           |  - Message Storage    |
|  - PrefixTree (Trie)  |                           |  - PrefixTree (Trie)  |
|  - Active Subscribers |                           |  - Active Subscribers |
+-----------------------+                           +-----------------------+
```

### Threading Model
1. **Server Main Thread**: Accepts incoming requests on the well-known server request FIFO (`/tmp/treenity.ipc`). Handles client connections, topic creation, administration commands (`LIST`, `INFO`), producer session registration, and client disconnect cleanup.
2. **Per-Topic Dedicated Threads (`Topic::worker_thread`)**: Each topic instance maintains its own worker thread and event loop. When a producer publishes a message to a topic, the main server thread appends the message to the topic's buffer and wakes up the topic thread via condition variable. The topic thread processes subscriber delivery asynchronously without blocking main server ingress.

### Synchronization Strategy
- **Server State Protection (`server_mutex`)**: A recursive lock guarding shared server tables including active topics (`HashMap<string, unique_ptr<Topic>>`), client session metadata (`HashMap<string, ClientMetadata>`), and producer mapping table (`HashMap<uint32_t, string>`).
- **Per-Topic Fine-Grained Locking (`topic_mutex`)**: Guards the message history buffer (`vector<Message>`), subscriber table, and the topic's `PrefixTree`. Locking is isolated per topic, avoiding global mutex contention when multiple topics are active simultaneously.
- **Condition Variable Notification (`cv`)**: Topic worker threads sleep on `std::condition_variable cv` when idle, waking up only when new messages are published or when a shutdown signal is received.
- **Atomic State Control**: Teardown sequence uses `std::atomic<bool> running` and sentinel shutdown payloads sent across FIFOs to safely unblock blocked `read()` and `write()` calls during SIGINT/SIGTERM termination.

---

## IPC Choice

The system uses **POSIX Named Pipes (FIFOs)** for inter-process communication.

### Justification
- **Stream Semantics & POSIX Standard**: FIFOs provide clean Unix stream interface semantics compatible with standard `read()`, `write()`, and descriptor management tools without system-wide kernel queue limits.
- **Resource Cleanup & File System Namespace**: Unlike System V message queues (which require complex `msgctl` cleanup and risk stale kernel queue pollution on ungraceful crashes), FIFOs exist as filesystem nodes (`mkfifo`) and are easily managed via `unlink()`.
- **Low Overhead**: Direct pipe kernel buffers avoid overhead associated with socket protocol state machines or SysV structures.

### Bidirectional Communication Architecture
Bidirectional IPC is achieved through a dual-FIFO pattern:
1. **Server Request FIFO**: A single well-known FIFO (default `/tmp/treenity.ipc`) created by the server. All clients write request frames (e.g. `CREATE`, `LIST`, `PRODUCER`, `SUBSCRIBER`, `DISCONNECT`) to this pipe.
2. **Per-Client Response / Streaming FIFOs**: Upon connection, each client creates a unique client-side FIFO named `/tmp/treenity_client_<PID>.fifo`.
   - For synchronous command responses (`CREATE`, `LIST`, `INFO`), the server writes status response headers and payloads back to the client's FIFO.
   - For subscribers, the server opens the client's dedicated FIFO and streams matched published messages directly from the topic's worker thread.

---

## Data Structures

### Custom `HashMap<Key, Value>`
The repository includes a custom, header-only templated Hash Table (`src/data_structures/HashMap.hpp`).

- **Implementation Details**: Uses separate chaining with `std::vector<std::list<std::pair<Key, Value>>>` to handle hash collisions smoothly.
- **Dynamic Resizing**: Automatically rehashes and doubles bucket capacity when the load factor exceeds `0.75` (default max load factor).
- **Time Complexity**: Average $O(1)$ lookup, insertion, and erasure; worst-case $O(N)$ in collision degradation.
- **Usage**: Manages the topic directory (`topics`), client sessions (`client_index`), producer associations (`producer_topics`), and Trie node child pointers.

### Prefix Matching Structure (`PrefixTree` / Trie)
Prefix-based subscriber filtering is implemented using a specialized Trie (`src/data_structures/PrefixTree.hpp` and `src/data_structures/PrefixTree.cpp`).

```
                              Root Node [""]
                           Subscribers: [SubA]
                                 /    \
                               'u'    'o'
                               /        \
                             's'        'r'
                             /            \
                           'e'            'd'
                           /                \
                         'r'                'e'
                         /                    \
                     Node ["user"]          Node ["orde"]
                  Subscribers: [SubB]              \
                                                  'r'
                                                   \
                                               Node ["order"]
                                            Subscribers: [SubC]
```

- **Node Structure**: Each node contains:
  1. `std::vector<std::string> subscribers`: List of client IDs subscribed to the exact prefix terminated at this node.
  2. `HashMap<char, std::unique_ptr<Node>> children`: Custom Hash Map indexing child nodes by character.
- **Prefix Matching Algorithm (`match`)**:
  1. Collects wildcard subscribers registered at the root node (`""`).
  2. Traverses character by character along the input message key.
  3. At every ancestor node along the path, appends subscribers to the resulting list.
  4. Returns immediately if a branch path terminates early.
- **Complexity Justification**: Key lookup efficiency is $O(|K| + M)$, where $|K|$ is the length of the published message key and $M$ is the number of matched subscribers. The search cost is entirely independent of total non-matching subscribers $N$.
- **Automatic Pruning**: When a subscriber disconnects, `remove()` removes the client ID and prunes unused intermediate nodes to conserve memory.

---

## Testing

The project incorporates unit testing using **Google Test (GTest)** bundled under `tests/googletest`.

### Building and Running Tests

Run the complete test suite via the top-level Makefile:
```bash
make test
```
Or directly inside the `tests` directory:
```bash
cd tests
make test
```

### Test Coverage
- **`test_hashmap.cpp`**: Verifies `HashMap` constructors, move semantics, insertion, element replacement, erase operations, collision chaining, and dynamic rehashing growth.
- **`test_prefixtree.cpp`**: Validates `PrefixTree` insertion, exact string matches, hierarchical prefix matching, wildcard root matching (`""`), node branch pruning, and clearing operations.
- **`test_protocol.cpp`**: Tests binary protocol Little-Endian serialization, deserialization, frame boundary handling, error code responses, and message packing/unpacking.

---

## Resources

### Documentation & References
- **POSIX FIFOs & Pipes**: `mkfifo(3)`, `pipe(2)`, `fifo(7)` Linux Programmer's Manual.
- **Cpp reference**: [https://en.cppreference.com/](https://en.cppreference.com/).
- **Google Test**: [GoogleTest Documentation](https://github.com/google/googletest).

### AI Usage Disclosure
Artificial Intelligence was utilized during the development of this project for the following specific tasks:
- **Documentation & README Generation**: Structuring, drafting, and formatting project documentation and technical architecture diagrams.
- **Unit Test Boilerplate**: Assisting in generating Google Test suite cases for `HashMap`, `PrefixTree`, and binary protocol boundary tests.
- **Code Refactoring Assistance**: Providing recommendations on modern C++17 move semantics and clean code organization.

