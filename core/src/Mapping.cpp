#include "bgime/Mapping.hpp"
#include "bgime/Utf.hpp"

#include <algorithm>

namespace bgime {

Mapping Mapping::standardLatin() {
    Mapping mapping;

    // Multi-key rules must exist, sorting will ensure longest-match behavior.
    mapping.addRule(U"sht", U"\u0449"); // щ
    mapping.addRule(U"zh",  U"\u0436"); // ж
    mapping.addRule(U"ts",  U"\u0446"); // ц
    mapping.addRule(U"ch",  U"\u0447"); // ч
    mapping.addRule(U"sh",  U"\u0448"); // ш
    mapping.addRule(U"yu",  U"\u044E"); // ю
    mapping.addRule(U"ya",  U"\u044F"); // я

    mapping.addRule(U"a", U"\u0430"); // а
    mapping.addRule(U"b", U"\u0431"); // б
    mapping.addRule(U"v", U"\u0432"); // в
    mapping.addRule(U"g", U"\u0433"); // г
    mapping.addRule(U"d", U"\u0434"); // д
    mapping.addRule(U"e", U"\u0435"); // е
    mapping.addRule(U"z", U"\u0437"); // з
    mapping.addRule(U"i", U"\u0438"); // и
    mapping.addRule(U"y", U"\u0439"); // й
    mapping.addRule(U"k", U"\u043A"); // к
    mapping.addRule(U"l", U"\u043B"); // л
    mapping.addRule(U"m", U"\u043C"); // м
    mapping.addRule(U"n", U"\u043D"); // н
    mapping.addRule(U"o", U"\u043E"); // о
    mapping.addRule(U"p", U"\u043F"); // п
    mapping.addRule(U"r", U"\u0440"); // р
    mapping.addRule(U"s", U"\u0441"); // с
    mapping.addRule(U"t", U"\u0442"); // т
    mapping.addRule(U"u", U"\u0443"); // у
    mapping.addRule(U"f", U"\u0444"); // ф
    mapping.addRule(U"h", U"\u0445"); // х

    // Convenience keys for letters that are awkward on a plain Latin layout.
    mapping.addRule(U"q", U"\u044A"); // ъ
    mapping.addRule(U"`", U"\u044A"); // ъ

    mapping.sortLongestFirst();
    return mapping;
}

Mapping Mapping::turkishQ() {
    Mapping mapping = standardLatin();

    // Turkish keyboard friendly shortcuts.
    mapping.addRule(U"\u015Ft", U"\u0449"); // şt -> щ
    mapping.addRule(U"\u015F", U"\u0448"); // ş -> ш
    mapping.addRule(U"\u00E7", U"\u0447"); // ç -> ч
    mapping.addRule(U"j",      U"\u0436"); // j -> ж
    mapping.addRule(U"\u0131", U"\u044A"); // ı -> ъ
    mapping.addRule(U"\u00FC", U"\u044E"); // ü -> ю

    mapping.sortLongestFirst();
    return mapping;
}

void Mapping::addRule(std::u32string key, std::u32string value) {
    rules_.push_back({utf::toLower(key), std::move(value)});
}

void Mapping::sortLongestFirst() {
    std::stable_sort(rules_.begin(), rules_.end(), [](const MappingRule& a, const MappingRule& b) {
        return a.key.size() > b.key.size();
    });
}

const std::vector<MappingRule>& Mapping::rules() const {
    return rules_;
}

bool Mapping::hasPrefix(std::u32string_view text) const {
    const auto lower = utf::toLower(text);
    for (const auto& rule : rules_) {
        if (rule.key.size() > lower.size() && rule.key.substr(0, lower.size()) == lower) {
            return true;
        }
    }
    return false;
}

bool Mapping::hasExactKey(std::u32string_view text) const {
    const auto lower = utf::toLower(text);
    for (const auto& rule : rules_) {
        if (rule.key == lower) {
            return true;
        }
    }
    return false;
}

} // namespace bgime
