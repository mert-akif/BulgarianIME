#pragma once

#include "bgime/Mapping.hpp"
#include "bgime/Transliterator.hpp"

#include <string>

namespace bgime {

enum class EditType {
    PassThrough,
    Append,
    ReplacePending
};

struct ComposeResult {
    EditType edit = EditType::PassThrough;
    std::u32string text;
    bool consumeKey = false;
    bool pending = false;
};

// Streaming composer for the hook/IME layer.
//
// Important invariant:
//   pending_ contains ONLY the shortest suffix that may still become a
//   multi-key transliteration (s/sh, z, t, c, y, ...).
// Normal letters are converted and committed immediately.
class Composer {
public:
    explicit Composer(Mapping mapping = Mapping::turkishQ());

    [[nodiscard]] ComposeResult handleChar(char32_t ch);
    [[nodiscard]] ComposeResult handleBackspace();
    [[nodiscard]] ComposeResult handleEscape();

    // Clears internal composition state WITHOUT changing text already written
    // to the target application. Use this on mouse clicks/focus changes.
    void reset();

    [[nodiscard]] const std::u32string& rawBuffer() const;
    [[nodiscard]] std::u32string currentPreedit() const;
    [[nodiscard]] bool empty() const;

private:
    Mapping mapping_;
    Transliterator transliterator_;
    std::u32string pending_;

    [[nodiscard]] ComposeResult processFreshChar(char32_t ch);
};

} // namespace bgime
