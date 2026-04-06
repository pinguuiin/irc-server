#include "Server.hpp"
#include "Client.hpp"
#include <unistd.h> // for close()
#include <cstring> // for strerror(), memset()
#include <fcntl.h>
#include <sys/socket.h>
#include <netinet/in.h> // for struct sockaddr
#include <arpa/inet.h> // for inet_ntop()
#include <iostream> // for io and error handling


Server::~Server()
{
	if (_serFd != -1) {
		close(_serFd);
		_serFd = -1;  // these could be safely removed if they turn out to be redundant in the end
	}
	if (_epollFd != -1) {
		close(_epollFd);
		_epollFd = -1;
	}
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

/* Create epoll instance and start polling for incoming connection
requests from clients and messages sent by registered clients */
void Server::handlePolling()
{

	// Create epoll instance
	_epollFd = epoll_create1(0);
	if (_epollFd = -1)
		throw std::runtime_error(std::string("epoll_create1 error: ") + std::strerror(errno));

	// Add listening socket to monitoring list
	struct epoll_event ev, events[MAXCONN];

	ev.events = EPOLLIN;
	ev.data.fd = _serFd;
	if (epoll_ctl(_epollFd, EPOLL_CTL_ADD, _serFd, &ev) == -1)
		throw std::runtime_error(std::string("epoll_ctl error: ") + std::strerror(errno));

	// Poll and handle events of accepting connection and receiving messages
	int n, nfds, cliFd;
	struct sockaddr_in addr;
	socklen_t len;
	char buf[INET_ADDRSTRLEN];
	Client client;

	cliFd = -1;
	len = sizeof(addr);

	while (1) {
		nfds = epoll_wait(_epollFd, events, MAXCONN, 1000); // timeout=?
		if (nfds == -1)
			throw std::runtime_error(std::string("epoll_wait error: ") + std::strerror(errno));
		for (n = 0; n < nfds; ++n) {
			if (events[n].data.fd == _serFd) {
				cliFd = accept(_serFd, (struct sockaddr *)&addr, &len);
				if (cliFd == -1)
					throw std::runtime_error(std::string("accept error: ") + std::strerror(errno));
				if (fcntl(cliFd, F_SETFL, O_NONBLOCK) == -1)
					throw std::runtime_error(std::string("fcntl error: ") + std::strerror(errno));
				ev.events = EPOLLIN; // not sure whether to add EPOLLET
				ev.data.fd = cliFd;
				if (epoll_ctl(_epollFd, EPOLL_CTL_ADD, cliFd, &ev) == -1)
					throw std::runtime_error(std::string("epoll_ctl error: ") + std::strerror(errno));
				if (inet_ntop(AF_INET, &(addr.sin_addr), buf, INET_ADDRSTRLEN) == NULL)
					throw std::runtime_error(std::string("inet_ntop error: ") + std::strerror(errno));
				client.setIp(buf);                    //
				_client.insert({cliFd, client});
				_fds.push_back(cliFd);
			}
			else
				recv();
		}
	}

}

void	Server::initServer()
{
	createSocket();
	handlePolling();
}
