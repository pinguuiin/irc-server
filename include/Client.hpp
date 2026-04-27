#ifndef CLIENT_HPP
#define CLIENT_HPP

#include <string>

class Server;

class Client {
public:
	Client(int fd, const std::string& ip, Server* server);
	~Client();
	int getFd() const;
	const std::string& getNickname() const;
	const std::string& getUsername() const;
	bool isAuthenticated() const;
	void setNickname(const std::string& nick);
	void setUsername(const std::string& user);
	void authenticate();
	void sendMessage(const std::string& msg);
	void receiveData(const std::string& data);
	std::string getNextMessage();

private:
	int _fd;
	std::string _ip;
	Server* _server;
	std::string _nickname;
	std::string _username;
	bool _authenticated;
	std::string _recvBuffer;
};

#endif
