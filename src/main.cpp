#include "Server.hpp"
#include <iostream>
#include <sstream>

static bool isAllDigits(const std::string& s)
{
	if (s.empty())
		return false;
	for (size_t i = 0; i < s.length(); ++i) {
		if (!std::isdigit(s[i]))
			return false;
	}
	return true;
}

static int stringToPort(const char* arg)
{
	std::string s(arg);
	if (!isAllDigits(s))
		return -1;
	std::istringstream iss(s);
	int p;
	if (!(iss >> p))
		return -1;
	if (p < 1024 || p > 65535)
		return -1;
	return p;
}

int main(int argc, char *argv[])
{
	if (argc != 3) {
		std::cerr << "Invalid input. Usage: " << argv[0] << " <port> <password>"
				  << std::endl;
		return 1;
	}
	int port = stringToPort(argv[1]);
	if (port == -1) {
		std::cerr << "Invalid port. Valid range: 1024 - 65535" << std::endl;
		return 1;
	}
	std::string password(argv[2]);
	if (password.empty()) {
		std::cerr << "Invalid password. Password must not be empty" << std::endl;
		return 1;
	}

	std::cout << "\n==================== IRC SERVER ====================\n" << std::endl;

	Server server(port, password);
	try {
		server.initServer();

		/* Anything else */

	}
	catch(const std::exception &e) {
		std::cerr << e.what() << std::endl;
		return 1;
	}

	return 0;
}
