#pragma once

#include <functional>

#include <juce_gui_basics/juce_gui_basics.h>

#include "../engine/EngineHost.h"
#include "TimelineModel.h"

namespace fdgpe
{
/** Règle graduée en mesures et en temps. Un clic ou un glissé déplace la tête de lecture. */
class TimelineRuler final : public juce::Component
{
public:
    TimelineRuler (EngineHost&, TimelineModel&, std::function<void (double)> onSeek);

    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseWheelMove (const juce::MouseEvent&, const juce::MouseWheelDetails&) override;

private:
    EngineHost& host;
    TimelineModel& model;
    std::function<void (double)> seek;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (TimelineRuler)
};

/** Tête de lecture : ligne violette dessinée par-dessus la règle et les pistes.
    Elle ne redessine qu'une bande étroite, pour rester légère. */
class PlayheadOverlay final : public juce::Component
{
public:
    explicit PlayheadOverlay (TimelineModel&);

    void setPosition (double seconds);
    void refreshAll();

    void paint (juce::Graphics&) override;

private:
    int xFor (double seconds) const;

    TimelineModel& model;
    double position = 0.0;
    int lastX = -1000;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PlayheadOverlay)
};
} // namespace fdgpe
