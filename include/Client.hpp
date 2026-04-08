#pragma once

#include <string>

class Client {

	public:
		Client() = default;
		~Client();

		const int &getFd() const;
		const std::string &getIp() const;
		void setFd(int fd);
		void setIp(std::string ip);

	private:
		int	_fd{-1};
		std::string _ip;
};
