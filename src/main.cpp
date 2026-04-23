#include "Server.hpp"
#include <cstdlib>
#include <iostream>
#include <sstream>
#include <string>

static bool isAllDigits(const std::string& s)
{
	if (s.empty())
		return false;
	for (size_t i = 0; i < s.length(); ++i) {
		if (s[i] < '0' || s[i] > '9')
			return false;
	}
	return true;
}

static int parsePort(const char* arg)
{
	std::string s(arg);
	if (!isAllDigits(s))
		return -1;
	std::istringstream iss(s);
	int p;
	iss >> p;
	if (p < 1 || p > 65535)
		return -1;
	return p;
}

int main(int argc, char** argv)
{
	if (argc != 3) {
		std::cerr << "Usage: " << argv[0] << " <port> <password>" << std::endl;
		return 1;
	}
	int port = parsePort(argv[1]);
	if (port < 0) {
		std::cerr << "Error: invalid port" << std::endl;
		return 1;
	}
	std::string password(argv[2]);
	if (password.empty()) {
		std::cerr << "Error: password must not be empty" << std::endl;
		return 1;
	}
	Server server(port, password);
	server.run();
	return 0;
}
