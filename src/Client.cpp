#include "Client.hpp"
#include <unistd.h> // for close()
#include <cstring> // for strerror(), memset()
#include <fcntl.h>
#include <sys/socket.h>
#include <netinet/in.h> // for struct sockaddr
#include <iostream> // for io and error handling


Client::~Client()
{
	if (_fd != -1) {
		close(_fd);
		_fd = -1;
	}
}
