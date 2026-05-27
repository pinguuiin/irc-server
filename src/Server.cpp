#include "Server.hpp"
#include "Channel.hpp"
#include "Client.hpp"

#include <unistd.h> // for close()
#include <cstring> // for strerror(), memset()
#include <fcntl.h>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <netinet/in.h> // for struct sockaddr
#include <arpa/inet.h> // for inet_ntop()
#include <iostream> // for io and error handling

volatile sig_atomic_t Server::_running = 0;

Server::Server(uint16_t port, std::string password)
	: _port(port), _password(password) {}

Server::~Server()
{
	if (_serFd != -1)
		close(_serFd);
	if (_epollFd != -1)
		close(_epollFd);
	if (_newCliFd != -1)
		close(_newCliFd);

	for (auto &cli : _clients)
	{
		if (cli.first != -1)
			close(cli.first);
	}
	for (auto &ch : _channels)
	{
		delete ch.second;
	}
}

void Server::signalHandler(int signum)
{
	(void)signum;
	_running = 0;
}

void Server::initServer()
{
	_running = 1;
	createSocket();
	handlePolling();
}

/**
 * Creates, configures, and binds the listening TCP socket.
 *
 * SO_REUSEADDR lets the server rebind immediately after a restart
 * without waiting for the OS TIME_WAIT period to expire.
 */
void Server::createSocket()
{
	// create and configure socket
	int opt = 1;

	_serFd = socket(AF_INET, SOCK_STREAM, 0);
	if (_serFd == -1)
		throw std::runtime_error(std::string("socket error: ") + std::strerror(errno));
	if (setsockopt(_serFd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) == -1)               // not sure about the option
		throw std::runtime_error(std::string("setsockopt error: ") + std::strerror(errno));
	if (fcntl(_serFd, F_SETFL, O_NONBLOCK) == -1)
		throw std::runtime_error(std::string("fcntl error: ") + std::strerror(errno));

	// set up socket address
	struct sockaddr_in addr;

	memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_port = htons(_port);
	addr.sin_addr.s_addr = htonl(INADDR_ANY);

	// bind socket to address and set it to passive to listen to incoming connection requests
	if (bind(_serFd, (struct sockaddr *)&addr, sizeof(addr)) == -1)
		throw std::runtime_error(std::string("bind error: ") + std::strerror(errno));
	if (listen(_serFd, MAXCONN) == -1)
		throw std::runtime_error(std::string("listen error: ") + std::strerror(errno));
	std::cout << "Server listening on port " << _port << "..." << std::endl;
}

/**
 * Creates the epoll instance and runs the main I/O event loop.
 *
 * epoll_wait() uses a 1000 ms timeout so the _running flag is checked
 * at least once per second, enabling a clean shutdown on SIGINT/SIGQUIT.
 */
void Server::handlePolling()
{
	_epollFd = epoll_create1(0);
	if (_epollFd == -1)
		throw std::runtime_error(std::string("epoll_create1 error: ") + std::strerror(errno));

	// Add listening socket to monitoring list
	struct epoll_event ev, events[MAXCONN];

	// listening socket handles only read (accepting new connection request)
	ev.events = EPOLLIN;
	ev.data.fd = _serFd;
	if (epoll_ctl(_epollFd, EPOLL_CTL_ADD, _serFd, &ev) == -1)
		throw std::runtime_error(std::string("epoll_ctl_add error: ") + std::strerror(errno));

	// Poll on monitored sockets
	int n, nfds;

	while (_running == 1)
	{
		nfds = epoll_wait(_epollFd, events, MAXCONN, 1000); // timeout=?
		if (nfds == -1)
		{
			if (_running == 0) // signal interrupted epoll_wait, silently stop the server
				break;
			throw std::runtime_error(std::string("epoll_wait error: ") + std::strerror(errno));
		}
		for (n = 0; n < nfds; ++n)
		{
			int eventFd = events[n].data.fd;
			uint32_t evFlags = events[n].events;

			// New connection on the listening socket
			if (eventFd == _serFd)
			{
				acceptNewClient();
				continue; // newly registered fd cannot have events int this batch
			}
			/// Handle dead fds first (EPOLLHUP/EPOLLERR may arrive together with EPOLLIN).
			// Calling receiveMessage() here lets recv() return 0 / -1 so removeClient()
			// is triggered. We 'continue' to prevent EPOLLIN/EPOLLOUT from firing on a
			// fd we're about to remove.
			if (evFlags & (EPOLLHUP | EPOLLERR))
			{
				receiveMessage(eventFd);
				continue;
			}
			// Guard: this fd may have been removed by an earlier event in the same
			// epoll batch (e.g. a previous EPOLLHUP for the same fd).
			if (evFlags & EPOLLIN)
			{
				if (_clients.find(eventFd) != _clients.end())
					receiveMessage(eventFd);
			}
			// Writeable: send buffer has space, flush queued output.
			if (events[n].events & EPOLLOUT)
			{
				if (_clients.find(eventFd) != _clients.end())
					sendMessage(eventFd);
			}
		}
	}
	stopServer();
}

/* Accept incoming connection request from the new client, register it to the
interesting list of epoll, and add new client to client map */
void Server::acceptNewClient()
{
	struct sockaddr_in addr;
	socklen_t len;
	char ipBuf[INET_ADDRSTRLEN];

	len = sizeof(addr);

	// Accept new connection request and add it to monitoring list
	_newCliFd = accept(_serFd, (struct sockaddr *)&addr, &len);
	if (_newCliFd == -1)
	{
		std::cerr << "accept warning: " << std::strerror(errno) << "\n";
		return;
	}

	if (fcntl(_newCliFd, F_SETFL, O_NONBLOCK) == -1)
		std::cerr << "fcntl warning fd=" << _newCliFd << ": " << std::strerror(errno) << "\n";

	// Register with EPOLLIN only - new client has nothing queued to send yet.
	// EPOLLOUT is added dynamically by enableWriteEvent() when output is queued.
	struct epoll_event clientEv;
	clientEv.events = EPOLLIN;
	clientEv.data.fd = _newCliFd;
	if (epoll_ctl(_epollFd, EPOLL_CTL_ADD, _newCliFd, &clientEv) == -1)
		std::cerr << "epoll_ctl_add warning fd=" << _newCliFd << ": " << std::strerror(errno) << "\n";

	// Create new client instance and add it to server
	if (inet_ntop(AF_INET, &(addr.sin_addr), ipBuf, INET_ADDRSTRLEN) == NULL)
	{
		epoll_ctl(_epollFd, EPOLL_CTL_DEL, _newCliFd, NULL); // must remove from epoll
		std::cerr << "inet_ntop warning fd=" << _newCliFd << ": " << std::strerror(errno) << "\n";
		close(_newCliFd);
		_newCliFd = -1;
		return;
	}
	_clients.emplace(std::make_pair(_newCliFd, Client(_newCliFd, ipBuf, this)));
	std::cout << "A new client is connected" << "\n";
	_newCliFd = -1;
}

void Server::removeClient(int fd)
{
	// Guard: if this fd is already gone, do nothing
	if (_clients.find(fd) == _clients.end())
		return;
	// We use cerr instead of throw; on abrupt disconnects the fd may
	// already be invalid, and killing the whole server over it is wrong.
	if (epoll_ctl(_epollFd, EPOLL_CTL_DEL, fd, NULL) == -1)
		std::cerr << "epoll_ctl_del warning fd=" << fd << ": " << std::strerror(errno) << "\n";
	_clients.erase(fd);
	close(fd);
	std::cout << "A client is removed from the server" << "\n";
}

Client* Server::getClient(int fd)
{
	std::map<int, Client>::iterator it = _clients.find(fd);
	if (it == _clients.end())
		return NULL;
	return &it->second;
}

Client* Server::getClientByNickname(const std::string& nickname)
{
	for (std::map<int, Client>::iterator it = _clients.begin(); it != _clients.end(); ++it) {
		if (it->second.getNickname() == nickname)
			return &it->second;
	}
	return NULL;
}

/**
 * Reads all available data from the client's non-blocking socket.
 *
 * Loops on recv() until EAGAIN (no more data) or an error/disconnect:
 *   - recv() == 0: peer closed the connection cleanly (no QUIT was sent).
 *                  Broadcasts a QUIT to affected channels, cleans up, and
 *                  calls removeClient().
 *   - recv()  < 0: non-EAGAIN error — log and return. The fd stays alive
 *                  until the next EPOLLHUP/EPOLLERR triggers cleanup.
 */
void Server::receiveMessage(int fd)
{
	char buf[1024];
	ssize_t n;

	while (1)
	{
		memset(buf, 0, sizeof(buf));
		n = recv(fd, buf, sizeof(buf), 0);
		if (n < 0)
		{
			if (errno == EAGAIN || errno == EWOULDBLOCK)  // End of input
				return ;
			std::cerr << "recv warning fd=" << fd << ": " << std::strerror(errno) << "\n";
			return;
		}
		if (n == 0)  // Client closed connection without sending QUIT(close terminal or kill irssi process)
		{
			Client* client = getClient(fd);
			if (client)
			{
				// Build the quit message to broadcast to channels
				std::string nick = client->getNickname().empty() ? "*" : client->getNickname();
				std::string user = client->getUsername().empty() ? "unknown" : client->getUsername();
				std::string quitMsg = ":" + nick + "!" + user + "@localhost QUIT :Connection closed\r\n";

				//Find every channel this client was in and notify + remove them
				std::vector<Channel*> toLeave;
				for (std::map<std::string, Channel*>::iterator it = _channels.begin(); it != _channels.end(); ++it)
				{
					if (it->second->hasClient(client))
						toLeave.push_back(it->second);
				}
				for (size_t i = 0; i < toLeave.size(); ++i)
				{
					toLeave[i]->broadcastMessage(quitMsg, client);
					toLeave[i]->removeClient(client);

					if (toLeave[i]->getClients().empty())
						removeChannel(toLeave[i]->getName());
				}
			}
			removeClient(fd);
			return; // fd is dead — stop the loop immediately
		}
		else
		{
			Client* client = getClient(fd);
			if (!client)
				return;
			client->receiveAndHandleMessage(buf);
			// handleCommand may have removed this client (e.g. QUIT, bad PASS)
			if (_clients.find(fd) == _clients.end())
				return;
		}
	}
}

void Server::queueMessage(int fd, const std::string& msg)
{
	Client* client = getClient(fd);

	if (!client)
	{
		std::cerr << "program warning fd=" << fd << ": " << "client not in the list" << "\n";
		return;
	}
	client->appendSendBuffer(msg);
	enableWriteEvent(fd);
}

void Server::sendMessage(int fd)
{
	bool res = _clients.at(fd).sendPendingMessage();
	// check again - sendPendingMessage may have triggered a disconnect
	if (_clients.find(fd) == _clients.end())
		return;
	if (res == true)
		disableWriteEvent(fd);
}

void Server::enableWriteEvent(int fd)
{
	epoll_event ev;

	ev.events = EPOLLIN | EPOLLOUT;
	ev.data.fd = fd;

	if (epoll_ctl(_epollFd, EPOLL_CTL_MOD, fd, &ev) == -1)
		std::cerr << "enableWriteEvent warning fd=" << fd << ": " << std::strerror(errno) << "\n";
}

void Server::disableWriteEvent(int fd)
{
	epoll_event ev;

	ev.events = EPOLLIN;
	ev.data.fd = fd;

	if (epoll_ctl(_epollFd, EPOLL_CTL_MOD, fd, &ev) == -1)
		std::cerr << "disableWriteEvent warning fd=" << fd << ": " << std::strerror(errno) << "\n";
}

Channel* Server::getChannel(const std::string& name)
{
	std::map<std::string, Channel*>::iterator it = _channels.find(name);
	if (it == _channels.end())
		return NULL;
	return it->second;
}

Channel* Server::createChannel(const std::string& name)
{
	Channel* channel = getChannel(name);
	if (channel != NULL)
		return channel;
	channel = new Channel(name, this);
	_channels.insert(std::make_pair(name, channel));
	return channel;
}

//for handleQuit(CommandHandler.cpp) needs to iterate over all channels.
const std::map<std::string, Channel*>& Server::getChannels() const
{
	return _channels;
}

// removeChannel(Without this, channels pile up in memory forever even after
// everyone leaves, which is a memory leak)
void Server::removeChannel(const std::string& name)
{
	std::map<std::string, Channel*>::iterator it = _channels.find(name);
	if (it == _channels.end())
		return;
	delete it->second;
	_channels.erase(it);
}

const std::string& Server::getPassword() const
{
	return _password;
}

void Server::stopServer()
{
	std::cout << "Server is shutting down" << std::endl;
}
