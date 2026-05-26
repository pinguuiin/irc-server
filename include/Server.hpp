#ifndef SERVER_HPP
#define SERVER_HPP

#include <sys/epoll.h>
#include <string>
#include <map>
#include <csignal>

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

		void acceptNewClient();
		void removeClient(int fd);
		Client* getClient(int fd);
		Client* getClientByNickname(const std::string& nickname);

		void receiveMessage(int fd);
		void queueMessage(int fd, const std::string& msg);
		void sendMessage(int fd);

		void enableWriteEvent(int fd);
		void disableWriteEvent(int fd);

		Channel* getChannel(const std::string& name);
		Channel* createChannel(const std::string& name);

		//for handleQuit(CommandHandler.cpp) needs to iterate over all channels.
		const std::map<std::string, Channel*>& getChannels() const;

		// removeChannel(Without this, channels pile up in memory forever even after
		// everyone leaves, which is a memory leak)
		void removeChannel(const std::string& name);

		const std::string& getPassword() const;

		void stopServer();

		// signal() expects a plain function pointer void(*)(int), but a non-static
		// member function has an implicit "this" parameter, so it can't decay to
		// that type. It only compiles if signalHandler is static
		static void signalHandler(int);

	private:
		const uint16_t _port;
		const std::string _password;
		int _serFd{-1}; // listening socket fd
		int _epollFd{-1};
		int _newCliFd{-1};
		std::map<int, Client> _clients;
		std::map<std::string, Channel*> _channels;

		static volatile sig_atomic_t  _running;
};

#endif
