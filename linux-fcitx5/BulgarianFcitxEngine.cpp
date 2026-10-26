#include "BulgarianFcitxEngine.hpp"

#include "bgime/Utf.hpp"

#include <fcitx-utils/capabilityflags.h>
#include <fcitx-utils/key.h>
#include <fcitx-utils/keysym.h>
#include <fcitx/addonmanager.h>
#include <fcitx/inputcontext.h>
#include <fcitx/inputcontextmanager.h>
#include <fcitx/inputpanel.h>
#include <fcitx/instance.h>
#include <fcitx/text.h>
#include <fcitx/userinterface.h>

namespace bgime::linux_fcitx5 {
namespace {

bool hasShortcutModifier(const fcitx::Key& key) {
    return key.states().testAny(fcitx::KeyStates{
        fcitx::KeyState::Ctrl,
        fcitx::KeyState::Alt,
        fcitx::KeyState::Super,
        fcitx::KeyState::Mod3,
    });
}

bool isCommitBoundary(const fcitx::Key& key) {
    return key.check(FcitxKey_space) ||
           key.check(FcitxKey_Return) ||
           key.check(FcitxKey_KP_Enter) ||
           key.check(FcitxKey_Tab) ||
           key.check(FcitxKey_Delete) ||
           key.isCursorMove();
}

char32_t keyToCodePoint(const fcitx::Key& key) {
    const auto value = fcitx::Key::keySymToUnicode(key.sym());
    return static_cast<char32_t>(value);
}

} // namespace

BulgarianFcitxEngine::BulgarianFcitxEngine(fcitx::Instance* instance)
    : instance_(instance) {
    instance_->inputContextManager().registerProperty(
        "bulgarianimeState", &factory_);
}

BulgarianFcitxEngine::~BulgarianFcitxEngine() {
    if (factory_.registered()) {
        factory_.unregister();
    }
}

void BulgarianFcitxEngine::clearPreedit(fcitx::InputContext* inputContext) {
    inputContext->inputPanel().reset();
    inputContext->updatePreedit();
    inputContext->updateUserInterface(fcitx::UserInterfaceComponent::InputPanel);
}

void BulgarianFcitxEngine::showPreedit(fcitx::InputContext* inputContext,
                                       std::u32string_view text) {
    inputContext->inputPanel().reset();

    const std::string utf8 = bgime::utf::u32ToUtf8(text);
    if (utf8.empty()) {
        inputContext->updatePreedit();
        inputContext->updateUserInterface(fcitx::UserInterfaceComponent::InputPanel);
        return;
    }

    const bool clientPreedit =
        inputContext->capabilityFlags().test(fcitx::CapabilityFlag::Preedit);

    fcitx::Text preedit;
    preedit.append(
        utf8,
        clientPreedit ? fcitx::TextFormatFlag::HighLight
                      : fcitx::TextFormatFlag::NoFlag);

    // Keep the input-method popup stable while typing. The actual pending
    // string is only 1-2 visible characters in this project anyway.
    preedit.setCursor(clientPreedit ? 0 : static_cast<int>(utf8.size()));

    if (clientPreedit) {
        inputContext->inputPanel().setClientPreedit(preedit);
    } else {
        inputContext->inputPanel().setPreedit(preedit);
    }

    inputContext->updatePreedit();
    inputContext->updateUserInterface(fcitx::UserInterfaceComponent::InputPanel);
}

void BulgarianFcitxEngine::commitText(fcitx::InputContext* inputContext,
                                      std::u32string_view text) {
    if (!text.empty()) {
        inputContext->commitString(bgime::utf::u32ToUtf8(text));
    }
}

void BulgarianFcitxEngine::commitPending(fcitx::InputContext* inputContext,
                                         BulgarianState* state) const {
    if (!state->composer.empty()) {
        commitText(inputContext, state->composer.currentPreedit());
        state->composer.reset();
    }
    clearPreedit(inputContext);
}

void BulgarianFcitxEngine::clearState(fcitx::InputContext* inputContext,
                                      BulgarianState* state) const {
    state->composer.reset();
    clearPreedit(inputContext);
}

void BulgarianFcitxEngine::keyEvent(const fcitx::InputMethodEntry& entry,
                                    fcitx::KeyEvent& keyEvent) {
    FCITX_UNUSED(entry);

    // Key releases must pass through untouched.
    if (keyEvent.isRelease()) {
        return;
    }

    auto* inputContext = keyEvent.inputContext();
    auto* state = inputContext->propertyFor(&factory_);
    const fcitx::Key key = keyEvent.key();

    // Keep application shortcuts usable. If a tiny suffix is still pending,
    // commit it first so Ctrl+C / Alt+Tab etc. cannot leave stale state.
    if (hasShortcutModifier(key)) {
        commitPending(inputContext, state);
        return;
    }

    if (key.check(FcitxKey_BackSpace)) {
        if (state->composer.empty()) {
            return; // Normal application backspace.
        }

        const auto result = state->composer.handleBackspace();
        if (result.pending) {
            showPreedit(inputContext, result.text);
        } else {
            clearPreedit(inputContext);
        }
        keyEvent.filterAndAccept();
        return;
    }

    if (key.check(FcitxKey_Escape)) {
        if (!state->composer.empty()) {
            (void)state->composer.handleEscape();
            clearPreedit(inputContext);
            keyEvent.filterAndAccept();
        }
        return;
    }

    // Space / Enter / Tab / cursor movement makes the current suffix final,
    // then the original key itself is forwarded to the application.
    if (isCommitBoundary(key)) {
        commitPending(inputContext, state);
        return;
    }

    const char32_t ch = keyToCodePoint(key);
    if (ch == U'\0') {
        commitPending(inputContext, state);
        return;
    }

    // The core Composer was originally designed for the Windows streaming
    // hook, where the pending Cyrillic glyph is already visible in the app.
    // Fcitx preedit is not committed yet, so remember it in case this new key
    // breaks the sequence (e.g. "s" followed by "d").
    const bool hadPending = !state->composer.empty();
    const std::u32string oldPending =
        hadPending ? state->composer.currentPreedit() : std::u32string{};

    const auto result = state->composer.handleChar(ch);
    if (!result.consumeKey) {
        // Unknown punctuation/character: finish pending text and allow the
        // original key through unchanged.
        if (hadPending) {
            commitText(inputContext, oldPending);
        }
        state->composer.reset();
        clearPreedit(inputContext);
        return;
    }

    // Append while an older pending sequence existed means the new key could
    // NOT extend it. Commit the old preedit first (s + d => с then д).
    if (hadPending && result.edit == bgime::EditType::Append) {
        commitText(inputContext, oldPending);
    }

    if (result.pending) {
        showPreedit(inputContext, result.text);
    } else {
        clearPreedit(inputContext);
        commitText(inputContext, result.text);
    }

    keyEvent.filterAndAccept();
}

void BulgarianFcitxEngine::deactivate(const fcitx::InputMethodEntry& entry,
                                      fcitx::InputContextEvent& event) {
    FCITX_UNUSED(entry);
    auto* inputContext = event.inputContext();
    auto* state = inputContext->propertyFor(&factory_);

    // On focus-out Fcitx/client may already have committed client-preedit;
    // do not double-commit. For an explicit IM switch, preserve the pending
    // character before leaving the engine.
    if (event.type() == fcitx::EventType::InputContextFocusOut) {
        clearState(inputContext, state);
    } else {
        commitPending(inputContext, state);
    }
}

void BulgarianFcitxEngine::reset(const fcitx::InputMethodEntry& entry,
                                 fcitx::InputContextEvent& event) {
    FCITX_UNUSED(entry);
    auto* inputContext = event.inputContext();
    auto* state = inputContext->propertyFor(&factory_);
    clearState(inputContext, state);
}

fcitx::AddonInstance*
BulgarianFcitxEngineFactory::create(fcitx::AddonManager* manager) {
    return new BulgarianFcitxEngine(manager->instance());
}

} // namespace bgime::linux_fcitx5

using BulgarianFcitxEngineFactoryAlias =
    bgime::linux_fcitx5::BulgarianFcitxEngineFactory;
FCITX_ADDON_FACTORY(BulgarianFcitxEngineFactoryAlias);
