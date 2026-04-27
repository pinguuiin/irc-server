#include "Server.hpp"
#include "Channel.hpp"
#include "Client.hpp"

#include <unistd.h> // for close()
#include <cstring> // for strerror(), memset()
#include <fcntl.h>
#include <sys/socket.h>
#include <netinet/in.h> // for struct sockaddr
#include <arpa/inet.h> // for inet_ntop()
#include <iostream> // for io and error handling

Server::Server(uint16_t port, std::string password)
	: _port(port), _password(password), _running(false) {}

Server::~Server()
{
	if (_serFd != -1)
		close(_serFd);
	if (_epollFd != -1)
		close(_epollFd);
	if (_newCliFd != -1)
		close(_newCliFd);

	for (auto cli : _clients) {
		if (cli.first != -1)
			close(cli.first);
	}
}

void Server::initServer()
{
	createSocket();
	handlePolling();
	_running = true;
}

/* Create and set up the listening socket for Server */
void	Server::createSocket()
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

/* Create epoll instance and start polling for new connection requests or
messages sent by registered clients */
void	Server::handlePolling()
{
	_epollFd = epoll_create1(0);
	if (_epollFd == -1)
		throw std::runtime_error(std::string("epoll_create1 error: ") + std::strerror(errno));

	// Add listening socket to monitoring list
	struct epoll_event ev, events[MAXCONN];

	ev.events = EPOLLIN;
	ev.data.fd = _serFd;
	if (epoll_ctl(_epollFd, EPOLL_CTL_ADD, _serFd, &ev) == -1)
		throw std::runtime_error(std::string("epoll_ctl error: ") + std::strerror(errno));

	// Poll on monitored sockets
	int n, nfds;

	while (1) {
		nfds = epoll_wait(_epollFd, events, MAXCONN, 1000); // timeout=?
		if (nfds == -1)
			throw std::runtime_error(std::string("epoll_wait error: ") + std::strerror(errno));
		for (n = 0; n < nfds; ++n) {
			if (events[n].data.fd == _serFd)
				acceptNewClient(ev);
			else
				receiveMessage(events[n].data.fd);
		}
	}
}

/* Accept incoming connection request from the new client, register it to the
interesting list of epoll, and add new client to client map */
void	Server::acceptNewClient(struct epoll_event &ev)
{
	int cliFd;
	struct sockaddr_in addr;
	socklen_t len;
	char ipBuf[INET_ADDRSTRLEN];

	cliFd = -1;
	len = sizeof(addr);

	// Accept new connection request and add it to monitoring list
	cliFd = accept(_serFd, (struct sockaddr *)&addr, &len);
	if (cliFd == -1)
		throw std::runtime_error(std::string("accept error: ") + std::strerror(errno));
	_newCliFd = cliFd;
	if (fcntl(cliFd, F_SETFL, O_NONBLOCK) == -1)
		throw std::runtime_error(std::string("fcntl error: ") + std::strerror(errno));
	ev.events = EPOLLIN; // not sure whether to add EPOLLET
	ev.data.fd = cliFd;
	if (epoll_ctl(_epollFd, EPOLL_CTL_ADD, cliFd, &ev) == -1)
		throw std::runtime_error(std::string("epoll_ctl error: ") + std::strerror(errno));

	// Create new client instance and add it to server
	if (inet_ntop(AF_INET, &(addr.sin_addr), ipBuf, INET_ADDRSTRLEN) == NULL)
		throw std::runtime_error(std::string("inet_ntop error: ") + std::strerror(errno));
	Client client(cliFd, ipBuf, this);
	_clients.insert({cliFd, &client});
	_newCliFd = -1;
}

void Server::removeClient(int fd)
{
	(void)fd;
	std::cerr << "TODO: Server::removeClient" << std::endl;
}

Client* Server::getClient(int fd)
{
	return _clients.at(fd);
}

void	Server::receiveMessage(int fd)
{
	char buf[1024];
	ssize_t n;

	while (1) {
		memset(buf, 0, sizeof(buf));
		n = recv(fd, buf, sizeof(buf), 0);
		if (n < 0) {
			if (errno == EAGAIN || errno == EWOULDBLOCK) { // End of input
				return ;
			}
			throw std::runtime_error(std::string("recv error: ") + std::strerror(errno));
		}
		if (n == 0) {  // The client shutdown the connection
			epoll_ctl(_epollFd, EPOLL_CTL_DEL, fd, NULL);
			_clients.erase(fd);
			close(fd);
		}
		else
			std::cout.write(buf, n);  // to be replaced with more complex message handler
	}
}

Channel* Server::getChannel(const std::string& name)
{
	(void)name;
	std::cerr << "TODO: Server::getChannel" << std::endl;
	return NULL;
}

Channel* Server::createChannel(const std::string& name, Client* creator)
{
	(void)name;
	(void)creator;
	std::cerr << "TODO: Server::createChannel" << std::endl;
	return NULL;
}

const std::string& Server::getPassword() const
{
	return _password;
}

void Server::stopServer()
{
	std::cerr << "TODO: Server::stop" << std::endl;
	_running = false;
}
