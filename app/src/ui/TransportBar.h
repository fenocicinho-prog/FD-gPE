#pragma once

#include <functional>

#include <juce_gui_basics/juce_gui_basics.h>

#include "../engine/EngineHost.h"

namespace fdgpe
{
/** Barre du haut : logo (chargé depuis FD-gPE/logo.png si présent), transport,
    tempo, signature, marqueurs et undo/redo. Les outils (bibliothèque de
    plugins, piano roll, carte de tempo, export, réglages audio, mixeur,
    magnétisme) sont regroupés dans un menu unique, avec un accès direct à la
    bibliothèque de plugins. */
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
    void setSnapLabel (const juce::String&, bool active);

    /** Appelé à chaque nouveau message d'état (affiché dans la barre du bas). */
    std::function<void (const juce::String&)> onStatus;

private:
    void loadLogo();
    void showMenu();
    void handleMenuChoice (int id);
    bool isSnapActive() const;

    EngineHost& host;
    Callbacks callbacks;

    juce::TextButton returnButton, playButton, recButton, loopButton, metroButton,
                      undoButton, redoButton, tapButton,
                      prevMarkerButton, addMarkerButton, nextMarkerButton,
                      menuButton, libraryButton, projectButton,
                      tempoDownButton, tempoUpButton;
    juce::Label positionLabel, secondsLabel, tempoCaption, tempoLabel, sigLabel,
                 cpuLabel, masterLabel, statusLabel;
    juce::Slider masterSlider;

    juce::Image logo;
    juce::String snapText { "1/4" };
    bool snapActive = true;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (TransportBar)
};
} // namespace fdgpe
