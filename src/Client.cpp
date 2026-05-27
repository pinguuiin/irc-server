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

// ── Getters ────────────────────────────────────────────────────────────────────
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

// ── Setters ────────────────────────────────────────────────────────────────────
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

// ── Registration step trackers ─────────────────────────────────────────────────
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

// ── Authentication ─────────────────────────────────────────────────────────────
// Called by CommandHandler::tryCompleteRegistration() once PASS + NICK + USER are all validated.
void Client::authenticate()
{
	_authenticated = true;
}

// ── Send buffer ────────────────────────────────────────────────────────────────
/**
 * Appends msg to the outgoing buffer.
 * The actual flush is triggered by Server::sendMessage() via EPOLLOUT.
 */
void Client::appendSendBuffer(const std::string& msg)
{
	_sendBuffer += msg;
}

/**
 * Attempts to drain _sendBuffer through a non-blocking send() loop.
 *
 * send() is called repeatedly until either:
 *   - The buffer is empty           → return true  (caller may disable EPOLLOUT)
 *   - EAGAIN / EWOULDBLOCK          → return false (kernel buffer full, keep EPOLLOUT)
 *   - A real socket error           → return false (caller should handle disconnect)
 *
 * MSG_NOSIGNAL prevents SIGPIPE if the peer already closed the connection.
 */
bool Client::sendPendingMessage()
{
	while (!_sendBuffer.empty())
	{

		ssize_t sent = send(_fd, _sendBuffer.c_str(), _sendBuffer.size(), MSG_NOSIGNAL);

		if (sent < 0)
		{
			if (errno == EAGAIN || errno == EWOULDBLOCK)
				return false;
			std::cerr << "Client fd=" << _fd << ": send() error: " << std::strerror(errno) << "\n";
			return false;
		}
		// Erase the bytes that were successfully sent.
		// substr from 'sent' keeps whatever wasn't sent yet.
		_sendBuffer.erase(0, static_cast<size_t>(sent));
	}
	return true;
}

// ── Receive / parse pipeline ───────────────────────────────────────────────────
/**
 * Appends the raw recv() chunk to _recvBuffer, then extracts and dispatches
 * every complete IRC line found in the buffer.
 *
 * After each handleCommand() call we re-check that 'this' still exists in the
 * Server's client map, because commands like QUIT or a wrong PASS may have
 * removed the client mid-loop — accessing 'this' afterwards would be UB.
 */
void Client::receiveAndHandleMessage(const char *buf)
{
	std::string msg;
	int fd = _fd;
	Server *server = _server;

	_recvBuffer += std::string(buf);
	while (!(msg = getNextMessage()).empty())
	{
		const CommandParser::ParsedCommand& cmd = CommandParser::parse(msg);
		// ignore invalid commands with empty command name
		if (!CommandParser::validateCommand(cmd))
			continue;
		CommandHandler cmdhandler(_server);
		cmdhandler.handleCommand(this, cmd);

		// CRITICAL: if handleCommand removed this client (e.g. QUIT or bad PASS),
		// 'this' is now a dangling pointer — stop immediately.
		if (server->getClient(fd) == nullptr)
			return;
	}
}

// ── Line framing ───────────────────────────────────────────────────────────────
/**
 * Extracts one complete IRC line from _recvBuffer.
 *
 * Scans for '\n'. Strips a trailing '\r' so both CRLF (\r\n) and bare LF (\n)
 * are accepted (handles irssi and plain netcat). Lines longer than 510
 * characters are silently truncated (IRC spec: max 512 bytes including \r\n).
 *
 * @return The next complete line without its line ending,
 *         or an empty string if no complete line is buffered yet.
 */
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
