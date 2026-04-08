#pragma once

#include <sys/epoll.h>
#include <map>

#define MAXCONN 128

class Client;

class Server {

	public:
		Server() = default;
		~Server();

		void createSocket();
		void handlePolling();
		void acceptNewClient(struct epoll_event &ev);
		void receiveMessage(int fd);
		void initServer();

	private:
		static constexpr uint16_t _port = 9999;
		int _serFd{-1}; // listening socket file descriptor
		int _epollFd{-1};
		std::map<int, Client> _client;
};
