#ifndef SUPPORTEDCOMMANDS_H
#define SUPPORTEDCOMMANDS_H


/* Supported commands */
#define PING "ping"
#define ECHO "echo"
#define COMMAND "command"
#define SET "set"
#define GET "get"
#define CONFIG "config"
#define SAVE "save"
#define KEYS "keys"
#define INFO "info"
#define REPLCONF "replconf"
#define PSYNC "psync"
#define WAIT "wait"
#define TYPE "type"
#define XADD "xadd"
#define XRANGE "xrange"
#define XREAD "xread"
#define INCR "incr"
#define MULTI "multi"
#define EXEC "exec"
#define DISCARD "discard"
#define LPOP "lpop"
#define RPOP "rpop"
#define LPUSH "lpush"
#define RPUSH "rpush"
#define LRANGE "lrange"
#define LLEN "llen"
#define BLPOP "blpop"
#define SUBSCRIBE "subscribe"
#define UNSUBSCRIBE "unsubscribe"
#define PUBLISH "publish"

// Some other utility defines
#define NULL_BULK_ENCODED "$-1\r\n"
#define STREAMS "streams"
#define NO_REPLY "NOREPLY"

#endif // SUPPORTEDCOMMANDS_H

