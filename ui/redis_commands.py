REDIS_COMMANDS = {
    "PING": {
        "description": """
        ## 🏓 PING
        
        **Purpose:** Test connectivity to the Redis server
        
        **Syntax:** `PING`
        
        **What it does:**
        - Returns PONG to confirm server is alive
        - Simplest Redis command for health checking
        - Super fast response time
        
        **Use cases:**
        - Health checks in production monitoring
        - Testing if server is responsive
        - Network latency testing
        
        **Fun fact:** 🎾 Named after the ping-pong sound!
        """,
        "example": "PING"
    },
    
    "ECHO": {
        "description": """
        ## � ECHO
        
        **Purpose:** Echo back the given message
        
        **Syntax:** `ECHO message`
        
        **What it does:**
        - Returns exactly what you send it
        - Perfect for testing and debugging
        - Handles any string input
        
        **Use cases:**
        - Testing client-server communication
        - Debugging network issues
        - Validating message encoding
        
        **Creative example:** 🎭 Use it as a "parrot mode" for fun!
        """,
        "example": "ECHO \"Hello, Redis World!\"\n# Expected: Hello, Redis World!\nECHO \"Multi word message\"\n# Expected: Multi word message"
    },
    
    "SET": {
        "description": """
        ## � SET
        
        **Purpose:** Store a key-value pair with optional expiration
        
        **Syntax:** `SET key value [PX milliseconds]`
        
        **What it does:**
        - Stores a string value at the specified key
        - Overwrites any existing value
        - Optional PX parameter sets expiration in milliseconds
        
        **Use cases:**
        - User session storage: `SET session:abc123 "user_data" PX 3600000`
        - Caching API responses: `SET api:weather:NYC "sunny, 25°C" PX 1800000`
        - Feature flags: `SET feature:new_ui "enabled"`
        
        **Pro tip:** 💡 Use descriptive keys like `user:123:profile` for better organization!
        """,
        "example": "# Store a simple greeting\nSET greeting \"Hello Redis World!\"\n# Expected: OK\n\n# Store user profile data\nSET user:1001:name \"Alice Cooper\"\n# Expected: OK\n\n# Store temporary token with 30-second expiration\nSET session:temp:abc123 \"user_session_data\" PX 30000\n# Expected: OK (expires automatically after 30 seconds)\n\n# Store application configuration\nSET config:app:version \"2.1.0\"\n# Expected: OK"
    },
    
    "GET": {
        "description": """
        ## � GET
        
        **Purpose:** Retrieve the value of a key
        
        **Syntax:** `GET key`
        
        **What it does:**
        - Retrieves the string value stored at key
        - Returns (nil) if key doesn't exist or expired
        - Super fast O(1) lookup operation
        
        **Use cases:**
        - Reading user sessions: `GET session:abc123`
        - Fetching cached data: `GET api:weather:NYC`
        - Getting configuration: `GET app:version`
        
        **Real-world example:** 🌐 Netflix uses Redis GET for user preferences!
        """,
        "example": "# First set some values to retrieve\nSET greeting \"Hello Redis World!\"\nSET user:1001:name \"Alice Cooper\"\nSET config:app:version \"2.1.0\"\n\n# Now retrieve the values\nGET greeting\n# Expected: \"Hello Redis World!\"\n\nGET user:1001:name\n# Expected: \"Alice Cooper\"\n\nGET config:app:version\n# Expected: \"2.1.0\"\n\n# Try to get a non-existent key\nGET nonexistent_key\n# Expected: (nil)"
    },
    
    "KEYS": {
        "description": """
        ## � KEYS
        
        **Purpose:** Find all keys matching a pattern
        
        **Syntax:** `KEYS pattern`
        
        **What it does:**
        - Returns all keys matching the glob pattern
        - Supports wildcards: * (any chars), ? (single char)
        - Scans entire keyspace - use carefully in production!
        
        **Use cases:**
        - Development debugging: `KEYS *` (see all keys)
        - Finding user sessions: `KEYS session:*`
        - Cleanup operations: `KEYS temp:*`
        
        **Warning:** ⚠️ Avoid `KEYS *` on large databases - it can be slow!
        """,
        "example": "# First create some keys to find\nSET user:1001:name \"Alice\"\nSET user:1002:name \"Bob\"\nSET session:abc123 \"active\"\nSET temp:cache \"data\"\n\n# Find all keys (be careful in production!)\nKEYS *\n# Expected: All keys in database\n\n# Find all user keys\nKEYS user:*\n# Expected: [\"user:1001:name\", \"user:1002:name\"]\n\n# Find session keys with pattern matching\nKEYS session:*\n# Expected: [\"session:abc123\"]"
    },
    
    "INFO": {
        "description": """
        ## ℹ️ INFO
        
        **Purpose:** Get detailed server information
        
        **Syntax:** `INFO section`
        
        **What it does:**
        - Returns comprehensive server statistics
        - Currently supports 'replication' section
        - Shows role (master/slave), replication offset, and more
        
        **Use cases:**
        - Monitoring server health
        - Debugging replication issues
        - Performance analysis
        
        **Cool feature:** 📊 Real-time insight into your Redis instance!
        """,
        "example": "INFO replication"
    },
    
    "TYPE": {
        "description": """
        ## 🏷️ TYPE
        
        **Purpose:** Determine the data type of a key
        
        **Syntax:** `TYPE key`
        
        **What it does:**
        - Returns the data type: string, list, stream, or none
        - Helps you understand what operations are valid
        - Returns 'none' for non-existent keys
        
        **Use cases:**
        - Type checking before operations
        - Debugging data structure issues
        - Dynamic application logic
        
        **Data types supported:** 📝 string, list, stream - more coming soon!
        """,
        "example": "# First create keys of different types\nSET greeting \"Hello World!\"\nLPUSH mylist \"item1\" \"item2\"\nXADD mystream * field value\n\n# Check the types\nTYPE greeting\n# Expected: string\n\nTYPE mylist\n# Expected: list\n\nTYPE mystream\n# Expected: stream\n\nTYPE missing_key\n# Expected: none"
    },
    
    "INCR": {
        "description": """
        ## ➕ INCR
        
        **Purpose:** Atomically increment a number by 1
        
        **Syntax:** `INCR key`
        
        **What it does:**
        - Increments the number stored at key by 1
        - Creates key with value 1 if it doesn't exist
        - Atomic operation - thread-safe!
        - Returns error if value is not a number
        
        **Use cases:**
        - Page view counters: `INCR page:home:views`
        - User score tracking: `INCR user:123:points`
        - Rate limiting: `INCR api:calls:user:456`
        
        **Real magic:** ✨ Even with millions of concurrent users, INCR never loses count!
        """,
        "example": "# Initialize a counter (starts from 0)\nINCR visitor_count\n# Expected: 1 (created and incremented)\n\nINCR visitor_count\n# Expected: 2 (incremented again)\n\n# Track user score\nINCR user:1001:score\n# Expected: 1 (new user score)\n\n# Daily signup counter\nINCR daily_signups\n# Expected: 1 (tracking daily metrics)"
    },
    
    # LIST COMMANDS
    "LPUSH": {
        "description": """
        ## ⬅️ LPUSH
        
        **Purpose:** Push elements to the LEFT (head) of a list
        
        **Syntax:** `LPUSH key element [element ...]`
        
        **What it does:**
        - Inserts elements at the beginning of list
        - Creates list if it doesn't exist
        - Returns new length of list
        - Elements added in reverse order: LPUSH list a b c → [c, b, a]
        
        **Use cases:**
        - Recent activity feeds (newest first)
        - Undo operations stack
        - Chat message history
        
        **Think of it as:** 📚 Adding books to the LEFT side of a bookshelf!
        """,
        "example": "LPUSH notifications \"New message\"\n# Expected: (integer) 1\nLPUSH notifications \"Another message\"\n# Expected: (integer) 2\nLRANGE notifications 0 -1\n# Expected: 1) Another message 2) New message\nLPOP notifications\n# Expected: Another message"
    },
    
    "RPUSH": {
        "description": """
        ## ➡️ RPUSH
        
        **Purpose:** Push elements to the RIGHT (tail) of a list
        
        **Syntax:** `RPUSH key element [element ...]`
        
        **What it does:**
        - Appends elements to the end of list
        - Creates list if it doesn't exist
        - Returns new length of list
        - Perfect for queues (FIFO - First In, First Out)
        
        **Use cases:**
        - Task queues: `RPUSH jobs "process_image" "send_email"`
        - Log collection: `RPUSH logs "2023-10-31 14:30 User login"`
        - Event streams: `RPUSH events "click" "scroll" "purchase"`
        
        **Real-world example:** 🚚 Amazon uses similar patterns for order processing queues!
        """,
        "example": "RPUSH queue task1 task2 task3\n# Expected: (integer) 3\nLRANGE queue 0 -1\n# Expected: 1) task1 2) task2 3) task3\nRPOP queue\n# Expected: task3\nLLEN queue\n# Expected: (integer) 2"
    },
    
    "LPOP": {
        "description": """
        ## ⬅️🎯 LPOP
        
        **Purpose:** Remove and return the FIRST (leftmost) element
        
        **Syntax:** `LPOP key [count]`
        
        **What it does:**
        - Removes element from the head of list
        - Returns the removed element
        - With count: returns array of removed elements
        - Returns (nil) if list is empty
        
        **Use cases:**
        - Processing LIFO stacks (Last In, First Out)
        - Getting latest notifications
        - Undo operations
        
        **Perfect combo:** 🔄 LPUSH + LPOP = Stack behavior!
        """,
        "example": "LPOP notifications\nLPOP activity 2\nLPOP empty_list"
    },
    
    "RPOP": {
        "description": """
        ## ➡️🎯 RPOP
        
        **Purpose:** Remove and return the LAST (rightmost) element
        
        **Syntax:** `RPOP key [count]`
        
        **What it does:**
        - Removes element from the tail of list
        - Returns the removed element
        - With count: returns array of removed elements
        - Returns (nil) if list is empty
        
        **Use cases:**
        - Processing FIFO queues (First In, First Out)
        - Task processing systems
        - Getting oldest items first
        
        **Perfect combo:** 🔄 RPUSH + RPOP = Queue behavior!
        """,
        "example": "RPOP queue\nRPOP logs 3\nRPOP playlist"
    },
    
    "LLEN": {
        "description": """
        ## 📏 LLEN
        
        **Purpose:** Get the length (size) of a list
        
        **Syntax:** `LLEN key`
        
        **What it does:**
        - Returns the number of elements in the list
        - Returns 0 if list doesn't exist
        - O(1) operation - super fast!
        
        **Use cases:**
        - Queue size monitoring: `LLEN task_queue`
        - Pagination calculations: Check list size before display
        - Capacity planning: Monitor growth trends
        
        **Performance tip:** 🚀 LLEN is always O(1) regardless of list size!
        """,
        "example": "LLEN notifications\nLLEN task_queue\nLLEN empty_list"
    },
    
    "LRANGE": {
        "description": """
        ## 📋 LRANGE
        
        **Purpose:** Get a range of elements from a list
        
        **Syntax:** `LRANGE key start stop`
        
        **What it does:**
        - Returns elements from start to stop (inclusive)
        - 0-based indexing: 0 is first element
        - Negative indices: -1 is last, -2 is second-to-last
        - `LRANGE mylist 0 -1` gets ALL elements
        
        **Use cases:**
        - Pagination: `LRANGE posts 0 9` (first 10 posts)
        - Recent items: `LRANGE activity -5 -1` (last 5 activities)
        - Debugging: `LRANGE mylist 0 -1` (see everything)
        
        **Pro tip:** 💡 Use negative indices to get recent items without knowing list size!
        """,
        "example": "RPUSH mylist item1 item2 item3 item4 item5\n# Expected: (integer) 5\nLRANGE mylist 0 4\n# Expected: 1) item1 2) item2 3) item3 4) item4 5) item5\nLRANGE mylist 0 2\n# Expected: 1) item1 2) item2 3) item3\nLRANGE mylist -2 -1\n# Expected: 1) item4 2) item5"
    },
    
    "BLPOP": {
        "description": """
        ## ⏰ BLPOP
        
        **Purpose:** Blocking LPOP - wait for element to arrive
        
        **Syntax:** `BLPOP key timeout`
        
        **What it does:**
        - Like LPOP but waits if list is empty
        - Blocks until element available or timeout reached
        - timeout=0 means wait forever
        - Returns [key, element] when successful
        
        **Use cases:**
        - Worker processes waiting for jobs
        - Real-time event processing
        - Producer-consumer patterns
        
        **Magic moment:** ✨ Perfect for building responsive real-time systems!
        """,
        "example": "BLPOP task_queue 0\nBLPOP notifications 5\nBLPOP jobs 30"
    },
    
    # STREAM COMMANDS
    "XADD": {
        "description": """
        ## 🌊 XADD
        
        **Purpose:** Add structured entries to a Redis Stream with automatic timestamping
        
        **Syntax:** `XADD key id field value [field value ...]`
        
        **What it does:**
        - Adds structured entries to a stream (creates stream if it doesn't exist)
        - Each entry has a unique ID and key-value field pairs
        - Automatically maintains chronological order
        - Returns the generated or specified entry ID
        
        **Entry ID Formats:**
        - `*` - Auto-generate complete ID using current time: `1698765432000-0`
        - `1698765432000-*` - Auto-generate sequence number: `1698765432000-0`
        - `1698765432000-5` - Explicit ID (must be greater than last entry)
        
        **ID Structure:** `<millisecondsTime>-<sequenceNumber>`
        - Time part: Unix timestamp in milliseconds
        - Sequence: Starts at 0, increments for same timestamp
        - Minimum valid ID: `0-1` (must be greater than `0-0`)
        
        **Validation Rules:**
        - New ID must be strictly greater than the last entry's ID
        - If stream is empty, ID must be greater than `0-0`
        - Error if ID <= last entry: "ERR The ID specified in XADD is equal or smaller than the target stream top item"
        
        **Use cases:**
        - **Event Logging:** User actions, system events, audit trails
        - **IoT Data:** Sensor readings with automatic timestamps
        - **Time Series:** Metrics, monitoring data, analytics
        - **Activity Feeds:** Social media, notifications, real-time updates
        
        **Real-world examples:** 🌍
        - **Banking:** Transaction logs with timestamps
        - **E-commerce:** Order events from creation to delivery
        - **Gaming:** Player actions and achievements
        - **Monitoring:** System metrics and alerts
        
        **Stream Structure:** 📚
        ```
        sensor_data:
          1698765432000-0: {temperature: 23.5, humidity: 65}
          1698765432100-0: {temperature: 23.7, humidity: 64}
          1698765432200-0: {temperature: 23.9, humidity: 63}
        ```
        """,
        "example": "# Create sensor data stream with timestamp-based entries\nXADD sensors * temperature 23.5 humidity 65 location office\n# Expected: \"1698765432000-0\" (auto-generated timestamp ID)\n\nXADD sensors * temperature 24.1 humidity 62 location office\n# Expected: \"1698765432100-0\" (next timestamp)\n\n# Add event log with explicit ID\nXADD events 1000-0 user alice action login timestamp 1698765432\n# Expected: \"1000-0\" (explicit ID)\n\n# Add error log with structured data\nXADD logs * level error message \"Database connection timeout\" service auth-api retry 3\n# Expected: \"1698765433000-0\" (new event with structured fields)"
    },
    
    "XRANGE": {
        "description": """
        ## 🔍 XRANGE
        
        **Purpose:** Query and retrieve stream entries within a specified range
        
        **Syntax:** `XRANGE key start end [COUNT count]`
        
        **What it does:**
        - Returns entries between start and end IDs (inclusive range)
        - Retrieves structured data with timestamps in chronological order
        - Supports partial IDs and special range indicators
        - Perfect for time-based queries and data analysis
        
        **Range Specifiers:**
        - `start` and `end`: Full IDs like `1526985054069-0`
        - Partial IDs: `1526985054069` (sequence defaults: start=0, end=max)
        - `-`: Start from the very beginning of the stream
        - `+`: Continue to the very end of the stream
        - `XRANGE mystream - +`: Get ALL entries
        
        **Response Format:**
        ```
        [
          ["1526985054069-0", ["temperature", "36", "humidity", "95"]],
          ["1526985054079-0", ["temperature", "37", "humidity", "94"]]
        ]
        ```
        
        **Use cases:**
        - **Time-based queries:** Get events from last hour
        - **Pagination:** Retrieve data in chunks with COUNT
        - **Analytics:** Analyze patterns over time periods
        - **Debugging:** Examine specific time ranges
        - **Reporting:** Generate historical data reports
        
        **Query Patterns:** 📊
        - **Recent data:** `XRANGE logs - + COUNT 10`
        - **Time range:** `XRANGE metrics 1698700000000 1698800000000`
        - **Debug all:** `XRANGE events - +`
        - **Pagination:** `XRANGE data 1698765400000 + COUNT 100`
        
        **Real-world examples:** 🕰️
        - **System monitoring:** Query metrics from last 5 minutes
        - **User analytics:** Get user actions for a specific day
        - **IoT data:** Retrieve sensor readings between timestamps
        - **Audit logs:** Examine security events in a time window
        """,
        "example": "# First add some data to query\nXADD sensors * temperature 23.5 humidity 65\nXADD sensors * temperature 24.1 humidity 62\nXADD sensors * temperature 25.0 humidity 60\n\n# Query all entries from beginning to end\nXRANGE sensors - +\n# Expected: All entries with their IDs and field-value pairs\n\n# Get only the first 2 entries\nXRANGE sensors - + COUNT 2\n# Expected: First 2 entries only\n\n# Query specific time range (using actual IDs from XADD responses)\nXRANGE sensors 1698765400000 1698765500000\n# Expected: Entries within the 100-second time window"
    },
    
    "XREAD": {
        "description": """
        ## 📖 XREAD
        
        **Purpose:** Read from streams with optional blocking for real-time data consumption
        
        **Syntax:** `XREAD [BLOCK milliseconds] STREAMS key [key ...] id [id ...]`
        
        **What it does:**
        - Reads entries from one or more streams starting after specified IDs
        - EXCLUSIVE operation: returns entries with IDs greater than specified
        - Can block and wait for new data to arrive (real-time streaming)
        - Supports reading from multiple streams simultaneously
        
        **Key Features:**
        - **Non-blocking mode:** Get existing data immediately
        - **Blocking mode:** Wait for new data (BLOCK milliseconds)
        - **Multiple streams:** Monitor several streams at once
        - **Cursor-based:** Use last seen ID to continue reading
        
        **Special ID Values:**
        - `0` or `0-0`: Read from the beginning of the stream
        - `$`: Read only NEW entries (nothing initially, then new ones)
        - Specific ID: Read entries after this ID
        
        **Response Format:**
        ```
        [
          ["stream1", [
            ["1526985054069-0", ["field1", "value1", "field2", "value2"]]
          ]],
          ["stream2", [...]]
        ]
        ```
        
        **Use cases:**
        - **Real-time processing:** React to events as they happen
        - **Live dashboards:** Display streaming data updates
        - **Event-driven systems:** Trigger actions on new events
        - **Data pipelines:** Process streams continuously
        - **Chat systems:** Get new messages in real-time
        
        **Blocking vs Non-blocking:** ⚡
        - **Non-blocking:** `XREAD STREAMS mystream 0` (immediate response)
        - **Blocking:** `XREAD BLOCK 1000 STREAMS mystream $` (wait up to 1 second)
        
        **Real-time patterns:** 🔄
        - **Event processor:** Block forever waiting for events
        - **Live monitoring:** Poll with short timeouts
        - **Batch + Real-time:** Read history, then switch to blocking
        
        **Multi-stream example:** 🌊
        ```
        XREAD BLOCK 0 STREAMS orders users notifications $ $ $
        # Monitor 3 streams simultaneously for new events
        ```
        """,
        "example": "# First add some data to read\nXADD activity * user alice action login timestamp 1698765432\nXADD activity * user bob action purchase item laptop\n\n# Read all existing entries from beginning\nXREAD STREAMS activity 0\n# Expected: All existing entries after ID 0-0\n\n# Wait for new entries (use $ to read only new data)\nXREAD BLOCK 1000 STREAMS activity $\n# Expected: Waits 1 second for new entries, returns new data or null\n\n# Monitor multiple streams simultaneously\nXREAD STREAMS activity sensors $ $\n# Expected: New entries from both streams (if any)"
    },
    
    # TRANSACTION COMMANDS
    "MULTI": {
        "description": """
        ## � MULTI
        
        **Purpose:** Start a transaction block
        
        **Syntax:** `MULTI`
        
        **What it does:**
        - Begins a transaction
        - Following commands are queued, not executed immediately
        - Use EXEC to execute all queued commands atomically
        - Use DISCARD to cancel the transaction
        
        **Transaction Flow:**
        1. MULTI - Start transaction
        2. Command1, Command2, ... - Queue commands (each returns "QUEUED")
        3. EXEC - Execute all queued commands atomically
        
        **Atomicity guarantee:** 🛡️ 
        - Either ALL commands succeed, or if one fails, the transaction is aborted
        - No other client can interfere during execution
        - Perfect for operations that must happen together (like transferring money)
        
        **Use cases:**
        - Financial transactions: Transfer money between accounts
        - Batch updates: Update multiple related records
        - Race condition prevention: Ensure consistent state
        - Inventory management: Reserve items atomically
        
        **Real-world example:** 💰 
        Bank transfer - debit one account and credit another in a single atomic operation!
        """,
        "example": "MULTI\n# Expected: OK\nSET account:alice 100\n# Expected: QUEUED\nSET account:bob 50\n# Expected: QUEUED\nINCR transaction:count\n# Expected: QUEUED\nEXEC\n# Expected: 1) OK 2) OK 3) (integer) 1\nGET account:alice\n# Expected: \"100\""
    },
    
    "EXEC": {
        "description": """
        ## ✅ EXEC
        
        **Purpose:** Execute all queued commands in the current transaction
        
        **Syntax:** `EXEC`
        
        **What it does:**
        - Executes ALL commands queued since MULTI in order
        - Returns an array containing the response of each executed command
        - Ends the transaction (back to normal mode)
        - If called without MULTI, returns error: "ERR EXEC without MULTI"
        - If transaction is empty (no commands queued), returns empty array
        
        **Response format:**
        - Array of results: Each element is the response from the corresponding queued command
        - Order preserved: Results match the order commands were queued
        - Empty array `[]` if no commands were queued
        
        **Atomicity:** ⚡
        - All commands execute as a single atomic operation
        - No other commands from other clients can interfere
        - If any command fails, previous ones in the transaction still completed
        
        **Use cases:**
        - Complete financial transfers
        - Batch data updates
        - Multi-step operations that must be consistent
        
        **Transaction example:** 💸
        ```
        MULTI          # Start transaction
        SET account:1 90   # Debit $10 (was $100)
        SET account:2 110  # Credit $10 (was $100)  
        EXEC           # Execute both atomically
        # Returns: [OK, OK]
        ```
        """,
        "example": "EXEC\n# Expected: Array of results from queued commands\n# Or empty array if no commands were queued\n# Or error if MULTI wasn't called first"
    },
    
    "DISCARD": {
        "description": """
        ## ❌ DISCARD
        
        **Purpose:** Cancel the current transaction and discard all queued commands
        
        **Syntax:** `DISCARD`
        
        **What it does:**
        - Cancels the current transaction started with MULTI
        - Discards ALL queued commands without executing them
        - Returns the client to normal (non-transaction) mode
        - Returns "OK" if successful
        - Returns error "ERR DISCARD without MULTI" if no transaction is active
        
        **When to use DISCARD:**
        - Error conditions detected before EXEC
        - User cancellation of pending operations
        - Logic requires aborting the transaction
        - Recovery from invalid states
        
        **Safety net:** 🛡️ 
        Changed your mind? Need to abort? DISCARD safely cancels everything!
        
        **Example scenario:** 🏪
        E-commerce checkout - if credit card validation fails, DISCARD the transaction 
        instead of executing the inventory reservation and order creation.
        
        **Use cases:**
        - Error recovery: Cancel when validation fails
        - User cancellation: Shopping cart abandonment
        - Logic branching: Conditional transaction execution
        - Resource cleanup: Prevent partial operations
        
        **Transaction lifecycle:**
        ```
        MULTI              # Start transaction
        SET item:123 reserved  # Queue command
        INCR inventory:sold    # Queue command
        # ... error detected ...
        DISCARD            # Cancel everything safely
        # Returns: OK
        # No commands were executed!
        ```
        """,
        "example": "DISCARD\n# Expected: OK (if in transaction)\n# Or error if no transaction is active\n# All queued commands are discarded safely"
    },
    
    # PUB/SUB COMMANDS
    "SUBSCRIBE": {
        "description": """
        ## 📻 SUBSCRIBE
        
        **Purpose:** Subscribe to channels for real-time message delivery
        
        **Syntax:** `SUBSCRIBE channel [channel ...]`
        
        **What it does:**
        - Registers the client as a subscriber to one or more channels
        - Puts client in "subscribed mode" with restricted command set
        - Receives all messages published to subscribed channels instantly
        - Returns subscription confirmation with channel count
        
        **Response Format:**
        ```
        ["subscribe", "channel_name", subscription_count]
        ```
        - Element 1: "subscribe" (confirmation type)
        - Element 2: Channel name that was subscribed to
        - Element 3: Total number of channels this client is subscribed to
        
        **Subscribed Mode Restrictions:** 🔒
        Once subscribed, only these commands are allowed:
        - SUBSCRIBE / UNSUBSCRIBE
        - PING (returns different format: ["pong", ""])
        - QUIT
        - Other commands return error: "ERR Can't execute 'command'"
        
        **Multiple Subscriptions:**
        - Can subscribe to multiple channels: `SUBSCRIBE news sports tech`
        - Can subscribe multiple times (count stays same for duplicate channels)
        - Each subscription returns individual confirmation
        
        **Use cases:**
        - **Chat systems:** Real-time messaging rooms
        - **Live notifications:** User-specific alerts and updates
        - **Event broadcasting:** System-wide announcements
        - **Real-time dashboards:** Live data updates
        - **Gaming:** Real-time multiplayer events
        
        **Real-world patterns:** 🌍
        - **Chat rooms:** `SUBSCRIBE chat:room123 chat:room456`
        - **User notifications:** `SUBSCRIBE user:alice:notifications`
        - **System alerts:** `SUBSCRIBE alerts:critical alerts:warning`
        - **Live sports:** `SUBSCRIBE sports:nfl:scores sports:nba:scores`
        
        **Important notes:** ⚠️
        - Client stays in subscribed mode until all channels are unsubscribed
        - Messages are delivered in real-time as they're published
        - No message history - only receives messages published after subscription
        """,
        "example": "# Subscribe to a news channel\nSUBSCRIBE news\n# Expected: [\"subscribe\", \"news\", 1] (subscribed to 1 channel)\n\n# Subscribe to multiple chat rooms\nSUBSCRIBE chat:room1 chat:room2 notifications\n# Expected: Three subscription confirmations:\n# [\"subscribe\", \"chat:room1\", 2]\n# [\"subscribe\", \"chat:room2\", 3] \n# [\"subscribe\", \"notifications\", 4]\n\n# Now you'll receive messages published to these channels:\n# When someone publishes: PUBLISH news \"Breaking story!\"\n# You'll receive: [\"message\", \"news\", \"Breaking story!\"]"
    },
    
    "UNSUBSCRIBE": {
        "description": """
        ## 📻❌ UNSUBSCRIBE
        
        **Purpose:** Unsubscribe from channels
        
        **Syntax:** `UNSUBSCRIBE [channel [channel ...]]`
        
        **What it does:**
        - Unsubscribes from specified channels
        - If no channels specified, unsubscribes from all
        - Stops receiving messages from those channels
        - Reduces network traffic
        
        **Use cases:**
        - Leave chat rooms
        - Stop notifications
        - Clean up subscriptions
        
        **Clean exit:** 🚪 Stop listening when you don't need updates anymore!
        """,
        "example": "UNSUBSCRIBE news\nUNSUBSCRIBE chat:room1\nUNSUBSCRIBE"
    },
    
    "PUBLISH": {
        "description": """
        ## 📢 PUBLISH
        
        **Purpose:** Broadcast messages to all subscribers of a channel
        
        **Syntax:** `PUBLISH channel message`
        
        **What it does:**
        - Sends message to ALL clients currently subscribed to the channel
        - Returns the number of subscribers that received the message
        - Instant, real-time delivery to active subscribers
        - Fire-and-forget: no acknowledgment from subscribers required
        
        **Response:** Returns integer count of subscribers who received the message
        - `0`: No subscribers (message sent to void)
        - `5`: Message delivered to 5 active subscribers
        
        **Message Delivery Format (to subscribers):**
        ```
        ["message", "channel_name", "message_content"]
        ```
        - Element 1: "message" (indicates published message)
        - Element 2: Channel name where message was published
        - Element 3: The actual message content
        
        **Broadcasting Behavior:**
        - Messages are delivered immediately to active subscribers
        - No message persistence - if no one is listening, message is lost
        - Subscribers receive messages in the order they were published
        - Each subscriber gets a complete copy of the message
        
        **Use cases:**
        - **Real-time chat:** Broadcast messages to chat room participants
        - **System notifications:** Alert all users about maintenance or updates
        - **Live events:** Sports scores, stock prices, breaking news
        - **Gaming:** Real-time game state updates to all players
        - **IoT coordination:** Send commands to multiple connected devices
        
        **Messaging patterns:** 📡
        - **Fan-out:** One publisher, many subscribers
        - **Event broadcasting:** System-wide announcements
        - **Real-time updates:** Live data feeds
        - **Notification systems:** User alerts and messages
        
        **Real-world examples:** 🌍
        - **Slack/Discord:** Chat message broadcasting
        - **Trading platforms:** Real-time price updates
        - **Live sports apps:** Score updates to millions of users
        - **Breaking news:** Instant alerts to news app users
        
        **Performance notes:** ⚡
        - Very fast - optimized for real-time delivery
        - Scales with number of subscribers
        - No storage overhead (fire-and-forget)
        """,
        "example": "# Broadcast a breaking news alert\nPUBLISH news \"BREAKING: New Redis features announced!\"\n# Expected: 15 (if 15 clients are subscribed to 'news' channel)\n\n# Send message to a specific chat room\nPUBLISH chat:room1 \"Hello everyone in room 1!\"\n# Expected: 3 (if 3 clients are in chat:room1)\n\n# System-wide alert to all subscribed clients\nPUBLISH alerts \"Scheduled maintenance in 10 minutes - please save your work\"\n# Expected: 127 (if 127 clients subscribed to alerts)\n\n# Publishing to channel with no subscribers\nPUBLISH empty:channel \"Nobody listening\"\n# Expected: 0 (no subscribers, message is lost)"
    },
    
    # REPLICATION & ADMIN COMMANDS
    "CONFIG": {
        "description": """
        ## ⚙️ CONFIG
        
        **Purpose:** Get server configuration parameters
        
        **Syntax:** `CONFIG GET parameter`
        
        **What it does:**
        - Retrieves configuration parameter values
        - Shows server settings and runtime parameters
        - Useful for debugging and monitoring
        
        **Use cases:**
        - Server administration
        - Configuration verification
        - Debugging server behavior
        
        **Behind the scenes:** 🎛️ Peek into your Redis server's configuration!
        """,
        "example": "CONFIG GET dir\nCONFIG GET dbfilename"
    },
    
    "COMMAND": {
        "description": """
        ## 📝 COMMAND
        
        **Purpose:** Get information about Redis commands
        
        **Syntax:** `COMMAND`
        
        **What it does:**
        - Returns information about available commands
        - Shows command metadata and documentation
        - Useful for introspection and tooling
        
        **Use cases:**
        - Building Redis clients
        - Command discovery
        - Documentation generation
        
        **Meta command:** 🤖 A command that tells you about commands!
        """,
        "example": "COMMAND"
    }
}