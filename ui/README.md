# 🚀 Redis Playground UI

An interactive web interface for exploring Redis commands and understanding the RESP protocol.

## ✨ Features

- 🎯 **Command Explorer**: Browse all supported Redis commands with descriptions and examples
- 🖥️ **Interactive Terminal**: Execute Redis commands in a shell-like interface
- 👁️ **RESP Protocol Viewer**: See the raw RESP format for requests and responses
- 🚀 **Auto Server Management**: Automatically starts/stops the Redis server
- 💡 **Smart Examples**: Pre-built examples for each command type
- 🎨 **Beautiful UI**: Clean, modern interface with syntax highlighting

## 🛠️ Setup

1. **Build the Redis server** (if not already done):
   ```bash
   cd ../build
   cmake ..
   cmake --build .
   ```

2. **Install dependencies** (using uv):
   ```bash
   uv pip install gradio redis
   ```

3. **Launch the UI**:
   ```bash
   python main.py
   # Or use the launcher script
   chmod +x launch.sh
   ./launch.sh
   ```

4. **Open your browser** to `http://localhost:7860`

## 🎮 How to Use

### Command Explorer (Left Panel)
- Select any Redis command from the dropdown
- Read the description and use cases
- View example usage patterns
- Click "Try This Example" to load it into the terminal

### Redis Terminal (Right Panel)
- Type Redis commands in the input field
- Click "Execute" or press Enter to run commands
- View results in three tabs:
  - **Output**: Formatted command results
  - **RESP Request**: Raw protocol request format
  - **RESP Response**: Raw protocol response format

### Quick Commands
- Use the quick command buttons for common operations
- Perfect for getting started quickly

## 🎯 Supported Commands

The UI supports all commands implemented in your Redis server:

**Core Commands:**
- **Basic**: PING, ECHO, COMMAND
- **Key-Value**: SET (with PX expiration), GET, KEYS, TYPE
- **Numbers**: INCR
- **Server Info**: INFO, CONFIG

**List Operations:**
- **Push**: LPUSH, RPUSH (with multiple elements)
- **Pop**: LPOP, RPOP (with count support)
- **Query**: LLEN, LRANGE (with negative indices)
- **Blocking**: BLPOP (with timeout)

**Stream Operations:**
- **Add**: XADD (append to streams)
- **Read**: XRANGE (range queries), XREAD (blocking reads)

**Transaction Support:**
- **Control**: MULTI, EXEC, DISCARD
- **Atomic Operations**: Queue and execute commands atomically

**Pub/Sub Messaging:**
- **Subscribe**: SUBSCRIBE, UNSUBSCRIBE
- **Publish**: PUBLISH (broadcast to subscribers)

**Advanced Features:**
- **Replication**: REPLCONF, PSYNC, WAIT
- **Persistence**: Automatic RDB support

## 🔍 RESP Protocol Learning

The UI shows you exactly what happens under the hood:

**Request Format:**
```
* Array: 2 elements
$ Bulk String: length 3
  Data: 'SET'
$ Bulk String: length 5  
  Data: 'mykey'
$ Bulk String: length 7
  Data: 'myvalue'
```

**Response Format:**
```
+ Simple String: 'OK'
```

This helps you understand how Redis clients communicate with the server!

## 🎨 UI Components

- **Header**: Welcome message and Redis introduction
- **Server Status**: Shows connection status and control buttons
- **Command Explorer**: Interactive command reference
- **Terminal Interface**: Command execution environment  
- **RESP Viewer**: Protocol message inspection
- **Quick Commands**: One-click common operations

## 🚀 Getting Started

1. Start with `PING` to test connectivity
2. Try `SET mykey hello` to store a value
3. Use `GET mykey` to retrieve it
4. Explore `KEYS *` to see all stored keys
5. Check out the RESP tabs to see the protocol in action!

## 💡 Tips

- Use the command dropdown to learn about each command
- Watch the RESP protocol to understand Redis internals
- Try the quick commands for common operations
- Use `INFO` to see server statistics
- `FLUSHDB` clears all data (be careful!)

Happy Redis exploring! 🎉