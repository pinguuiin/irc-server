#include "Server.hpp"
#include <unistd.h> // for close()
#include <cstring> // for strerror(), memset()
#include <fcntl.h>
#include <sys/socket.h>
#include <netinet/in.h> // for struct sockaddr
#include <iostream> // for io and error handling
#include <poll.h>


Server::~Server()
{
	if (_sockFd != -1)
		close(_sockFd);
}

/* Create and set up the listening socket for Server */
void	Server::createSocket()
{
	// Create and configure socket
	int opt = 1;

	_sockFd = socket(AF_INET, SOCK_STREAM, 0);
	if (_sockFd == -1)
		throw std::runtime_error(std::string("socket error: ") + std::strerror(errno));
	if (setsockopt(_sockFd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) == -1)
		throw std::runtime_error(std::string("setsockopt error: ") + std::strerror(errno));
	if (fcntl(_sockFd, F_SETFL, O_NONBLOCK) == -1)
		throw std::runtime_error(std::string("fcntl error: ") + std::strerror(errno));

	// Set up socket address
	struct sockaddr_in addr;

	memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_port = htons(_port);
	addr.sin_addr.s_addr = htonl(INADDR_ANY);

	// bind socket to address and set it to passive to listen to incoming connection requests
	if (bind(_sockFd, (struct sockaddr *)&addr, sizeof(addr)) == -1)
		throw std::runtime_error(std::string("bind error: ") + std::strerror(errno));
	if (listen(_sockFd, SOMAXCONN) == -1)
		throw std::runtime_error(std::string("listen error: ") + std::strerror(errno));
	std::cout << "Server listening on port " << _port << "..." << std::endl;
}

void	Server::initServer()
{
	createSocket();

	struct pollfd fds;
	while (1) {

	}
}
