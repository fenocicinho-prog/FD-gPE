#include "TimelineRuler.h"

#include "Palette.h"

namespace fdgpe
{
TimelineRuler::TimelineRuler (EngineHost& h, TimelineModel& m, std::function<void (double)> onSeek)
    : host (h), model (m), seek (std::move (onSeek))
{
    setMouseCursor (juce::MouseCursor::PointingHandCursor);
}

void TimelineRuler::paint (juce::Graphics& g)
{
    auto r = getLocalBounds().toFloat();

    g.setGradientFill (juce::ColourGradient (palette::panelHi, 0.0f, 0.0f, palette::panel, 0.0f, r.getBottom(), false));
    g.fillRect (r);

    g.setColour (palette::blueSoft.withAlpha (0.55f));
    g.drawHorizontalLine (getHeight() - 1, 0.0f, r.getRight());

    const double secondsPerBeat = 60.0 / host.getTempoBpm();
    const double pixelsPerBeat = secondsPerBeat * model.pixelsPerSecond;
    const int beatsPerBar = juce::jmax (1, host.getTimeSignature().first);
    const double pixelsPerBar = pixelsPerBeat * (double) beatsPerBar;

    // on n'écrit un numéro de mesure que s'il y a la place
    int labelEvery = 1;
    while (pixelsPerBar * labelEvery < 56.0)
        labelEvery *= 2;

    g.setFont (juce::Font (juce::FontOptions (11.5f, juce::Font::bold)));

    const long long firstBeat = (long long) std::floor (model.startSeconds / secondsPerBeat);

    for (long long beat = firstBeat; ; ++beat)
    {
        const float x = model.secondsToX ((double) beat * secondsPerBeat);

        if (x > r.getRight())
            break;

        if (beat < 0)
            continue;

        const bool isBar = (beat % beatsPerBar) == 0;

        if (isBar)
        {
            const long long bar = beat / beatsPerBar;

            g.setColour (palette::textDim);
            g.drawVerticalLine (juce::roundToInt (x), r.getHeight() * 0.35f, r.getHeight() - 1.0f);

            if ((bar % labelEvery) == 0)
            {
                g.setColour (palette::text);
                g.drawText (juce::String (bar + 1), juce::roundToInt (x) + 4, 2, 40, 16, juce::Justification::centredLeft);
            }
        }
        else if (pixelsPerBeat >= 12.0)
        {
            g.setColour (palette::line);
            g.drawVerticalLine (juce::roundToInt (x), r.getHeight() * 0.62f, r.getHeight() - 1.0f);
        }
    }

    // Marqueurs du projet : petit drapeau bleu et nom.
    for (const auto& marker : host.getMarkers())
    {
        const float x = model.secondsToX (marker.seconds);

        if (x < -60.0f || x > r.getRight() + 60.0f)
            continue;

        g.setColour (palette::blue);
        g.drawVerticalLine (juce::roundToInt (x), 0.0f, r.getHeight() - 1.0f);

        juce::Path flag;
        flag.addTriangle (x, 1.0f, x + 10.0f, 1.0f, x, 9.0f);
        g.fillPath (flag);

        g.setColour (palette::blue);
        g.setFont (juce::Font (juce::FontOptions (9.5f, juce::Font::bold)));
        g.drawText (marker.name, juce::roundToInt (x) + 5, 1, 82, 11, juce::Justification::centredLeft);
    }
}

void TimelineRuler::mouseDown (const juce::MouseEvent& e)
{
    if (seek)
        seek (juce::jmax (0.0, model.xToSeconds ((float) e.x)));
}

void TimelineRuler::mouseDrag (const juce::MouseEvent& e)
{
    if (seek)
        seek (juce::jmax (0.0, model.xToSeconds ((float) e.x)));
}

void TimelineRuler::mouseWheelMove (const juce::MouseEvent& e, const juce::MouseWheelDetails& wheel)
{
    // sur la règle, la molette défile ou zoome toujours
    juce::MouseWheelDetails w = wheel;

    if (! e.mods.isCtrlDown() && ! e.mods.isShiftDown() && juce::exactlyEqual (w.deltaX, 0.0f))
        w.deltaX = w.deltaY;

    model.handleWheel (e, w, (float) e.x);
}

//==============================================================================
PlayheadOverlay::PlayheadOverlay (TimelineModel& m) : model (m)
{
    setInterceptsMouseClicks (false, false);
}

int PlayheadOverlay::xFor (double seconds) const
{
    return juce::roundToInt (model.secondsToX (seconds));
}

void PlayheadOverlay::setPosition (double seconds)
{
    position = seconds;
    const int x = xFor (seconds);

    if (x != lastX)
    {
        repaint (lastX - 8, 0, 17, getHeight());
        repaint (x - 8, 0, 17, getHeight());
        lastX = x;
    }
}

void PlayheadOverlay::refreshAll()
{
    lastX = xFor (position);
    repaint();
}

void PlayheadOverlay::paint (juce::Graphics& g)
{
    const int x = xFor (position);

    if (x < 0 || x > getWidth())
        return;

    // ligne fine
    g.setColour (palette::blue.withAlpha (0.92f));
    g.fillRect ((float) x - 0.75f, 0.0f, 1.5f, (float) getHeight());

    // repère triangulaire sur la règle
    juce::Path marker;
    marker.addTriangle ((float) x - 7.0f, 0.0f, (float) x + 7.0f, 0.0f, (float) x, 11.0f);
    g.setColour (palette::blue);
    g.fillPath (marker);
}
} // namespace fdgpe
