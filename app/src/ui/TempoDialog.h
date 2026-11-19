#pragma once

#include <memory>
#include <vector>

#include <juce_gui_basics/juce_gui_basics.h>

#include "../engine/EngineHost.h"

namespace fdgpe
{
/** Une ligne de la carte de tempo : position en battements, BPM, signature. */
class TempoRow final : public juce::Component
{
public:
    TempoRow (EngineHost&, const TempoPoint&, std::function<void()> changed);

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    EngineHost& host;
    TempoPoint point;
    std::function<void()> changed;
    juce::Label beatLabel, bpmCaption, sigCaption;
    juce::TextEditor bpmEditor, sigEditor;
    juce::TextButton removeButton;
};

/** Fenêtre de gestion du tempo : carte de tempo, signatures rythmiques, tap. */
class TempoDialog final : public juce::Component
{
public:
    explicit TempoDialog (EngineHost&);

    void paint (juce::Graphics&) override;
    void resized() override;

    static void show (EngineHost&);

private:
    void rebuild();
    void scheduleRebuild();

    EngineHost& host;
    juce::Label titleLabel, hintLabel, statusLabel, currentLabel;
    juce::TextButton addTempoButton, addSigButton, tapButton, closeButton;
    juce::TextEditor bpmEditor, sigEditor;
    juce::Viewport viewport;
    juce::Component list;
    std::vector<std::unique_ptr<TempoRow>> rows;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (TempoDialog)
};
} // namespace fdgpe
