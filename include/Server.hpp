#pragma once

#include <string>
#include <sys/epoll.h>
#include <map>

#define MAXCONN 128

class Client;

class Server {

	public:
		Server() = delete;
		Server(uint16_t port, std::string password);
		~Server();

		void createSocket();
		void handlePolling();
		void acceptNewClient(struct epoll_event &ev);
		void receiveMessage(int fd);
		void initServer();

	private:
		const uint16_t _port;
		const std::string _password;
		int _serFd{-1}; // listening socket file descriptor
		int _epollFd{-1};
		std::map<int, Client> _client;
};
