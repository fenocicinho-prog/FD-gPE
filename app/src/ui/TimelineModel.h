#pragma once

#include <functional>

#include <juce_gui_basics/juce_gui_basics.h>

namespace fdgpe
{
/** Vue partagée de la ligne de temps : échelle (pixels par seconde) et début
    visible. La règle, les pistes et la tête de lecture l'utilisent toutes. */
struct TimelineModel
{
    double pixelsPerSecond = 60.0;
    double startSeconds = 0.0;
    double snapBeats = 1.0;          // 0 = magnétisme coupé
    std::function<void()> changed;

    float secondsToX (double seconds) const noexcept
    {
        return (float) ((seconds - startSeconds) * pixelsPerSecond);
    }

    double xToSeconds (float x) const noexcept
    {
        return startSeconds + (double) x / pixelsPerSecond;
    }

    void notify() const
    {
        if (changed)
            changed();
    }

    /** Molette : Ctrl = zoom, Maj ou molette horizontale = défilement.
        Renvoie false si l'événement n'est pas pour nous (défilement vertical des pistes). */
    bool handleWheel (const juce::MouseEvent& e, const juce::MouseWheelDetails& wheel, float xInLane)
    {
        if (e.mods.isCtrlDown())
        {
            const double anchor = xToSeconds (xInLane);
            pixelsPerSecond = juce::jlimit (10.0, 400.0, pixelsPerSecond * (1.0 + (double) wheel.deltaY * 0.8));
            startSeconds = juce::jmax (0.0, anchor - (double) xInLane / pixelsPerSecond);
            notify();
            return true;
        }

        if (e.mods.isShiftDown() || ! juce::exactlyEqual (wheel.deltaX, 0.0f))
        {
            const double delta = ! juce::exactlyEqual (wheel.deltaX, 0.0f) ? (double) wheel.deltaX : (double) wheel.deltaY;
            startSeconds = juce::jmax (0.0, startSeconds - delta * 600.0 / pixelsPerSecond);
            notify();
            return true;
        }

        return false;
    }
};
} // namespace fdgpe
