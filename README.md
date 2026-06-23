# irc-server

An **Internet Relay Chat (IRC) server** written in C++, built as part of the 42 school curriculum.

## About

In this project we implement our own IRC server compatible with a standard IRC client (e.g. `irssi`). The server handles multiple clients simultaneously using **non-blocking I/O** and a single `epoll()` (or equivalent) call.

## Requirements

- C++ compiler supporting C++20
- `make`
- A reference IRC client (we used `irssi` for testing)
- Linux or macOS

## Build & Run

```bash
make
./ircserv <port> <password>
```

- `<port>` - the port the server listens on
- `<password>` - the connection password required by clients

Example:

```bash
./ircserv 6667 pass
```

Then connect with a client:

```bash
irssi -c localhost -p 6667 -w pass -n yournickname
```

## Features

- Authentication with password (`PASS`)
- Setting nickname and username (`NICK`, `USER`)
- Joining channels (`JOIN`)
- Sending private messages to users and channels (`PRIVMSG`)
- Channel operator commands:
  - `KICK` - Eject a user from a channel
  - `INVITE` - Invite a user to a channel
  - `TOPIC` - View or change the channel topic
  - `MODE` - Manage channel modes:
    - `i`: Set/remove Invite-only channel
    - `t`: Set/remove the restrictions of the TOPIC command to channel operators
    - `k`: Set/remove the channel key (password)
    - `o`: Give/take channel operator privilege
    - `l`: Set/remove the user limit to channel

## Technical Highlights

- **Non-blocking sockets** with a single `epoll()` loop that handles multiple clients simultaneously
- **Read/write from a stream** messages are read/written into a buffer, and processed when encounter the `\r\n` or `\n` at the end
- **Standard return code** all return/error codes and messages follow the IRC protocol specs (RFC 1459/2812)
- **Robust Authentication** bulletproof handling of client authentication and disconnections

## Authors

- Ping
- Daniel
- Abdel
