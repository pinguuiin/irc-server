#ifndef CLIENT_HPP
#define CLIENT_HPP

#include <string>

class Server;

class Client {

	public:
		Client(int fd, std::string ip, Server* server);
		~Client();

		const int& getFd() const;
		const std::string& getIp() const;
		const std::string& getNickname() const;
		const std::string& getUsername() const;
		void setFd(int fd);
		void setIp(std::string ip);
		void setNickname(std::string nick);
		void setUsername(std::string user);

		bool isAuthenticated() const;
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
		std::string _recvBuffer;	// incoming raw bytes waiting to be framed
		std::string _sendBuffer; // outgoing bytes waiting to be flushed
};
// =====================================================================
// We're adding _sendBuffer to the private section so sendMessage()
// can store data that couldn't be sent yet (non-blocking socket).
// The public interface (method signatures) is unchanged.
// =====================================================================

#endif
