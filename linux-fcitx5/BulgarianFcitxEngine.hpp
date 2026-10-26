#pragma once

#include "bgime/Composer.hpp"

#include <fcitx/addonfactory.h>
#include <fcitx/event.h>
#include <fcitx/inputcontextproperty.h>
#include <fcitx/inputmethodengine.h>
#include <fcitx/instance.h>

#include <string_view>

namespace bgime::linux_fcitx5 {

struct BulgarianState final : public fcitx::InputContextProperty {
    bgime::Composer composer{bgime::Mapping::turkishQ()};
};

class BulgarianFcitxEngine final : public fcitx::InputMethodEngineV2 {
public:
    explicit BulgarianFcitxEngine(fcitx::Instance* instance);
    ~BulgarianFcitxEngine() override;

    void keyEvent(const fcitx::InputMethodEntry& entry,
                  fcitx::KeyEvent& keyEvent) override;

    void deactivate(const fcitx::InputMethodEntry& entry,
                    fcitx::InputContextEvent& event) override;

    void reset(const fcitx::InputMethodEntry& entry,
               fcitx::InputContextEvent& event) override;

private:
    fcitx::Instance* instance_;

    fcitx::FactoryFor<BulgarianState> factory_{
        [](fcitx::InputContext&) { return new BulgarianState; }};

    static void clearPreedit(fcitx::InputContext* inputContext);
    static void showPreedit(fcitx::InputContext* inputContext,
                            std::u32string_view text);
    static void commitText(fcitx::InputContext* inputContext,
                           std::u32string_view text);

    void commitPending(fcitx::InputContext* inputContext,
                       BulgarianState* state) const;
    void clearState(fcitx::InputContext* inputContext,
                    BulgarianState* state) const;
};

class BulgarianFcitxEngineFactory final : public fcitx::AddonFactory {
public:
    fcitx::AddonInstance* create(fcitx::AddonManager* manager) override;
};

} // namespace bgime::linux_fcitx5
