#include "../include/Client.hpp"
#include "../include/Server.hpp"
#include "../include/CommandParser.hpp"
#include "../include/CommandHandler.hpp"
#include <iostream>

#include <sys/socket.h>	// for send()
#include <cerrno>		// for errno, EAGAIN, EWOULDBLOCK
#include <iostream>
#include <cstring>		// for strerror()

Client::Client(int fd, std::string ip, Server* server)
	: _fd(fd), _ip(ip), _server(server), _passOk(false), _nickSet(false),
	_userSet(false), _authenticated(false)
{
	(void)_server;
}

// Intentionally empty - Server is responsible for closing _fd when client disconnects.
Client::~Client()
{
}

// Getters
const int& Client::getFd() const
{
	return _fd;
}

const std::string& Client::getIp() const
{
	return _ip;
}

const std::string& Client::getNickname() const
{
	return _nickname;
}

const std::string& Client::getUsername() const
{
	return _username;
}

Server* Client::getServer() const
{
    return _server;
}

// Setters
void Client::setFd(int fd)
{
	_fd = fd;
}

void Client::setIp(std::string ip)
{
	_ip = ip;
}

void Client::setNickname(std::string nick)
{
	_nickname = nick;
}

void Client::setUsername(std::string user)
{
	_username = user;
}

bool Client::isAuthenticated() const
{
	return _authenticated;
}

bool Client::isPassOk() const
{
	return _passOk;
}

bool Client::isNickSet() const
{
	return _nickSet;
}

bool Client::isUserSet() const
{
	return _userSet;
}

void Client::setPassOk()
{
	_passOk = true;
}

void Client::setNickSet()
{
	_nickSet = true;
}

void Client::setUserSet()
{
	_userSet = true;
}

// Called by CommandHandler once PASS + NICK + USER have all been received and validated.
void Client::authenticate()
{
	_authenticated = true;
}


// ── Send Message ────────────────────────────────────────────────────
// Queues 'msg' for sending and immediately tries to flush the queue.
//
// WHY a buffer at all?
//   The socket is non-blocking.  send() may not send every byte in one
//   call (e.g. the kernel's send-buffer is full).  We store whatever
//   didn't go out in _sendBuffer and try again next time sendMessage()
//   is called.  For IRC (messages ≤ 512 bytes) this almost never
//   matters in practice, but it's the correct approach.
//
// HOW the flush loop works:
//   1. Append the new message to _sendBuffer.
//   2. Keep calling send() until the buffer is empty or send() blocks.
//   3. If send() says EAGAIN/EWOULDBLOCK the socket isn't ready yet —
//      just return and leave the data in _sendBuffer for next time.
//   4. If send() returns a real error, print it and give up.  In a
//      production server you'd also disconnect the client here.
void Client::appendSendBuffer(const std::string& msg)
{
	// Step 0 - queue the new data
	_sendBuffer += msg; // Append to send buffer
}

void Client::sendPendingMessage()
{
	// Step 1 - try to drain the queue
	while (!_sendBuffer.empty())
	{
		 // send() returns how many bytes it actually sent.
        // MSG_NOSIGNAL prevents the process from dying on a broken pipe
        // (happens when the client disconnects mid-send on Linux).
        ssize_t sent = send(_fd, _sendBuffer.c_str(), _sendBuffer.size(), MSG_NOSIGNAL);

        if (sent < 0)
        {
            // Step 2 — socket buffer is full; try again later
            if (errno == EAGAIN || errno == EWOULDBLOCK)
                return;

            // Step 3 — real error (e.g. client hung up)
            std::cerr << "Client fd=" << _fd
                      << ": send() error: " << std::strerror(errno) << "\n";
            return;
        }

        // Erase the bytes that were successfully sent.
        // substr from 'sent' keeps whatever wasn't sent yet.
        _sendBuffer.erase(0, static_cast<size_t>(sent));
	}
}

// ── Receive and Handle Message ─────────────────────────────────────
// Server::receiveMessage() reads raw bytes from the socket and passes
// them here as a std::string chunk.  We just append to _recvBuffer;
// the actual line-framing happens in getNextMessage().
//
// Example: two recv() calls might give us:
//   chunk 1: "NICK alice\r\nUSER ali"
//   chunk 2: "ce 0 * :Alice\r\n"
// After both calls _recvBuffer holds the full two lines, ready to be
// extracted one at a time by getNextMessage().
void Client::receiveAndHandleMessage(const char *buf)
{
    _recvBuffer += std::string(buf);
    std::string msg;

    while (!(msg = getNextMessage()).empty())
	{
        const CommandParser::ParsedCommand& cmd = CommandParser::parse(msg);
        // ignore invalid commands with empty command name
        if (!CommandParser::validateCommand(cmd))
            return;
        CommandHandler cmdhandler(_server);
        cmdhandler.handleCommand(this, cmd);

		// CRITICAL: if handleCommand removed us (e.g. QUIT or bad password),
		// 'this' is now a dangling pointer; stops immediately
		if (_server->getClient(_fd) == nullptr)
			return;
    }
}

// ── getNextMessage() ─────────────────────────────────────────────────
// Returns ONE complete IRC line from _recvBuffer, without its line
// ending.  Returns an empty string if no complete line is ready yet.
//
// IRC line endings are \r\n (CRLF).  We look for '\n' and then strip
// a trailing '\r' if present — this handles both "\r\n" and bare "\n".
//
// Typical usage in Server (loop until no more complete lines):
//
//   std::string line;
//   while (!(line = client->getNextMessage()).empty())
//   {
//       auto cmd = CommandParser::parse(line);
//       handler->handleCommand(client, cmd);
//   }
//
// ── The updated version only accepts line ending with "\r\n"──────────
std::string Client::getNextMessage()
{
	// Accept both \r\n (IRC standard) and bare \n (nc without -C)
	auto pos = _recvBuffer.find('\n');

	// No '\r\n' found — we don't have a complete line yet
	if (pos == std::string::npos)
		return "";

	// Extract the line up to (but not including) the \n
	std::string msg = _recvBuffer.substr(0,pos);

	// Remove the consumed line including the '\n' from the buffer
	_recvBuffer.erase(0, pos + 1);

	// Strip trailing \r if present (handles proper \r\n CRLF)
	if (!msg.empty() && msg.back() == '\r')
		msg.pop_back();

	// Enforce 510-char limit (512 minus \r\n)
	if (msg.size() > 510)
		msg = msg.substr(0, 510);

	return msg;
}

