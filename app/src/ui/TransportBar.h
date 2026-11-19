#pragma once

#include <functional>

#include <juce_gui_basics/juce_gui_basics.h>

#include "../engine/EngineHost.h"

namespace fdgpe
{
/** Barre du haut : logo (chargé depuisFD-gPE/logo.png si présent), transport,
    tempo, signature, marqueurs, undo/redo et accès aux outils (plugins, piano
    roll, carte de tempo, export). */
class TransportBar final : public juce::Component
{
public:
    struct Callbacks
    {
        std::function<void()> record;
        std::function<void()> audioSettings;
        std::function<void()> pluginBrowser;
        std::function<void()> pianoRoll;
        std::function<void()> tempoMap;
        std::function<void()> exportWav;
        std::function<void()> projectMenu;
        std::function<void()> addMarker;
        std::function<void()> nextMarker;
        std::function<void()> prevMarker;
        std::function<void()> mixer;
        std::function<void()> cycleSnap;
    };

    TransportBar (EngineHost&, Callbacks);

    void paint (juce::Graphics&) override;
    void resized() override;

    void refresh();
    void setStatus (const juce::String&);
    void setSnapLabel (const juce::String&);

private:
    void loadLogo();

    EngineHost& host;
    Callbacks callbacks;

    juce::TextButton returnButton, playButton, recButton, loopButton, metroButton,
                      undoButton, redoButton, tapButton,
                      prevMarkerButton, addMarkerButton, nextMarkerButton,
                      pluginsButton, pianoButton, tempoButton, exportButton, audioButton,
                      mixerButton, snapButton, projectButton,
                      tempoDownButton, tempoUpButton;
    juce::Label positionLabel, secondsLabel, tempoCaption, tempoLabel, sigLabel,
                 cpuLabel, masterLabel, statusLabel;
    juce::Slider masterSlider;

    juce::Image logo;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (TransportBar)
};
} // namespace fdgpe
