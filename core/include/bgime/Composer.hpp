#pragma once

#include "bgime/Transliterator.hpp"
#include <string>

namespace bgime {

enum class ActionType {
    None,
    UpdatePreedit,
    Commit,
    Cancel,
    PassThrough
};

struct ComposeResult {
    ActionType action = ActionType::None;
    std::u32string preedit;
    std::u32string commit;
    bool consumeKey = false;
};

class Composer {
public:
    explicit Composer(Transliterator transliterator = Transliterator{});

    [[nodiscard]] ComposeResult handleChar(char32_t ch);
    [[nodiscard]] ComposeResult handleBackspace();
    [[nodiscard]] ComposeResult handleEnter();
    [[nodiscard]] ComposeResult handleEscape();
    [[nodiscard]] ComposeResult reset();

    [[nodiscard]] const std::u32string& rawBuffer() const;
    [[nodiscard]] std::u32string currentPreedit() const;
    [[nodiscard]] bool empty() const;

private:
    Transliterator transliterator_;
    std::u32string buffer_;

    [[nodiscard]] ComposeResult updatePreedit() const;
    [[nodiscard]] ComposeResult commitBuffer(std::u32string suffix = U"");
};

} // namespace bgime
