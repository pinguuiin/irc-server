#pragma once

#include <sys/epoll.h>
#include <vector>
#include <map>

#define MAXCONN 128

class Client;

class Server {

	public:
		Server() = default;
		~Server();

		void createSocket();
		void handlePolling();
		void initServer();

	private:
		static constexpr uint16_t _port = 9999;
		int _serFd{-1}; // listening socket file descriptor
		int _epollFd{-1};
		std::vector<int> _fds;
		std::map<int, Client> _client;
};
