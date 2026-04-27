#ifndef SERVER_HPP
#define SERVER_HPP

#include <sys/epoll.h>
#include <string>
#include <map>

#define MAXCONN 128

class Client;
class Channel;

class Server {

	public:
		Server(uint16_t port, std::string password);
		~Server();

		void initServer();

		void createSocket();
		void handlePolling();

		// void addClient(int fd, const std::string& ip);
		void acceptNewClient(struct epoll_event &ev);
		void removeClient(int fd);
		Client* getClient(int fd);

		void receiveMessage(int fd);

		Channel* getChannel(const std::string& name);
		Channel* createChannel(const std::string& name, Client* creator);

		const std::string& getPassword() const;

		void stopServer();

	private:
		const uint16_t _port;
		const std::string _password;
		int _serFd{-1}; // listening socket fd
		int _epollFd{-1};
		int _newCliFd{-1};
		bool _running;
		std::map<int, Client*> _clients;
		std::map<std::string, Channel*> _channels;
};

#endif
