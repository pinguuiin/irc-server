#pragma once

#include <string>
#include <sys/epoll.h>
#include <map>
#include <vector>

#define SERVER_NAME "ft_irc"	// for undefined error on Commands.cpp

#define MAXCONN 128

class Client;
class Channel;

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

		// Daniel's part
		// Your additions to Server.hpp private section
		std::map<std::string, int>      _nickMap;
		std::map<std::string, Channel*> _channels;

		void processBuffer(int fd);
		void parseAndDispatch(int fd, const std::string& line);
		void tryRegister(int fd);
		void handlePass(int fd, const std::vector<std::string>& tokens);
		void handleNick(int fd, const std::vector<std::string>& tokens);
		void handleUser(int fd, const std::vector<std::string>& tokens);
		void handlePrivmsg(int fd, const std::vector<std::string>& tokens);
		void handleOper(int fd, const std::vector<std::string>& tokens);
		void sendReply(int fd, const std::string& msg);
		void disconnectClient(int fd);
		bool isNickValid(const std::string& nick) const;
		std::string getClientPrefix(int fd) const;
};
