#pragma once

#include "bgime/Mapping.hpp"
#include <string>
#include <string_view>

namespace bgime {

class Transliterator {
public:
    explicit Transliterator(Mapping mapping = Mapping::turkishQ());

    [[nodiscard]] std::u32string convert(std::u32string_view input) const;
    [[nodiscard]] std::string convertUtf8(std::string_view input) const;

private:
    Mapping mapping_;

    [[nodiscard]] static std::u32string applyCase(std::u32string_view original,
                                                  std::u32string_view replacement);
};

} // namespace bgime
