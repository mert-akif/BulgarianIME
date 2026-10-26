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

#define REQUIRE_TRUE(value)                                                                 \
    do {                                                                                    \
        if (!(value)) {                                                                     \
            std::cerr << "Test failed at line " << __LINE__ << ": expected true\n";     \
            return EXIT_FAILURE;                                                           \
        }                                                                                   \
    } while (false)

#define REQUIRE_FALSE(value)                                                                \
    do {                                                                                    \
        if (value) {                                                                        \
            std::cerr << "Test failed at line " << __LINE__ << ": expected false\n";    \
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
    REQUIRE_EQ(convert(utf8(U"\u015Ft")), utf8(U"\u0449"));
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

    // Streaming composer: normal letters are committed immediately.
    bgime::Composer composer;
    auto a = composer.handleChar(U'a');
    REQUIRE_TRUE(a.edit == bgime::EditType::Append);
    REQUIRE_EQ(utf8(a.text), utf8(U"\u0430"));
    REQUIRE_FALSE(a.pending);
    REQUIRE_TRUE(composer.empty());

    // Only ambiguous suffixes stay pending.
    auto s = composer.handleChar(U's');
    REQUIRE_TRUE(s.edit == bgime::EditType::Append);
    REQUIRE_EQ(utf8(s.text), utf8(U"\u0441"));
    REQUIRE_TRUE(s.pending);
    REQUIRE_EQ(utf8(composer.rawBuffer()), "s");

    auto sh = composer.handleChar(U'h');
    REQUIRE_TRUE(sh.edit == bgime::EditType::ReplacePending);
    REQUIRE_EQ(utf8(sh.text), utf8(U"\u0448"));
    REQUIRE_TRUE(sh.pending); // sh can still become sht
    REQUIRE_EQ(utf8(composer.rawBuffer()), "sh");

    auto sht = composer.handleChar(U't');
    REQUIRE_TRUE(sht.edit == bgime::EditType::ReplacePending);
    REQUIRE_EQ(utf8(sht.text), utf8(U"\u0449"));
    REQUIRE_FALSE(sht.pending);
    REQUIRE_TRUE(composer.empty());

    // If a pending sequence cannot grow, leave the already-visible char in
    // place and process only the new key.
    auto s2 = composer.handleChar(U's');
    REQUIRE_TRUE(s2.pending);
    auto d = composer.handleChar(U'd');
    REQUIRE_TRUE(d.edit == bgime::EditType::Append);
    REQUIRE_EQ(utf8(d.text), utf8(U"\u0434"));
    REQUIRE_FALSE(d.pending);
    REQUIRE_TRUE(composer.empty());

    // Backspace edits only the tiny pending suffix.
    const auto pendingS = composer.handleChar(U's');
    const auto pendingSh = composer.handleChar(U'h');
    (void)pendingS;
    (void)pendingSh;
    auto back = composer.handleBackspace();
    REQUIRE_TRUE(back.edit == bgime::EditType::ReplacePending);
    REQUIRE_EQ(utf8(back.text), utf8(U"\u0441"));
    REQUIRE_TRUE(back.pending);

    // Reset must forget state without requesting any text deletion.
    composer.reset();
    REQUIRE_TRUE(composer.empty());

    std::cout << "All tests passed.\n";
    return EXIT_SUCCESS;
}
