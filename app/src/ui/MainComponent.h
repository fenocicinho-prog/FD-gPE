#pragma once

#include <memory>

#include <juce_gui_basics/juce_gui_basics.h>

#include "../engine/EngineHost.h"

namespace fdgpe
{
/** Écran du jalon M0 : prouve que la fenêtre s'ouvre et que le moteur tourne.
    Il sera remplacé par la vraie fenêtre principale (pistes, mixeur) au jalon M1. */
class MainComponent final : public juce::Component,
                            private juce::Timer
{
public:
    MainComponent();
    ~MainComponent() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;
    void refreshStatus();
    void showAudioSettings();

    // L'ordre compte : le moteur est déclaré en premier, donc détruit en dernier.
    EngineHost host;

    juce::Label titleLabel, subtitleLabel, statusLabel;
    juce::TextButton playButton, audioButton;
    std::unique_ptr<juce::DialogWindow> audioSettingsWindow;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainComponent)
};
} // namespace fdgpe
