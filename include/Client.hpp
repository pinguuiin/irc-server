#pragma once

#include <string>

class Client {

	public:
		Client() = default;
		~Client();

	private:
		int	_fd{-1};
		std::string _ip;
};
