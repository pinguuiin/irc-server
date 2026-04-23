#ifndef SERVER_HPP
#define SERVER_HPP

#include <map>
#include <string>

class Client;
class Channel;

class Server {
public:
	Server(int port, const std::string& password);
	~Server();
	void run();
	void stop();
	void addClient(int fd, const std::string& ip);
	void removeClient(int fd);
	Client* getClient(int fd);
	Channel* getChannel(const std::string& name);
	Channel* createChannel(const std::string& name, Client* creator);
	const std::string& getPassword() const;

private:
	int _port;
	std::string _password;
	bool _running;
	std::map<int, Client*> _clients;
	std::map<std::string, Channel*> _channels;
};

#endif
