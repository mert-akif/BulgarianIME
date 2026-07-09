#include "bgime/Transliterator.hpp"
#include "bgime/Utf.hpp"

#include <algorithm>

namespace bgime {

Transliterator::Transliterator(Mapping mapping)
    : mapping_(std::move(mapping)) {}

std::u32string Transliterator::applyCase(std::u32string_view original,
                                         std::u32string_view replacement) {
    if (original.empty()) {
        return std::u32string(replacement);
    }

    bool hasLetter = false;
    bool allUpper = true;

    for (char32_t ch : original) {
        if (utf::isUpperChar(ch) || utf::isLowerChar(ch) || utf::isAsciiLetter(ch)) {
            hasLetter = true;
            if (!utf::isUpperChar(ch) && !(ch >= U'A' && ch <= U'Z')) {
                allUpper = false;
                break;
            }
        }
    }

    if (hasLetter && allUpper) {
        return utf::toUpper(replacement);
    }

    const char32_t first = original.front();
    if (utf::isUpperChar(first) || (first >= U'A' && first <= U'Z')) {
        std::u32string out(replacement);
        if (!out.empty()) {
            out[0] = utf::toUpperChar(out[0]);
        }
        return out;
    }

    return std::u32string(replacement);
}

std::u32string Transliterator::convert(std::u32string_view input) const {
    std::u32string output;
    output.reserve(input.size());

    std::size_t i = 0;
    while (i < input.size()) {
        bool matched = false;

        for (const auto& rule : mapping_.rules()) {
            if (i + rule.key.size() > input.size()) {
                continue;
            }

            const auto originalSlice = input.substr(i, rule.key.size());
            const auto lowerSlice = utf::toLower(originalSlice);
            if (lowerSlice == rule.key) {
                output += applyCase(originalSlice, rule.value);
                i += rule.key.size();
                matched = true;
                break;
            }
        }

        if (!matched) {
            output.push_back(input[i]);
            ++i;
        }
    }

    return output;
}

std::string Transliterator::convertUtf8(std::string_view input) const {
    return utf::u32ToUtf8(convert(utf::utf8ToU32(input)));
}

} // namespace bgime
