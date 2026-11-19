#include "TimelineRuler.h"

#include <cmath>

#include "Palette.h"

namespace fdgpe
{
namespace
{
    // Poignées en bas de la règle : la tête de lecture et ses marqueurs occupent le haut.
    constexpr int handleHeight = 13;
    constexpr int handleHalfWidth = 7;

    /** Garde une borne de boucle dans un domaine valide : jamais négative, et
        toujours à au moins `minGap` de l'autre borne. Fonction pure, testable. */
    double constrainLoopEdge (double candidate, double otherEdge, bool isStart, double minGap) noexcept
    {
        candidate = juce::jmax (0.0, candidate);

        if (isStart)
            return juce::jmin (candidate, juce::jmax (0.0, otherEdge - minGap));

        return juce::jmax (candidate, otherEdge + minGap);
    }
}

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

    // Témoin discret de la zone de boucle : très pâle quand la boucle est coupée,
    // un peu plus visible quand elle est active (les poignées restent seules saturées).
    const auto loop = host.getLoopRange();
    const float loopX = model.secondsToX (loop.first);
    const float loopRight = model.secondsToX (loop.second);

    if (loopRight > loopX && loopX < r.getRight() && loopRight > 0.0f)
    {
        g.setColour (palette::violet.withAlpha (host.isLooping() ? 0.16f : 0.08f));
        g.fillRect (loopX, 0.0f, loopRight - loopX, r.getHeight() - 1.0f);
    }

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

    drawLoopHandle (g, loopX);
    drawLoopHandle (g, loopRight);
}

void TimelineRuler::drawLoopHandle (juce::Graphics& g, float x) const
{
    if (x < -handleHalfWidth || x > getWidth() + handleHalfWidth)
        return;

    const float y = (float) getHeight() - (float) handleHeight - 1.0f;
    const float centre = (float) juce::roundToInt (x);

    g.setColour (palette::violet);
    g.fillRoundedRectangle (centre - 4.0f, y, 8.0f, (float) handleHeight, 2.0f);

    // Deux petits traits clairs : une poignée, pas la tête de lecture.
    g.setColour (palette::violetSoft);
    g.drawVerticalLine (juce::roundToInt (centre) - 1, y + 3.0f, y + (float) handleHeight - 3.0f);
    g.drawVerticalLine (juce::roundToInt (centre) + 1, y + 3.0f, y + (float) handleHeight - 3.0f);
}

TimelineRuler::Drag TimelineRuler::loopHandleAt (const juce::MouseEvent& e) const
{
    // Seule la bande du bas est sensible : ailleurs, la règle place la tête de lecture.
    if (e.y < getHeight() - handleHeight - 2)
        return Drag::none;

    const auto loop = host.getLoopRange();
    const int startX = juce::roundToInt (model.secondsToX (loop.first));
    const int endX = juce::roundToInt (model.secondsToX (loop.second));

    if (std::abs (e.x - startX) <= handleHalfWidth)
        return Drag::start;

    if (std::abs (e.x - endX) <= handleHalfWidth)
        return Drag::end;

    return Drag::none;
}

void TimelineRuler::updateLoopFromDrag (const juce::MouseEvent& e)
{
    const auto loop = host.getLoopRange();
    double seconds = juce::jmax (0.0, model.xToSeconds ((float) e.x));

    if (model.snapBeats > 0.0)
        seconds = host.snapSeconds (seconds, model.snapBeats);

    // Un seizième de temps au tempo courant : la boucle ne peut pas s'inverser.
    const double minGap = (60.0 / juce::jmax (1.0, host.getTempoBpm())) * 0.25;

    if (drag == Drag::start)
        host.setLoopRange (constrainLoopEdge (seconds, loop.second, true, minGap), loop.second);
    else if (drag == Drag::end)
        host.setLoopRange (loop.first, constrainLoopEdge (seconds, loop.first, false, minGap));

    repaint();
}

void TimelineRuler::mouseDown (const juce::MouseEvent& e)
{
    drag = loopHandleAt (e);

    if (drag != Drag::none)
    {
        updateLoopFromDrag (e);
        return;
    }

    if (seek)
        seek (juce::jmax (0.0, model.xToSeconds ((float) e.x)));
}

void TimelineRuler::mouseDrag (const juce::MouseEvent& e)
{
    if (drag != Drag::none)
    {
        updateLoopFromDrag (e);
        return;
    }

    if (seek)
        seek (juce::jmax (0.0, model.xToSeconds ((float) e.x)));
}

void TimelineRuler::mouseUp (const juce::MouseEvent&)
{
    drag = Drag::none;
}

void TimelineRuler::mouseMove (const juce::MouseEvent& e)
{
    setMouseCursor (loopHandleAt (e) != Drag::none ? juce::MouseCursor::LeftRightResizeCursor
                                              : juce::MouseCursor::PointingHandCursor);
}

void TimelineRuler::mouseExit (const juce::MouseEvent&)
{
    // Pendant un glissé, JUCE continue d'envoyer mouseDrag à la règle même si le
    // pointeur en sort : ne pas annuler la poignée avant le mouseUp.
    if (drag == Drag::none)
        setMouseCursor (juce::MouseCursor::PointingHandCursor);
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
