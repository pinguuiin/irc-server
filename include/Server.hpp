/**
 * @file Server.hpp
 * @brief Declaration of the Server class — the top-level IRC server object.
 *
 * Server owns the listening socket, the epoll instance, all connected Client
 * objects, and all Channel objects. It drives the main I/O event loop via
 * epoll and delegates command parsing / dispatch to Client and CommandHandler.
 *
 * Non-blocking I/O model
 * ----------------------
 *  - All sockets are set O_NONBLOCK via fcntl().
 *  - epoll is used in edge-triggered mode (EPOLLIN / EPOLLOUT).
 *  - EPOLLOUT is enabled per-client only when outgoing data is queued
 *    (enableWriteEvent) and disabled once the send buffer is fully drained
 *    (disableWriteEvent), to avoid busy-looping.
 *
 * Signal handling
 * ---------------
 *  SIGINT and SIGQUIT both call signalHandler(), which sets _running = 0.
 *  The epoll_wait() loop checks _running after every timeout and exits
 *  gracefully when it becomes 0.
 */

#ifndef SERVER_HPP
#define SERVER_HPP

#include <sys/epoll.h>
#include <string>
#include <map>
#include <csignal>

/// Maximum number of simultaneous connections / epoll events processed per wait.
#define MAXCONN 128

class Client;
class Channel;

/**
 * @class Server
 * @brief Top-level IRC server: owns the event loop, all clients, and all channels.
 *
 * Typical lifetime:
 *  1. Construct with port + password.
 *  2. Register signal handlers (SIGINT / SIGQUIT → signalHandler).
 *  3. Call initServer() — this blocks in the epoll loop until a signal fires.
 *  4. Destructor closes all file descriptors and frees all Channel objects.
 */
class Server {

	public:
		/**
		 * @brief Constructs a Server with the given port and password.
		 *
		 * Does not open any sockets — call initServer() to start listening.
		 *
		 * @param port     TCP port to listen on (1024–65535).
		 * @param password Server password clients must supply via PASS.
		 */
		Server(uint16_t port, std::string password);

		/**
		 * @brief Destructor. Closes all open file descriptors and frees all channels.
		 *
		 * Closes _serFd, _epollFd, and _newCliFd if they are valid.
		 * Closes all client file descriptors in _clients.
		 * Deletes all Channel objects in _channels.
		 */
		~Server();

	// ── Startup ────────────────────────────────────────────────────────

		/**
		 * @brief Initialises and starts the server.
		 *
		 * Sets _running = 1, then calls createSocket() followed by
		 * handlePolling(). Blocks until a stop signal is received.
		 */
		void initServer();

		/**
		 * @brief Creates, configures, and binds the listening TCP socket.
		 *
		 * Steps performed:
		 *  1. socket(AF_INET, SOCK_STREAM, 0)
		 *  2. setsockopt SO_REUSEADDR
		 *  3. fcntl O_NONBLOCK
		 *  4. bind to INADDR_ANY on _port
		 *  5. listen(MAXCONN)
		 *
		 * @throws std::runtime_error on any system call failure.
		 */
		void createSocket();

		/**
		 * @brief Creates the epoll instance and runs the main I/O event loop.
		 *
		 * Adds the listening socket (_serFd) to epoll for EPOLLIN. Then loops
		 * on epoll_wait() until _running becomes 0:
		 *  - New connection on _serFd  → acceptNewClient()
		 *  - EPOLLHUP / EPOLLERR       → receiveMessage() (triggers removeClient)
		 *  - EPOLLIN                   → receiveMessage()
		 *  - EPOLLOUT                  → sendMessage()
		 *
		 * Calls stopServer() before returning.
		 *
		 * @throws std::runtime_error on epoll_create1 or epoll_ctl failure.
		 */
		void handlePolling();

	// ── Client management ─────────────────────────────────────────────
	 	/**
		 * @brief Accepts a new TCP connection and registers it with epoll.
		 *
		 * Performs accept(), sets O_NONBLOCK, registers the fd with EPOLLIN,
		 * resolves the IP via inet_ntop(), and inserts a new Client into
		 * _clients. On any failure, the fd is closed and the method returns
		 * without throwing (errors are logged to stderr).
		 */
		void acceptNewClient();

		/**
		 * @brief Removes a client: deregisters from epoll, erases from map,
		 *        and closes the file descriptor.
		 *
		 * Safe to call on an fd that has already been removed (no-op).
		 * Errors from epoll_ctl are logged to stderr rather than thrown,
		 * because the fd may already be invalid on abrupt disconnects.
		 *
		 * @param fd File descriptor of the client to remove.
		 */
		void removeClient(int fd);

		/**
		 * @brief Looks up a Client by file descriptor.
		 * @param fd The file descriptor to look up.
		 * @return Pointer to the Client, or NULL if not found.
		 */
		Client* getClient(int fd);

		/**
		 * @brief Looks up a Client by IRC nickname (case-sensitive).
		 * @param nickname The nickname to search for.
		 * @return Pointer to the matching Client, or NULL if not found.
		 */
		Client* getClientByNickname(const std::string& nickname);

	// ── I/O helpers ───────────────────────────────────────────────────
		/**
		 * @brief Reads all available data from a client's socket.
		 *
		 * Loops on recv() until EAGAIN/EWOULDBLOCK or an error/disconnect.
		 * On recv() == 0 (clean close): broadcasts QUIT to the client's
		 * channels, removes the client from each channel, and calls
		 * removeClient(). On recv() < 0 (error): logs and returns.
		 * Each successful chunk is forwarded to Client::receiveAndHandleMessage().
		 *
		 * @param fd File descriptor to read from.
		 */
		void receiveMessage(int fd);

		/**
		 * @brief Appends a message to a client's send buffer and enables EPOLLOUT.
		 *
		 * Calls Client::appendSendBuffer(msg) then enableWriteEvent(fd) so the
		 * epoll loop will call sendMessage() when the socket becomes writable.
		 *
		 * @param fd  File descriptor of the target client.
		 * @param msg Fully-formed IRC message string (must include \\r\\n).
		 */
		void queueMessage(int fd, const std::string& msg);

		/**
		 * @brief Flushes a client's outgoing send buffer.
		 *
		 * Calls Client::sendPendingMessage(). If it returns true (buffer fully
		 * drained), calls disableWriteEvent() to remove EPOLLOUT and avoid
		 * busy-looping.
		 *
		 * @param fd File descriptor of the client to flush.
		 */
		void sendMessage(int fd);

		/**
		 * @brief Adds EPOLLOUT to a client's epoll interest set.
		 *
		 * Called by queueMessage() whenever outgoing data is buffered.
		 * The epoll loop will then call sendMessage() when the socket is writable.
		 *
		 * @param fd File descriptor to update.
		 */
		void enableWriteEvent(int fd);

		/**
		 * @brief Removes EPOLLOUT from a client's epoll interest set.
		 *
		 * Called by sendMessage() once the send buffer is fully drained,
		 * reverting the fd back to EPOLLIN-only to avoid unnecessary wakeups.
		 *
		 * @param fd File descriptor to update.
		 */
		void disableWriteEvent(int fd);

	// ── Channel management ────────────────────────────────────────────
		/**
		 * @brief Looks up a Channel by name (case-sensitive).
		 * @param name Channel name (e.g. "#general").
		 * @return Pointer to the Channel, or NULL if it does not exist.
		 */
		Channel* getChannel(const std::string& name);

		/**
		 * @brief Returns an existing Channel or creates a new one.
		 *
		 * If the channel already exists, the existing pointer is returned
		 * unchanged. Otherwise a new Channel is heap-allocated, inserted
		 * into _channels, and returned.
		 *
		 * @param name Channel name (e.g. "#general").
		 * @return Non-null pointer to the Channel.
		 */
		Channel* createChannel(const std::string& name);

		/**
		 * @brief Returns a const reference to the full channel map.
		 *
		 * Used by CommandHandler::handleQuit() to iterate over all channels
		 * and broadcast a QUIT message before removing the client.
		 *
		 * @return Const reference to the internal channel map.
		 */
		const std::map<std::string, Channel*>& getChannels() const;

		/**
		 * @brief Destroys and removes a channel by name.
		 *
		 * Called when the last member leaves a channel to prevent memory leaks.
		 * Does nothing if the channel does not exist.
		 *
		 * @param name Channel name to remove.
		 */
		void removeChannel(const std::string& name);

		/** @brief Returns the server password. */
		const std::string& getPassword() const;

		/**
		 * @brief Performs any remaining cleanup after the event loop exits.
		 *
		 * Currently a no-op; cleanup is handled by the destructor. Kept as a
		 * named method for clarity and future extensibility.
		 */
		void stopServer();

		/**
		 * @brief Signal handler for SIGINT and SIGQUIT.
		 *
		 * Sets _running = 0 so the epoll_wait() loop exits cleanly on the
		 * next iteration. Must be static because signal() expects a plain
		 * function pointer void(*)(int) — a non-static member function has
		 * an implicit 'this' parameter and cannot decay to that type.
		 *
		 * @param signum The signal number received (unused).
		 */
		static void signalHandler(int);

	private:
		const uint16_t _port;						//< TCP port the server listens on
		const std::string _password;				//< Server password (checked by PASS command)
		int _serFd{-1}; 							//< Listening socket file descriptor
		int _epollFd{-1};							//< epoll instance file descriptor
		int _newCliFd{-1};							//< Temporary fd during acceptNewClient()
		std::map<int, Client> _clients;				//< All connected clients, keyed by fd
		std::map<std::string, Channel*> _channels;	//< All active channels, keyed by name

		// Set to 1 in initServer(), cleared to 0 by signalHandler().
		// Declared volatile sig_atomic_t to be safely readable from a signal handler.
		static volatile sig_atomic_t  _running;
};

#endif
