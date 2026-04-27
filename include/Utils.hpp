#ifndef UTILS_HPP
#define UTILS_HPP

#include <string>
#include <vector>

class Utils {

	public:
		static std::string toUpper(const std::string& str);
		static std::vector<std::string> split(const std::string& str, char delim);
		static std::string trim(const std::string& str);
		static bool isValidNickname(const std::string& nick);
		static bool isValidChannelName(const std::string& name);
};

#endif
