#pragma once

#include <string>
#include <string_view>
#include <vector>

namespace bgime {

struct MappingRule {
    std::u32string key;
    std::u32string value;
};

class Mapping {
public:
    static Mapping standardLatin();
    static Mapping turkishQ();

    void addRule(std::u32string key, std::u32string value);
    void sortLongestFirst();

    [[nodiscard]] const std::vector<MappingRule>& rules() const;
    [[nodiscard]] bool hasPrefix(std::u32string_view text) const;
    [[nodiscard]] bool hasExactKey(std::u32string_view text) const;

private:
    std::vector<MappingRule> rules_;
};

} // namespace bgime
