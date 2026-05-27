/**
 * @file Client.hpp
 * @brief Declaration of the Client class representing a connected IRC user.
 *
 * Each Client corresponds to one TCP connection accepted by the Server.
 * It owns two string buffers:
 *   - _recvBuffer: raw bytes received from the socket, accumulated until
 *                  complete IRC lines can be extracted.
 *   - _sendBuffer: outgoing bytes waiting to be flushed via non-blocking send().
 *
 * Registration is tracked through three independent flags (_passOk, _nickSet,
 * _userSet). The client is considered "authenticated" only when all three flags
 * are true and authenticate() has been called.
 */

#ifndef CLIENT_HPP
#define CLIENT_HPP

#include <string>

class Server;

/**
 * @class Client
 * @brief Represents a single connected IRC client.
 *
 * Responsibilities:
 *  - Holding connection identity: file descriptor, IP address, nickname,
 *    and username.
 *  - Tracking the three-step registration handshake (PASS / NICK / USER).
 *  - Buffering incoming raw bytes and splitting them into complete IRC lines.
 *  - Buffering outgoing IRC messages and flushing them through a
 *    non-blocking send loop.
 *
 * The Server is responsible for closing the file descriptor when a client
 * disconnects; the Client destructor intentionally does NOT call close().
 */
class Client {

	public:
		/**
		 * @brief Constructs a new Client.
		 *
		 * All registration flags (_passOk, _nickSet, _userSet, _authenticated)
		 * are initialised to false. Both _recvBuffer and _sendBuffer start empty.
		 *
		 * @param fd     Accepted socket file descriptor for this client.
		 * @param ip     Client's IP address as a dotted-decimal string.
		 * @param server Non-owning pointer to the owning Server.
		 */
		Client(int fd, std::string ip, Server* server);

		/**
		 * @brief Destructor.
		 *
		 * Intentionally empty — the Server is responsible for closing
		 * the socket file descriptor on client disconnect.
		 */
		~Client();

	// ── Getters ────────────────────────────────────────────────────────
		/** @brief Returns the client's socket file descriptor. */
		const int& getFd() const;

		/** @brief Returns the client's IP address (e.g. "127.0.0.1"). */
		const std::string& getIp() const;

		/** @brief Returns the client's current nickname (empty before NICK). */
		const std::string& getNickname() const;

		/** @brief Returns the client's username/ident (empty before USER). */
		const std::string& getUsername() const;

		/** @brief Returns a non-owning pointer to the owning Server. */
		Server* getServer() const;

	// ── Setters ────────────────────────────────────────────────────────
		/** @brief Updates the client's socket file descriptor. */
		void setFd(int fd);

		/** @brief Updates the client's IP address string. */
		void setIp(std::string ip);

		/** @brief Sets the client's IRC nickname. */
		void setNickname(std::string nick);

 		/** @brief Sets the client's IRC username (ident). */
		void setUsername(std::string user);

	// ── Registration step trackers ─────────────────────────────────────
		/** @brief Returns true if a correct PASS has been verified. */
		bool isPassOk() const;

		/** @brief Returns true if a valid NICK was set during registration. */
		bool isNickSet() const;

		/** @brief Returns true if USER was received during registration. */
		bool isUserSet() const;

		/** @brief Marks the PASS step as completed. */
		void setPassOk();

		/** @brief Marks the NICK step as completed. */
		void setNickSet();

		/** @brief Marks the USER step as completed. */
		void setUserSet();

	// ── Authentication ─────────────────────────────────────────────────
		/**
		 * @brief Returns true if the client has completed full registration.
		 *
		 * A client is authenticated once PASS, NICK, and USER have all been
		 * validated and authenticate() has been called (triggered by the
		 * 001 RPL_WELCOME send in tryCompleteRegistration).
		 */
		bool isAuthenticated() const;

		/**
		 * @brief Marks the client as fully authenticated.
		 *
		 * Called by CommandHandler::tryCompleteRegistration() once all
		 * three registration steps are confirmed.
		 */
		void authenticate();

	// ── Buffer management ──────────────────────────────────────────────
		/**
		 * @brief Appends raw bytes from recv() to _recvBuffer, then parses
		 *        and dispatches any complete IRC lines found.
		 *
		 * Called by Server::receiveMessage() after each recv() chunk.
		 * Internally loops on getNextMessage(), parses each line with
		 * CommandParser::parse(), and dispatches via CommandHandler::handleCommand().
		 *
		 * @warning After handleCommand() returns, 'this' may be a dangling pointer
		 *          if the command removed the client (e.g. QUIT or bad password).
		 *          The method checks Server::getClient(fd) and returns immediately
		 *          if the client no longer exists.
		 *
		 * @param buf Null-terminated buffer of raw bytes from recv().
		 */
		void receiveAndHandleMessage(const char *buf);

		/**
		 * @brief Appends a fully-formed IRC message to the outgoing send buffer.
		 *
		 * The actual flush is triggered by Server::queueMessage(), which calls
		 * this and then enables EPOLLOUT on the client's fd.
		 *
		 * @param msg IRC message string to queue (must include \\r\\n).
		 */
		void appendSendBuffer(const std::string& msg);

		/**
		 * @brief Attempts to flush all pending data in _sendBuffer to the socket.
		 *
		 * Uses a non-blocking send() loop. If the kernel send-buffer is full
		 * (EAGAIN / EWOULDBLOCK), the unsent portion stays in _sendBuffer and
		 * false is returned so the caller knows to keep EPOLLOUT active.
		 *
		 * @return true  if the buffer was fully drained (EPOLLOUT may be removed).
		 * @return false if data remains or a socket error occurred.
		 */
		bool sendPendingMessage();

		/**
		 * @brief Extracts the next complete IRC line from _recvBuffer.
		 *
		 * Scans for '\\n'. Strips a trailing '\\r' if present (handles both CRLF
		 * and bare LF). Lines are silently capped at 510 characters (512 minus \\r\\n).
		 *
		 * @return The next complete line without its line ending,
		 *         or an empty string if no complete line is available yet.
		 */
		std::string getNextMessage();

	private:
		int _fd;					//< Socket file descriptor (lifetime managed by Server)
		std::string _ip;			//< Client IP address (dotted-decimal)
		Server* _server;			//< Back-pointer to the owning Server (not owned)
		std::string _nickname;		//< IRC nickname (empty until NICK is received)
		std::string _username;		//< IRC username/ident (empty until USER is received)

		// Three separate flags - one per registration step.
		// _authenicate only becomes true once ALL three are done.
		bool _passOk;				//< true once a correct PASS has been verified
		bool _nickSet;				//< true once a valid NICK has been set
		bool _userSet;				//< true once USER has been received
		bool _authenticated;		//< true once 001 RPL_WELCOME has been sent
		std::string _recvBuffer;	//< Incoming raw bytes waiting to be framed into lines
		std::string _sendBuffer;	//< Outgoing bytes waiting to be flushed to the socket
};

#endif
