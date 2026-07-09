#include "bgime/Composer.hpp"
#include "bgime/Transliterator.hpp"
#include "bgime/Utf.hpp"

#include <cstdlib>
#include <iostream>
#include <string>

#define REQUIRE_EQ(actual, expected)                                                        \
    do {                                                                                    \
        const std::string actualValue = (actual);                                           \
        const std::string expectedValue = (expected);                                       \
        if (actualValue != expectedValue) {                                                 \
            std::cerr << "Test failed at line " << __LINE__ << "\n"                       \
                      << "Expected: " << expectedValue << "\n"                            \
                      << "Actual:   " << actualValue << "\n";                             \
            return EXIT_FAILURE;                                                           \
        }                                                                                   \
    } while (false)

static std::string utf8(std::u32string_view text) {
    return bgime::utf::u32ToUtf8(text);
}

static std::string convert(const std::string& input) {
    bgime::Transliterator t(bgime::Mapping::turkishQ());
    return t.convertUtf8(input);
}

int main() {
    REQUIRE_EQ(convert("sh"), utf8(U"\u0448"));
    REQUIRE_EQ(convert("ch"), utf8(U"\u0447"));
    REQUIRE_EQ(convert("zh"), utf8(U"\u0436"));
    REQUIRE_EQ(convert("sht"), utf8(U"\u0449"));
    REQUIRE_EQ(convert("ts"), utf8(U"\u0446"));
    REQUIRE_EQ(convert("ya"), utf8(U"\u044F"));
    REQUIRE_EQ(convert("yu"), utf8(U"\u044E"));

    REQUIRE_EQ(convert("Sh"), utf8(U"\u0428"));
    REQUIRE_EQ(convert("SH"), utf8(U"\u0428"));
    REQUIRE_EQ(convert("SHT"), utf8(U"\u0429"));

    REQUIRE_EQ(convert("zdravey"), utf8(U"\u0437\u0434\u0440\u0430\u0432\u0435\u0439"));
    REQUIRE_EQ(convert("az sym student"), utf8(U"\u0430\u0437 \u0441\u0439\u043C \u0441\u0442\u0443\u0434\u0435\u043D\u0442"));
    REQUIRE_EQ(convert(utf8(U"az s\u0131m")), utf8(U"\u0430\u0437 \u0441\u044A\u043C"));
    REQUIRE_EQ(convert(utf8(U"\u015F \u00E7 j \u0131 \u00FC")), utf8(U"\u0448 \u0447 \u0436 \u044A \u044E"));

    bgime::Composer composer;
    auto r1 = composer.handleChar(U's');
    REQUIRE_EQ(bgime::utf::u32ToUtf8(r1.preedit), utf8(U"\u0441"));

    auto r2 = composer.handleChar(U'h');
    REQUIRE_EQ(bgime::utf::u32ToUtf8(r2.preedit), utf8(U"\u0448"));

    auto r3 = composer.handleBackspace();
    REQUIRE_EQ(bgime::utf::u32ToUtf8(r3.preedit), utf8(U"\u0441"));

    auto r4 = composer.handleChar(U'h');
    REQUIRE_EQ(bgime::utf::u32ToUtf8(r4.preedit), utf8(U"\u0448"));

    auto r5 = composer.handleChar(U' ');
    REQUIRE_EQ(bgime::utf::u32ToUtf8(r5.commit), utf8(U"\u0448 "));

    std::cout << "All tests passed.\n";
    return EXIT_SUCCESS;
}
