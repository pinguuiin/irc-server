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
		Server* getServer() const;

		void setFd(int fd);
		void setIp(std::string ip);
		void setNickname(std::string nick);
		void setUsername(std::string user);

		// Registration step trackers
		bool isPassOk() const;
		bool isNickSet() const;
		bool isUserSet() const;
		void setPassOk();
		void setNickSet();
		void setUserSet();

		bool isAuthenticated() const;
		void authenticate();

		void receiveAndHandleMessage(const char *buf);
		void appendSendBuffer(const std::string& msg);
		bool sendPendingMessage();
		std::string getNextMessage();

	private:
		int _fd;
		std::string _ip;
		Server* _server;
		std::string _nickname;
		std::string _username;
		// Three separate flags - one per registration step.
		// _authenicate only becomes true once ALL three are done.
		bool _passOk;
		bool _nickSet;
		bool _userSet;
		bool _authenticated;
		std::string _recvBuffer;	// incoming raw bytes waiting to be framed
		std::string _sendBuffer; // outgoing bytes waiting to be flushed
};

#endif
