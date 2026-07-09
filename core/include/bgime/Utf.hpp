#pragma once

#include <string>
#include <string_view>

namespace bgime::utf {

std::u32string utf8ToU32(std::string_view input);
std::string u32ToUtf8(std::u32string_view input);

char32_t toLowerChar(char32_t ch);
char32_t toUpperChar(char32_t ch);

std::u32string toLower(std::u32string_view input);
std::u32string toUpper(std::u32string_view input);

bool isUpperChar(char32_t ch);
bool isLowerChar(char32_t ch);
bool isAsciiLetter(char32_t ch);
bool isComposeChar(char32_t ch);
bool isCommitSeparator(char32_t ch);

} // namespace bgime::utf
