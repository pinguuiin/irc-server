# ft_irc — Architecture, Workflow, and Task Dispatch

This document describes how the IRC server is structured, how a request moves through the system at runtime, and how work is split across three developers. The authoritative public APIs live in `ft_irc/include/*.hpp` (frozen unless the team agrees to change them).

**Toolchain:** The team may standardize on **C++20** (e.g. `-std=c++20`) in the `ft_irc` Makefile when everyone is ready; the scaffold may still compile with stricter flags until you flip the standard.

---

## 1. Project architecture

### 1.1 Layout

All product code lives under `ft_irc/`:

- **`include/`** — Headers only: `Server`, `Client`, `Channel`, `CommandParser`, `CommandHandler`, `Utils` (include guards, no `using namespace std` in headers).
- **`src/`** — Implementations plus `main.cpp` (argv: `<port> <password>`, constructs `Server`, calls `run()`).
- **`Makefile`** — Builds binary `ircserv`.

### 1.2 Runtime model

Single **process**, **single-threaded** event loop: one **listening socket** and many **client sockets** are monitored together (e.g. `poll()`). I/O is **non-blocking**; partial reads/writes are buffered on each `Client`. There is no requirement for worker threads; keep shared state on `Server` and coordinate only from the poll thread.

### 1.3 Components (who talks to whom)

| Piece | Role |
|--------|------|
| **Server** | Owns port/password, poll loop, client FD → `Client*` map, channel name → `Channel*` map. Accepts connections, registers FDs, reads/writes when ready, tears down on disconnect. Exposes `addClient` / `removeClient` / `getClient` / `getChannel` / `createChannel` / `getPassword`. |
| **Client** | One TCP connection: recv buffer, line framing (`receiveData` / `getNextMessage`), send queue (`sendMessage`), IRC registration state (nick/user, `authenticate`). Holds `Server*` for callbacks and lookups. |
| **Channel** | Topic, modes (invite-only, key, user limit), members, operators, `broadcastMessage`. |
| **CommandParser** | IRC line → `ParsedCommand` (`command`, `params`, `trailing`); optional `validateCommand`. |
| **CommandHandler** | Given `Client*` + `ParsedCommand`, dispatches PASS/NICK/USER/JOIN/PRIVMSG/KICK/INVITE/TOPIC/MODE (and any extensions you add privately). Uses `Server` to resolve clients/channels. |
| **Utils** | Shared string helpers and validation (`isValidNickname`, `isValidChannelName`, etc.). |

**Suggested wiring (implementation detail, not frozen):** `Server` constructs or holds a `CommandHandler` and, after `Client::getNextMessage()` returns a full line, runs `CommandParser::parse` then `CommandHandler::handleCommand`.

### 1.4 Conceptual diagram

```text
  main
    └── Server::run()
            poll/listen
            ├── accept → addClient → new Client(fd, ip, this)
            ├── readable client fd → read → client.receiveData() → … → getNextMessage()
            │         └── CommandParser::parse → CommandHandler::handleCommand
            │                 ├── Server / Client / Channel public APIs
            │                 └── Utils
            └── writable client fd → flush outbound queue (where you implement it)
```

---

## 2. Execution workflow (end-to-end)

1. **Startup:** `main` parses arguments, creates `Server(port, password)`, calls `run()`.
2. **Listen:** Server creates listening socket, binds to port, adds listen FD to poll set.
3. **Accept:** On listen FD readable, `accept` → `addClient(fd, ip)` → allocate `Client`, store in map by fd, add client fd to poll set.
4. **Read:** On client FD readable, read into a stack or heap buffer, append via `Client::receiveData`. Loop: while `getNextMessage()` returns non-empty, treat each as one IRC message (trailing `\r\n` stripped in parser or client layer — team convention).
5. **Dispatch:** `CommandParser::parse` → if needed `validateCommand` → `CommandHandler::handleCommand`. Handler uses `Server::getPassword`, `getClient`, `getChannel`, `createChannel`, and `Client`/`Channel` methods; sends replies with `Client::sendMessage`.
6. **Write:** When a client FD is writable, drain that client’s outbound buffer (non-blocking `send`); if incomplete, wait for next writable event.
7. **Disconnect / errors:** Remove fd from poll set, `removeClient`, destroy `Client`; update or destroy `Channel` membership as needed.
8. **Shutdown:** `stop()` sets a flag; `run()` exits loop, closes fds, frees clients/channels.

---

## 3. Task dispatch (three people)

Boundaries are **by file** to reduce merge conflicts. **Do not change another member’s files without agreement.** Changes to **public** method signatures in headers need **team consensus**.

### Member 1 — Server core & I/O

**Owns:** `Server.hpp`, `Server.cpp`, `main.cpp` (and poll/listen/accept/read/write integration).

**Scope:** Listening socket, `poll` (or equivalent), non-blocking I/O, calling `addClient` / `removeClient`, driving read/write paths, constructing top-level objects (e.g. `CommandHandler`), integrating parser/handler into the loop.

**Uses only:** public APIs of `Client`, `Channel`, `CommandParser`, `CommandHandler`, `Utils`.

**Avoid:** implementing IRC command semantics inside `Server` (delegate to `CommandHandler`).

---

### Member 2 — Client session & registration

**Owns:** `Client.hpp`, `Client.cpp`.

**Scope:** Per-connection buffers, `getNextMessage` framing, `sendMessage` buffering and back-pressure, nick/user state, `authenticate` rules in coordination with handler (PASS/NICK/USER), disconnect cleanup hooks.

**Shared:** **`Utils`** — Member 2 is **primary owner** of `Utils.hpp` / `Utils.cpp` for nick/channel validation and string helpers; small additions by others go through PR + Member 2 review.

**Avoid:** owning the poll loop or channel mode logic.

---

### Member 3 — Channels, parsing, commands

**Owns:** `Channel.hpp`, `Channel.cpp`, `CommandParser.hpp`, `CommandParser.cpp`, `CommandHandler.hpp`, `CommandHandler.cpp`.

**Scope:** Channel membership/operators/topic/modes, full `CommandHandler` dispatch table, numeric replies and broadcast rules, `CommandParser` IRC grammar.

**Uses:** `Server`, `Client`, `Channel`, `Utils` **via public interfaces only**.

**Avoid:** replacing Member 1’s poll/accept code or Member 2’s buffer strategy without coordination.

---

### Integration checkpoints (all three)

| Checkpoint | Owner lead | What “done” means |
|------------|------------|-------------------|
| Client construction from accept | 1 + 2 | `addClient` creates `Client` with correct `fd`, `ip`, `Server*`. |
| Full line to handler | 2 + 1 | `getNextMessage` + Server loop deliver one message per call. |
| PASS/NICK/USER vs password | 2 + 3 | Handler calls `getPassword` / client setters; client reflects authenticated state. |
| JOIN / channel lifecycle | 1 + 3 | `createChannel` / `getChannel` used; channel list consistent on part/quit. |
| PRIVMSG / KICK / MODE | 3 | Correct targets, operators, replies. |

---

### Makefile and standard

- Any switch to **C++20** should be one commit to `ft_irc/Makefile` plus a quick note in chat so everyone rebases and fixes warnings.
- Keep **`-Wall -Wextra -Werror`** unless the team explicitly relaxes a flag for a known third-party issue (you have none today).

---

This file is intentionally short: **architecture** (§1), **runtime workflow** (§2), **ownership and handoffs** (§3). For exact method names and signatures, use the headers in `ft_irc/include/`.
