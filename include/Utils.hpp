/**
 * @file Utils.hpp
 * @brief Declaration of the Utils class — stateless string and validation helpers.
 *
 * All methods are static. Utils has no instance state and does not need
 * to be instantiated. It provides general-purpose string manipulation and
 * IRC-specific validation used across the codebase.
 */

#ifndef UTILS_HPP
#define UTILS_HPP

#include <string>
#include <vector>

/**
 * @class Utils
 * @brief Collection of stateless utility functions.
 *
 * String helpers:
 *  - toUpper():  ASCII uppercase conversion (used for command name matching).
 *  - split():    Tokenises a string by a single delimiter character.
 *  - trim():     Strips leading and trailing whitespace.
 *
 * IRC validators:
 *  - isValidNickname():    Enforces RFC 1459 nickname rules.
 *  - isValidChannelName(): Enforces RFC 1459 channel name rules.
 */
class Utils {

	public:
		/**
		 * @brief Converts every ASCII letter in @p str to uppercase.
		 *
		 * Uses std::toupper() with unsigned char casting to avoid UB on
		 * negative char values.
		 *
		 * @param str Input string.
		 * @return A new string with all ASCII letters uppercased.
		 */
		static std::string toUpper(const std::string& str);

		/**
		 * @brief Splits @p str into tokens separated by @p delim.
		 *
		 * Empty tokens are preserved (e.g. splitting "a,,b" by ',' gives
		 * {"a", "", "b"}). The last token is always added, even if the string
		 * does not end with the delimiter.
		 *
		 * @param str   The string to split.
		 * @param delim The single-character delimiter.
		 * @return A vector of substring tokens.
		 */
		static std::vector<std::string> split(const std::string& str, char delim);

		/**
		 * @brief Strips leading and trailing ASCII whitespace from @p str.
		 *
		 * Uses std::isspace() with unsigned char casting.
		 *
		 * @param str Input string.
		 * @return A new string with surrounding whitespace removed.
		 */
		static std::string trim(const std::string& str);

		/**
		 * @brief Validates an IRC nickname against RFC 1459 §2.3.1 rules.
		 *
		 * Rules enforced:
		 *  - Length: 1 to 9 characters.
		 *  - First character: a letter (a-z / A-Z) or a "special" character
		 *    (one of: [ ] \\ ^ _ ` { | }).
		 *  - Remaining characters: letter, digit, hyphen '-', or special.
		 *
		 * Examples:
		 *  - "alice"       → valid
		 *  - "Bob-42"      → valid
		 *  - "9lives"      → invalid (starts with digit)
		 *  - "toolongname" → invalid (> 9 characters)
		 *
		 * @param nick The nickname string to validate.
		 * @return true if the nickname is valid per RFC 1459.
		 */
		static bool isValidNickname(const std::string& nick);

		/**
		 * @brief Validates an IRC channel name against RFC 1459 §1.3 rules.
		 *
		 * Rules enforced:
		 *  - Must start with '#' (only '#' channels are supported).
		 *  - Total length: 2 to 50 characters (the '#' counts as 1).
		 *  - Must not contain: space ' ', bell '\\a', or comma ','.
		 *    (Comma is the JOIN list separator and cannot appear in a name.)
		 *
		 * Examples:
		 *  - "#general"   → valid
		 *  - "#a"         → valid
		 *  - "#"          → invalid (no name after '#')
		 *  - "general"    → invalid (missing '#' prefix)
		 *  - "#bad name"  → invalid (contains space)
		 *
		 * @param name The channel name string to validate.
		 * @return true if the name is valid per RFC 1459.
		 */
		static bool isValidChannelName(const std::string& name);
};

#endif
