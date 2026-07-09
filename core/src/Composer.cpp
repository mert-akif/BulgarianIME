#include "bgime/Composer.hpp"
#include "bgime/Utf.hpp"

namespace bgime {

Composer::Composer(Transliterator transliterator)
    : transliterator_(std::move(transliterator)) {}

ComposeResult Composer::updatePreedit() const {
    ComposeResult result;
    result.action = ActionType::UpdatePreedit;
    result.preedit = transliterator_.convert(buffer_);
    result.consumeKey = true;
    return result;
}

ComposeResult Composer::commitBuffer(std::u32string suffix) {
    ComposeResult result;
    result.action = ActionType::Commit;
    result.commit = transliterator_.convert(buffer_) + suffix;
    result.consumeKey = true;
    buffer_.clear();
    return result;
}

ComposeResult Composer::handleChar(char32_t ch) {
    if (utf::isComposeChar(ch)) {
        buffer_.push_back(ch);
        return updatePreedit();
    }

    if (!buffer_.empty()) {
        return commitBuffer(std::u32string{ch});
    }

    ComposeResult result;
    result.action = ActionType::PassThrough;
    result.commit = std::u32string{ch};
    result.consumeKey = false;
    return result;
}

ComposeResult Composer::handleBackspace() {
    ComposeResult result;

    if (buffer_.empty()) {
        result.action = ActionType::PassThrough;
        result.consumeKey = false;
        return result;
    }

    buffer_.pop_back();
    result.action = ActionType::UpdatePreedit;
    result.preedit = transliterator_.convert(buffer_);
    result.consumeKey = true;
    return result;
}

ComposeResult Composer::handleEnter() {
    if (!buffer_.empty()) {
        return commitBuffer(U"\n");
    }

    ComposeResult result;
    result.action = ActionType::PassThrough;
    result.commit = U"\n";
    result.consumeKey = false;
    return result;
}

ComposeResult Composer::handleEscape() {
    ComposeResult result;
    result.action = ActionType::Cancel;
    result.consumeKey = !buffer_.empty();
    buffer_.clear();
    return result;
}

ComposeResult Composer::reset() {
    ComposeResult result;
    result.action = ActionType::Cancel;
    result.consumeKey = !buffer_.empty();
    buffer_.clear();
    return result;
}

const std::u32string& Composer::rawBuffer() const {
    return buffer_;
}

std::u32string Composer::currentPreedit() const {
    return transliterator_.convert(buffer_);
}

bool Composer::empty() const {
    return buffer_.empty();
}

} // namespace bgime
