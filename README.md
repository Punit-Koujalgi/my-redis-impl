# 🚀 Redis Implementation in C++ - check out [demo](https://huggingface.co/spaces/pkoujalgi/My-Redis-Impl)

[![Build Status](https://img.shields.io/badge/build-passing-brightgreen.svg)](https://shields.io/) [![Redis Protocol](https://img.shields.io/badge/protocol-RESP-red.svg)](https://redis.io/docs/latest/develop/reference/protocol-spec/) [![C++](https://img.shields.io/badge/C%2B%2B-23-blue.svg)](https://isocpp.org/) [![License](https://img.shields.io/badge/license-MIT-green.svg)](LICENSE)

A high-performance Redis server implementation written in modern C++23, featuring comprehensive support for Redis core functionality, data structures, transactions, pub/sub, streams, and replication.

## 📋 Table of Contents

- [🔥 Features](#-features)
- [🔧 Building the Server](#-building-the-server)
- [🚀 Running the Server](#-running-the-server)
- [🔌 Connecting with redis-cli](#-connecting-with-redis-cli)
- [💻 Web UI](#-web-ui)
- [📚 Supported Commands](#-supported-commands)

## 🔥 Features

This Redis implementation provides a comprehensive feature set that mirrors the official Redis server:

### 🌟 Core Features
- ⚡ **High-performance TCP server** with concurrent client support
- 🔤 **RESP Protocol** fully compliant Redis Serialization Protocol
- 💾 **In-memory data storage** with multiple data types
- 🔄 **Persistence** with RDB snapshots
- 🔁 **Replication** - Master-slave replication with real-time sync

### 📊 Data Structures
- 📝 **Strings** - Basic key-value storage with expiration support
- 📋 **Lists** - Doubly-linked lists with push/pop operations
- 🌊 **Streams** - Log-based data structure for real-time data
- 🏷️ **Type system** with dynamic type checking

### 🚀 Advanced Capabilities
- 💸 **Transactions** - ACID transactions with MULTI/EXEC/DISCARD
- 📢 **Pub/Sub** - Message publishing and subscription system
- 🧵 **Multi-slave Replication** - support for multiple slaves and **WAIT** command
- ⏰ **Expiration** - TTL support for automatic key cleanup
- 🔍 **Pattern matching** - KEYS command with glob patterns

## 🔧 Building the Server

### Prerequisites
- **CMake** 3.13 or higher
- **C++23** compatible compiler (GCC 11+ or Clang 14+)
- **Make** build system

### Build Steps

1. **Clone and navigate to the project**:
   ```bash
   cd /path/to/my_redis
   ```

2. **Create build directory**:
   ```bash
   mkdir -p build
   cd build
   ```

3. **Configure with CMake**:
   ```bash
   cmake ..
   ```

4. **Build the server**:
   ```bash
   cmake --build .
   ```

The executable `server` will be created in the `build` directory.

## 🚀 Running the Server

### Default Port (6379)
```bash
./build/server
```

### Custom Port
```bash
./build/server --port 1234
```

The server will start listening for connections and display:
```
Signal handling setup complete..
Redis port: 1234
Starting EventLoop...
```

## 🔌 Connecting with redis-cli

You can connect to your Redis server using the official `redis-cli` client:

### Basic Connection
```bash
redis-cli -p 6379
```

### Example Commands

#### 🏓 Test Connection
```bash
127.0.0.1:6379> PING
PONG
```

#### 📝 String Operations
```bash
127.0.0.1:6379> SET mykey "Hello Redis!"
OK
127.0.0.1:6379> GET mykey
"Hello Redis!"
127.0.0.1:6379> SET counter 42
OK
127.0.0.1:6379> INCR counter
(integer) 43
```

#### 📋 List Operations
```bash
127.0.0.1:6379> RPUSH mylist "first" "second" "third"
(integer) 3
127.0.0.1:6379> LRANGE mylist 0 -1
1) "first"
2) "second"
3) "third"
127.0.0.1:6379> LPOP mylist
"first"
127.0.0.1:6379> LLEN mylist
(integer) 2
```

#### 🌊 Stream Operations
```bash
127.0.0.1:6379> XADD mystream * name "Alice" age "30"
"1640995200000-0"
127.0.0.1:6379> XADD mystream * name "Bob" age "25"
"1640995200001-0"
127.0.0.1:6379> XRANGE mystream - +
1) 1) "1640995200000-0"
   2) 1) "name"
      2) "Alice"
      3) "age"
      4) "30"
2) 1) "1640995200001-0"
   2) 1) "name"
      2) "Bob"
      3) "age"
      4) "25"
```

#### 💸 Transaction Example
```bash
127.0.0.1:6379> MULTI
OK
127.0.0.1:6379> SET key1 "value1"
QUEUED
127.0.0.1:6379> SET key2 "value2"
QUEUED
127.0.0.1:6379> INCR counter
QUEUED
127.0.0.1:6379> EXEC
1) OK
2) OK
3) (integer) 44
```

#### 📢 Pub/Sub Example
```bash
# In terminal 1 (Subscriber)
127.0.0.1:6379> SUBSCRIBE news
Reading messages... (press Ctrl-C to quit)
1) "subscribe"
2) "news"
3) (integer) 1

# In terminal 2 (Publisher)
127.0.0.1:6379> PUBLISH news "Breaking: Redis implementation completed!"
(integer) 1

# Back to terminal 1 - you'll see:
1) "message"
2) "news"
3) "Breaking: Redis implementation completed!"
```

## 💻 Web UI

This project includes a beautiful web-based UI built with Gradio for interactive Redis exploration!

### 🛠️ Setup UI

1. **Navigate to UI directory**:
   ```bash
   cd ui
   ```

2. **Install dependencies using uv**:
   ```bash
   uv sync
   ```

3. **Launch the UI**:
   ```bash
   uv run main.py
   ```

4. **Open in browser**:
   Visit the link displayed in terminal (typically `http://127.0.0.1:7860`)

### ✨ UI Features
- 🎯 **Command Explorer** - Browse all supported commands with examples
- 🖥️ **Interactive Terminal** - Execute Redis commands in real-time
- 👁️ **RESP Protocol Viewer** - See raw protocol messages
- 🚀 **Auto Server Management** - Automatically manages Redis server
- 💡 **Smart Examples** - Pre-built command examples
- 🎨 **Beautiful Interface** - Modern, clean design

## 📝 How to start your own Replica?

```bash
./build/server --port <PORT> --replicaof "<MASTER_HOST> <MASTER_PORT>"
```

The server will come up as slave of Master and will start listening for connections on PORT.
- **You can start as many slaves as needed for high availabilty.**
- **Commands will be replicated to all the slaves.**
- **The WAIT command** can be used to check how many replicas have acknowledged a write command. This allows a client to measure the durability of a write command before considering it successful.

The command format is:
```
WAIT <numreplicas> <timeout>
```
Here's what each argument means:

numreplicas: The minimum number of replicas that must acknowledge the write command.

timeout: The maximum time (in milliseconds) the client is willing to wait.


## 📚 Supported Commands

### 🏓 Connection & Server
| Command | Description | Example |
|---------|-------------|---------|
| `PING` | Test server connectivity | `PING` → `PONG` |
| `ECHO` | Echo a message | `ECHO "hello"` → `"hello"` |
| `COMMAND` | Get available commands | `COMMAND` → `[array of commands]` |

### 📝 String Operations
| Command | Description | Example |
|---------|-------------|---------|
| `SET` | Set key to value | `SET mykey "value"` → `OK` |
| `GET` | Get value by key | `GET mykey` → `"value"` |
| `INCR` | Increment numeric value | `INCR counter` → `(integer) 1` |
| `TYPE` | Get value type | `TYPE mykey` → `"string"` |

### 📋 List Operations
| Command | Description | Example |
|---------|-------------|---------|
| `RPUSH` | Push to list tail | `RPUSH mylist "a" "b"` → `(integer) 2` |
| `LPUSH` | Push to list head | `LPUSH mylist "z"` → `(integer) 3` |
| `RPOP` | Pop from list tail | `RPOP mylist` → `"b"` |
| `LPOP` | Pop from list head | `LPOP mylist` → `"z"` |
| `LRANGE` | Get list range | `LRANGE mylist 0 -1` → `1) "a"` |
| `LLEN` | Get list length | `LLEN mylist` → `(integer) 1` |
| `BLPOP` | Blocking list pop | `BLPOP mylist 10` → `1) "mylist" 2) "a"` |

### 🌊 Stream Operations
| Command | Description | Example |
|---------|-------------|---------|
| `XADD` | Add entry to stream | `XADD stream * field value` → `"1234567890-0"` |
| `XRANGE` | Read stream range | `XRANGE stream - +` → `[entries...]` |
| `XREAD` | Read from streams | `XREAD STREAMS stream 0` → `[stream data...]` |

### 💸 Transaction Commands
| Command | Description | Example |
|---------|-------------|---------|
| `MULTI` | Start transaction | `MULTI` → `OK` |
| `EXEC` | Execute transaction | `EXEC` → `[results...]` |
| `DISCARD` | Discard transaction | `DISCARD` → `OK` |

### 📢 Pub/Sub Commands
| Command | Description | Example |
|---------|-------------|---------|
| `SUBSCRIBE` | Subscribe to channel | `SUBSCRIBE news` → `1) "subscribe"...` |
| `UNSUBSCRIBE` | Unsubscribe from channel | `UNSUBSCRIBE news` → `1) "unsubscribe"...` |
| `PUBLISH` | Publish to channel | `PUBLISH news "message"` → `(integer) 1` |

### 🔧 Administrative Commands
| Command | Description | Example |
|---------|-------------|---------|
| `CONFIG` | Get/set configuration | `CONFIG GET *` → `[config pairs...]` |
| `SAVE` | Save snapshot | `SAVE` → `OK` |
| `KEYS` | Find keys by pattern | `KEYS *` → `[key list...]` |
| `INFO` | Server information | `INFO` → `[server stats...]` |
| `WAIT` | Wait for replicas | `WAIT 1 1000` → `(integer) 1` |

### 🔁 Replication Commands
| Command | Description | Example |
|---------|-------------|---------|
| `REPLCONF` | Replication configuration | `REPLCONF ACK 0` → `OK` |
| `PSYNC` | Partial sync | `PSYNC ? -1` → `+FULLRESYNC...` |


---

<div align="center">

**🎉 Built with ❤️ using modern C++23**

*A complete Redis implementation showcasing advanced systems programming concepts*

</div>
