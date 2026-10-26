#include "bgime/Composer.hpp"
#include "bgime/Utf.hpp"

namespace bgime {

Composer::Composer(Mapping mapping)
    : mapping_(std::move(mapping)), transliterator_(mapping_) {}

ComposeResult Composer::processFreshChar(char32_t ch) {
    const std::u32string raw{ch};
    const bool exact = mapping_.hasExactKey(raw);
    const bool canGrow = mapping_.hasPrefix(raw);

    // Unknown characters are best left to the application. This avoids
    // needless SendInput round-trips for punctuation we do not map.
    if (!exact && !canGrow) {
        return {};
    }

    ComposeResult result;
    result.edit = EditType::Append;
    result.text = transliterator_.convert(raw);
    result.consumeKey = true;
    result.pending = canGrow;

    if (canGrow) {
        pending_ = raw;
    } else {
        pending_.clear();
    }

    return result;
}

ComposeResult Composer::handleChar(char32_t ch) {
    if (!utf::isComposeChar(ch)) {
        reset();
        return {};
    }

    if (pending_.empty()) {
        return processFreshChar(ch);
    }

    std::u32string candidate = pending_;
    candidate.push_back(ch);

    const bool exact = mapping_.hasExactKey(candidate);
    const bool canGrow = mapping_.hasPrefix(candidate);

    if (exact || canGrow) {
        ComposeResult result;
        result.edit = EditType::ReplacePending;
        result.text = transliterator_.convert(candidate);
        result.consumeKey = true;
        result.pending = canGrow;

        if (canGrow) {
            pending_ = std::move(candidate);
        } else {
            // The sequence is complete (for example zh, ch, ts, ya, yu,
            // or sht). Nothing else needs to be retained in memory.
            pending_.clear();
        }

        return result;
    }

    // The previous pending suffix cannot be extended by this character.
    // It is already visible in the target app, so commit it in place by
    // simply forgetting it, then process ONLY the newly pressed key.
    pending_.clear();
    return processFreshChar(ch);
}

ComposeResult Composer::handleBackspace() {
    if (pending_.empty()) {
        return {};
    }

    pending_.pop_back();

    ComposeResult result;
    result.edit = EditType::ReplacePending;
    result.consumeKey = true;

    if (pending_.empty()) {
        result.pending = false;
        return result;
    }

    result.text = transliterator_.convert(pending_);
    result.pending = mapping_.hasPrefix(pending_);

    if (!result.pending) {
        pending_.clear();
    }

    return result;
}

ComposeResult Composer::handleEscape() {
    if (pending_.empty()) {
        return {};
    }

    pending_.clear();

    ComposeResult result;
    result.edit = EditType::ReplacePending;
    result.consumeKey = true;
    result.pending = false;
    return result;
}

void Composer::reset() {
    pending_.clear();
}

const std::u32string& Composer::rawBuffer() const {
    return pending_;
}

std::u32string Composer::currentPreedit() const {
    return transliterator_.convert(pending_);
}

bool Composer::empty() const {
    return pending_.empty();
}

} // namespace bgime
