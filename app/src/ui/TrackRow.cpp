#include "TrackRow.h"

#include <cstddef>

#include "FdLookAndFeel.h"
#include "Palette.h"
#include "Text.h"

namespace fdgpe
{
namespace
{
    void styleSmallLabel (juce::Label& label, const juce::String& text, float size, juce::Colour colour,
                          juce::Justification justification)
    {
        label.setText (text, juce::dontSendNotification);
        label.setFont (juce::Font (juce::FontOptions (size, juce::Font::bold)));
        label.setColour (juce::Label::textColourId, colour);
        label.setJustificationType (justification);
        label.setInterceptsMouseClicks (false, false);
    }
}

//==============================================================================
// PluginStrip
//==============================================================================
PluginStrip::PluginStrip (EngineHost& h, te::AudioTrack& t, Callbacks cb)
    : host (h), track (t), callbacks (std::move (cb))
{
    refresh();
}

int PluginStrip::tileCount() const
{
    return (int) names.size() + 1;
}

juce::Rectangle<int> PluginStrip::tileBounds (int index) const
{
    const int tileW = 88;
    const int gap = 5;
    return juce::Rectangle<int> (index * (tileW + gap), 0, tileW, getHeight());
}

void PluginStrip::refresh()
{
    names.clear();
    enabled.clear();

    for (auto plugin : host.getTrackPlugins (track))
    {
        if (plugin == nullptr)
            continue;

        names.push_back (EngineHost::getPluginDisplayName (*plugin));
        enabled.push_back (plugin->isEnabled());
    }

    repaint();
}

void PluginStrip::resized()
{
    repaint();
}

void PluginStrip::paint (juce::Graphics& g)
{
    for (int i = 0; i < tileCount(); ++i)
    {
        const auto r = tileBounds (i).toFloat().reduced (0.0f, 1.0f);

        if (i == (int) names.size())
        {
            // tuile "+"
            g.setColour (hovered == i ? palette::blueSoft : palette::panel);
            g.fillRoundedRectangle (r, 6.0f);
            g.setColour (hovered == i ? palette::blue : palette::outline);
            g.drawRoundedRectangle (r, 6.0f, 1.0f);
            g.setColour (hovered == i ? palette::blue : palette::textDim);
            g.setFont (juce::Font (juce::FontOptions (15.0f, juce::Font::bold)));
            g.drawText ("+", r, juce::Justification::centred);
            continue;
        }

        const bool on = enabled[(std::size_t) i];

        g.setColour (on ? palette::blueFaint : palette::panel);
        g.fillRoundedRectangle (r, 6.0f);
        g.setColour (on ? palette::blue : palette::outline);
        g.drawRoundedRectangle (r, 6.0f, hovered == i ? 1.8f : 1.0f);

        // témoin d'activation
        g.setColour (on ? palette::blue : palette::textFaint);
        g.fillEllipse (r.getX() + 8.0f, r.getCentreY() - 3.0f, 6.0f, 6.0f);

        // croix de suppression
        g.setColour (hovered == i ? palette::red : palette::textFaint);
        g.setFont (juce::Font (juce::FontOptions (11.0f, juce::Font::bold)));
        g.drawText ("x", r.getRight() - 16.0f, r.getY(), 13.0f, r.getHeight(), juce::Justification::centred);

        g.setColour (on ? palette::text : palette::textDim);
        g.setFont (juce::Font (juce::FontOptions (10.5f, juce::Font::bold)));
        g.drawFittedText (names[(std::size_t) i],
                          r.reduced (18.0f, 2.0f).withTrimmedRight (10.0f).toNearestInt(),
                          juce::Justification::centredLeft, 1);
    }
}

void PluginStrip::mouseMove (const juce::MouseEvent& e)
{
    int found = -1;

    for (int i = 0; i < tileCount(); ++i)
        if (tileBounds (i).contains (e.getPosition()))
            found = i;

    if (found != hovered)
    {
        hovered = found;
        repaint();
    }
}

void PluginStrip::mouseExit (const juce::MouseEvent&)
{
    hovered = -1;
    repaint();
}

void PluginStrip::mouseDown (const juce::MouseEvent& e)
{
    const int index = e.x / 93;

    if (index < 0 || index >= tileCount() || ! tileBounds (index).contains (e.getPosition()))
        return;

    if (index == (int) names.size())
    {
        if (callbacks.add)
            callbacks.add();
        return;
    }

    auto plugins = host.getTrackPlugins (track);

    if (index >= (int) plugins.size() || plugins[(std::size_t) index] == nullptr)
        return;

    const auto r = tileBounds (index);

    if (e.mods.isPopupMenu())
    {
        juce::PopupMenu menu;
        menu.addItem (1, tr ("Ouvrir l'éditeur"));
        menu.addItem (2, tr ("Monter dans la chaîne"));
        menu.addItem (3, tr ("Descendre dans la chaîne"));
        menu.addSeparator();
        menu.addItem (4, plugins[(std::size_t) index]->isEnabled() ? tr ("Désactiver") : tr ("Activer"));
        menu.addItem (5, tr ("Retirer le plugin"));

        if (auto choice = menu.showAt (this, e.x, e.y))
        {
            switch (choice)
            {
                case 1: if (callbacks.editor) callbacks.editor (*plugins[(std::size_t) index]); break;
                case 2: host.movePlugin (track, *plugins[(std::size_t) index], -1); break;
                case 3: host.movePlugin (track, *plugins[(std::size_t) index], 1); break;
                case 4: host.setPluginEnabled (*plugins[(std::size_t) index], ! plugins[(std::size_t) index]->isEnabled()); break;
                case 5: host.removePluginFromTrack (track, *plugins[(std::size_t) index]); break;
                default: break;
            }
        }
    }
    else if (e.x >= r.getRight() - 18)
    {
        host.removePluginFromTrack (track, *plugins[(std::size_t) index]);
    }
    else if (e.x <= r.getX() + 18)
    {
        host.setPluginEnabled (*plugins[(std::size_t) index], ! plugins[(std::size_t) index]->isEnabled());
    }
    else if (callbacks.editor)
    {
        callbacks.editor (*plugins[(std::size_t) index]);
    }

    refresh();
    host.saveProject();

    if (callbacks.changed)
        callbacks.changed();
}

void PluginStrip::mouseDoubleClick (const juce::MouseEvent& e)
{
    const int index = e.x / 93;
    auto plugins = host.getTrackPlugins (track);

    if (index >= 0 && index < (int) plugins.size() && plugins[(std::size_t) index] != nullptr && callbacks.editor)
        callbacks.editor (*plugins[(std::size_t) index]);
}

//==============================================================================
// TrackRow
//==============================================================================
TrackRow::TrackRow (EngineHost& h, te::AudioTrack& t, TimelineModel& m, Callbacks cb)
    : host (h), track (t), model (m), callbacks (std::move (cb)),
      pluginStrip (host, track,
      {
          [this] { if (callbacks.addPlugin) callbacks.addPlugin (track); },
          [this] (te::Plugin& p) { if (callbacks.pluginEditor) callbacks.pluginEditor (p); },
          [this] { host.saveProject(); }
      })
{
    styleSmallLabel (kindLabel, host.isMidiTrack (track) ? "MIDI" : "AUDIO", 10.5f,
                     palette::textDim, juce::Justification::centred);

    nameLabel.setText (track.getName(), juce::dontSendNotification);
    nameLabel.setFont (juce::Font (juce::FontOptions (16.0f, juce::Font::bold)));
    nameLabel.setEditable (false, true);
    nameLabel.onTextChange = [this]
    {
        const auto newName = nameLabel.getText().trim();

        if (newName.isNotEmpty())
            host.renameTrack (track, newName);
        else
            nameLabel.setText (track.getName(), juce::dontSendNotification);
    };

    styleSmallLabel (volLabel, "VOL", 9.5f, palette::textDim, juce::Justification::centredLeft);
    styleSmallLabel (panLabel, "PAN", 9.5f, palette::textDim, juce::Justification::centredLeft);

    FdLookAndFeel::setLed (armButton, palette::red);
    FdLookAndFeel::setLed (muteButton, palette::amber);
    FdLookAndFeel::setLed (soloButton, palette::blue);

    armButton.onClick = [this]
    {
        host.setTrackArmed (track, ! host.isTrackArmed (track));

        if (callbacks.changed)
            callbacks.changed();
    };

    muteButton.onClick = [this] { track.setMute (! track.isMuted (false)); refreshControls(); };
    soloButton.onClick = [this] { track.setSolo (! track.isSolo (false));  refreshControls(); };
    deleteButton.onClick = [this] { if (callbacks.remove) callbacks.remove (track); };

    volumeSlider.setSliderStyle (juce::Slider::LinearHorizontal);
    volumeSlider.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
    volumeSlider.setRange (0.0, 1.0, 0.0);
    volumeSlider.setDoubleClickReturnValue (true, (double) te::decibelsToVolumeFaderPosition (0.0f));
    volumeSlider.onValueChange = [this]
    {
        if (auto* vp = track.getVolumePlugin())
            vp->setSliderPos ((float) volumeSlider.getValue());
    };

    panSlider.setSliderStyle (juce::Slider::LinearHorizontal);
    panSlider.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
    panSlider.setRange (-1.0, 1.0, 0.0);
    panSlider.setDoubleClickReturnValue (true, 0.0);
    panSlider.onValueChange = [this]
    {
        if (auto* vp = track.getVolumePlugin())
            vp->setPan ((float) panSlider.getValue());
    };

    for (juce::Component* c : { (juce::Component*) &kindLabel, (juce::Component*) &nameLabel, (juce::Component*) &volLabel,
                                (juce::Component*) &panLabel, (juce::Component*) &armButton, (juce::Component*) &muteButton,
                                (juce::Component*) &soloButton, (juce::Component*) &deleteButton,
                                (juce::Component*) &volumeSlider, (juce::Component*) &panSlider, (juce::Component*) &pluginStrip })
        addAndMakeVisible (c);

    if (auto* meter = track.getLevelMeterPlugin())
        meter->measurer.addClient (meterClient);

    refreshControls();
}

TrackRow::~TrackRow()
{
    if (auto* meter = track.getLevelMeterPlugin())
        meter->measurer.removeClient (meterClient);
}

//==============================================================================
juce::Rectangle<int> TrackRow::getLaneBounds() const
{
    return getLocalBounds().withTrimmedLeft (headerWidth);
}

void TrackRow::resized()
{
    auto header = getLocalBounds().removeFromLeft (headerWidth).reduced (10, 8);

    meterBounds = header.removeFromRight (8);
    header.removeFromRight (10);

    auto row1 = header.removeFromTop (22);
    kindLabel.setBounds (row1.removeFromLeft (48));
    row1.removeFromLeft (8);
    deleteButton.setBounds (row1.removeFromRight (24));
    row1.removeFromRight (6);
    nameLabel.setBounds (row1);

    header.removeFromTop (8);
    auto row2 = header.removeFromTop (28);
    const int bw = (row2.getWidth() - 12) / 3;
    armButton.setBounds (row2.removeFromLeft (bw));
    row2.removeFromLeft (6);
    muteButton.setBounds (row2.removeFromLeft (bw));
    row2.removeFromLeft (6);
    soloButton.setBounds (row2.removeFromLeft (bw));

    header.removeFromTop (10);
    auto row3 = header.removeFromTop (22);
    volLabel.setBounds (row3.removeFromLeft (26));
    volumeSlider.setBounds (row3.removeFromLeft (row3.getWidth() * 55 / 100));
    row3.removeFromLeft (8);
    panLabel.setBounds (row3.removeFromLeft (26));
    panSlider.setBounds (row3);

    header.removeFromTop (12);
    auto stripLabel = header.removeFromTop (14);

    pluginStrip.setBounds (header.removeFromTop (30));

    // Le petit titre est dessiné dans paint().
    juce::ignoreUnused (stripLabel);
}

void TrackRow::refreshControls()
{
    const bool armed = host.isTrackArmed (track);

    armButton.setToggleState (armed, juce::dontSendNotification);
    muteButton.setToggleState (track.isMuted (false), juce::dontSendNotification);
    soloButton.setToggleState (track.isSolo (false), juce::dontSendNotification);

    if (auto* vp = track.getVolumePlugin())
    {
        if (! volumeSlider.isMouseButtonDown())
            volumeSlider.setValue (vp->getSliderPos(), juce::dontSendNotification);

        if (! panSlider.isMouseButtonDown())
            panSlider.setValue (vp->getPan(), juce::dontSendNotification);
    }

    if (nameLabel.getText() != track.getName() && ! nameLabel.isBeingEdited())
        nameLabel.setText (track.getName(), juce::dontSendNotification);

    pluginStrip.refresh();
    repaint (getLocalBounds().removeFromLeft (headerWidth));
}

void TrackRow::updateMeter()
{
    float db = -100.0f;

    for (int ch = 0; ch < meterClient.getNumChannelsUsed(); ++ch)
        db = juce::jmax (db, meterClient.getAndClearAudioLevel (ch).dB);

    const float target = juce::jlimit (0.0f, 1.0f, juce::jmap (db, -60.0f, 0.0f, 0.0f, 1.0f));
    const float previous = meterLevel;

    meterLevel = target > meterLevel ? target : meterLevel * 0.80f;

    if (std::abs (meterLevel - previous) > 0.02f)
        repaint (meterBounds.expanded (2));
}

void TrackRow::repaintLane()
{
    repaint (getLaneBounds());
}

void TrackRow::selectClip (te::Clip* clip)
{
    if (selectedClip != clip)
    {
        selectedClip = clip;
        repaintLane();
    }
}

//==============================================================================
void TrackRow::paint (juce::Graphics& g)
{
    auto header = getLocalBounds().removeFromLeft (headerWidth).toFloat();

    g.setGradientFill (juce::ColourGradient (palette::panelHi, 0.0f, header.getY(),
                                             palette::panel, 0.0f, header.getBottom(), false));
    g.fillRect (header);

    g.setColour (juce::Colours::white.withAlpha (0.9f));
    g.drawHorizontalLine (0, header.getX(), header.getRight());

    // titre du bandeau de plugins
    g.setColour (palette::textDim);
    g.setFont (juce::Font (juce::FontOptions (9.5f, juce::Font::bold)));
    g.drawText (tr ("PLUGINS"), header.getX() + 10.0f, header.getBottom() - 48.0f, 100.0f, 12.0f,
                juce::Justification::centredLeft);

    paintMeter (g);

    auto lane = getLaneBounds();
    g.setColour (host.isMidiTrack (track) ? palette::midiClip : palette::audioClip);
    g.fillRect (lane);

    {
        juce::Graphics::ScopedSaveState save (g);
        g.reduceClipRegion (lane);
        paintLane (g, lane);
    }

    g.setColour (palette::blueSoft);
    g.drawHorizontalLine (getHeight() - 1, 0.0f, (float) getWidth());
    g.setColour (palette::outline);
    g.drawVerticalLine (headerWidth - 1, 0.0f, (float) getHeight());
}

void TrackRow::paintMeter (juce::Graphics& g)
{
    const auto r = meterBounds.toFloat().reduced (0.0f, 2.0f);
    const int segments = 18;
    const float gap = 1.5f;
    const float segH = (r.getHeight() - gap * (float) (segments - 1)) / (float) segments;
    const int lit = juce::roundToInt (meterLevel * (float) segments);

    for (int i = 0; i < segments; ++i)
    {
        const float y = r.getBottom() - (float) (i + 1) * segH - (float) i * gap;
        const auto colour = i < 13 ? palette::green : (i < 16 ? palette::amber : palette::red);

        g.setColour (i < lit ? colour : palette::line);
        g.fillRoundedRectangle (r.getX(), y, r.getWidth(), segH, 1.0f);
    }
}

void TrackRow::paintLane (juce::Graphics& g, juce::Rectangle<int> lane)
{
    const auto clips = track.getClips();

    // Nettoie le cache : le pointeur d'un clip supprimé peut être réutilisé.
    for (auto it = thumbnails.begin(); it != thumbnails.end(); )
    {
        bool present = false;
        for (auto* c : clips)
            if (c == it->first) { present = true; break; }
        if (present) ++it;
        else it = thumbnails.erase (it);
    }

    const double secondsPerBar = 4.0 * 60.0 / host.getTempoBpm();

    if (secondsPerBar * model.pixelsPerSecond >= 10.0)
    {
        g.setColour (palette::outline.withAlpha (0.28f));

        for (double t = std::floor (model.startSeconds / secondsPerBar) * secondsPerBar; ; t += secondsPerBar)
        {
            const float x = (float) lane.getX() + model.secondsToX (t);

            if (x > (float) lane.getRight())
                break;

            g.drawVerticalLine (juce::roundToInt (x), (float) lane.getY(), (float) lane.getBottom());
        }
    }

    const auto bounds = lane.toFloat();
    const bool midiTrack = host.isMidiTrack (track);
    const auto base = midiTrack ? palette::midiClipHi : palette::audioClipHi;
    const auto accent = host.getTrackColour (track);

    for (auto* clip : clips)
    {
        const auto range = clip->getPosition().time;
        const float x1 = bounds.getX() + model.secondsToX (range.getStart().inSeconds());
        const float x2 = bounds.getX() + model.secondsToX (range.getEnd().inSeconds());

        if (x2 < bounds.getX() || x1 > bounds.getRight())
            continue;

        juce::Rectangle<float> r (x1, bounds.getY() + 10.0f, juce::jmax (4.0f, x2 - x1), bounds.getHeight() - 20.0f);
        const bool selected = (clip == selectedClip);

        g.setGradientFill (juce::ColourGradient (base.withAlpha (0.95f), 0.0f, r.getY(),
                                                 accent.withAlpha (0.45f), 0.0f, r.getBottom(), false));
        g.fillRoundedRectangle (r, 5.0f);

        g.setColour (selected ? palette::blue : accent);
        g.drawRoundedRectangle (r.reduced (0.5f), 5.0f, selected ? 2.4f : 1.2f);

        // Contenu réel du clip : forme d'onde pour l'audio, notes pour le MIDI.
        {
            juce::Graphics::ScopedSaveState save (g);
            g.reduceClipRegion (r.toNearestInt().reduced (2));

            if (auto* midi = dynamic_cast<te::MidiClip*> (clip))
            {
                const auto notes = host.getNotes (*midi);
                const double clipLengthBeats = juce::jmax (0.25,
                    host.secondsToBeat (range.getEnd().inSeconds())
                        - host.secondsToBeat (range.getStart().inSeconds()));

                int lowest = 127, highest = 0;

                for (const auto& n : notes)
                {
                    lowest = juce::jmin (lowest, n.pitch);
                    highest = juce::jmax (highest, n.pitch);
                }

                if (! notes.empty())
                {
                    const int span = juce::jmax (12, highest - lowest + 1);
                    const float noteH = juce::jmax (1.5f, (r.getHeight() - 22.0f) / (float) span);

                    for (const auto& n : notes)
                    {
                        const float nx = r.getX() + (float) (n.startBeat / clipLengthBeats) * r.getWidth();
                        const float nw = juce::jmax (1.5f, (float) (n.lengthBeats / clipLengthBeats) * r.getWidth());
                        const float ny = r.getBottom() - 4.0f
                                       - (float) (n.pitch - lowest + 1) * noteH;
                        g.setColour (palette::blue.withAlpha (0.72f));
                        g.fillRoundedRectangle (nx, ny, nw, juce::jmax (1.2f, noteH - 0.8f), 1.0f);
                    }
                }
            }
            else if (dynamic_cast<te::AudioClipBase*> (clip) != nullptr)
            {
                drawWaveform (g, *clip, r.reduced (1.0f, 0.0f));
            }
        }

        g.setColour (palette::text);
        g.setFont (juce::Font (juce::FontOptions (12.0f, juce::Font::bold)));
        g.drawFittedText (clip->getName(), r.toNearestInt().reduced (8, 4), juce::Justification::topLeft, 1);
    }

    // Enregistrement en cours
    if (host.isRecording() && host.isTrackArmed (track))
    {
        const float x1 = bounds.getX() + model.secondsToX (host.getRecordStartSeconds());
        const float x2 = bounds.getX() + model.secondsToX (host.getPositionSeconds());

        juce::Rectangle<float> r (x1, bounds.getY() + 10.0f, juce::jmax (2.0f, x2 - x1), bounds.getHeight() - 20.0f);
        g.setColour (palette::red.withAlpha (0.20f));
        g.fillRoundedRectangle (r, 5.0f);
        g.setColour (palette::red);
        g.drawRoundedRectangle (r.reduced (0.5f), 5.0f, 1.4f);
    }
}

void TrackRow::drawWaveform (juce::Graphics& g, te::Clip& clip, juce::Rectangle<float> area)
{
    auto* audio = dynamic_cast<te::AudioClipBase*> (&clip);

    if (audio == nullptr)
        return;

    auto& thumbnail = thumbnails[&clip];

    if (thumbnail == nullptr)
    {
        const auto file = audio->getAudioFile();

        if (file.getFile().existsAsFile())
            thumbnail = std::make_unique<te::SmartThumbnail> (host.getEngine(), file, *this, &host.getEdit());
    }

    if (thumbnail == nullptr)
        return;

    // Ne dessine que la partie réellement visible : la miniature garde ainsi
    // sa résolution sur la lane au lieu de s'étaler sur tout le clip.
    const auto lane = getLaneBounds().toFloat();
    const auto visible = area.getIntersection (lane);

    if (visible.isEmpty())
        return;

    const auto position = clip.getPosition();
    const double clipStart = position.time.getStart().inSeconds();
    const double offsetSeconds = position.offset.inSeconds();

    // Bords visibles en temps timeline (secondsToX est relatif au bord de la lane).
    const double timelineStart = model.xToSeconds (visible.getX() - lane.getX());
    const double timelineEnd = model.xToSeconds (visible.getRight() - lane.getX());

    // Passe en temps source pour ne lire que le morceau de fichier affiché.
    const double sourceStart = offsetSeconds + (timelineStart - clipStart);
    const double sourceEnd = offsetSeconds + (timelineEnd - clipStart);

    if (sourceEnd <= sourceStart)
        return;

    const auto range = te::TimeRange (te::TimePosition::fromSeconds (sourceStart),
                                      te::TimeDuration::fromSeconds (sourceEnd - sourceStart));

    g.setColour (palette::blue.withAlpha (0.78f));
    thumbnail->drawChannels (g, visible.toNearestInt(), range, 0.9f);
}

//==============================================================================
te::Clip* TrackRow::clipAt (const juce::MouseEvent& e) const
{
    const auto lane = getLaneBounds();

    if (! lane.contains (e.getPosition()))
        return nullptr;

    const double seconds = model.xToSeconds ((float) (e.x - lane.getX()));
    return host.getClipAt (track, seconds);
}

void TrackRow::showClipMenu (te::Clip& clip, const juce::MouseEvent& e)
{
    juce::PopupMenu menu;
    menu.addItem (1, tr ("Couper à la tête de lecture"));
    menu.addItem (2, tr ("Dupliquer"));
    menu.addItem (3, tr ("Supprimer"));

    if (dynamic_cast<te::MidiClip*> (&clip) != nullptr)
        menu.addItem (4, tr ("Ouvrir dans le piano roll"));

    if (auto choice = menu.showAt (this, e.x, e.y))
    {
        switch (choice)
        {
            case 1: if (callbacks.splitClip)    callbacks.splitClip (clip);    break;
            case 2: if (callbacks.duplicateClip) callbacks.duplicateClip (clip); break;
            case 3: if (callbacks.deleteClip)   callbacks.deleteClip (clip);   break;
            case 4:
                if (auto* midi = dynamic_cast<te::MidiClip*> (&clip))
                    if (callbacks.openPianoRoll)
                        callbacks.openPianoRoll (*midi);
                break;
            default: break;
        }
    }
}

double TrackRow::snap (double seconds) const
{
    return model.snapBeats > 0.0 ? host.snapSeconds (seconds, model.snapBeats)
                                 : juce::jmax (0.0, seconds);
}

void TrackRow::mouseDown (const juce::MouseEvent& e)
{
    const auto lane = getLaneBounds();

    if (! lane.contains (e.getPosition()))
    {
        if (e.mods.isPopupMenu())
            showTrackMenu (e);
        return;
    }

    auto* clip = clipAt (e);
    selectClip (clip);

    if (callbacks.selected)
        callbacks.selected (track, clip);

    if (clip != nullptr)
    {
        if (e.mods.isPopupMenu())
        {
            showClipMenu (*clip, e);
            return;
        }

        // Glisser le clip, ou redimensionner en saisissant son bord droit.
        dragClip = clip;
        dragStartPoint = e.getPosition();
        dragOriginalPosition = clip->getPosition();
        dragMoved = false;

        const float x1 = (float) lane.getX() + model.secondsToX (clip->getPosition().time.getStart().inSeconds());
        const float x2 = (float) lane.getX() + model.secondsToX (clip->getPosition().time.getEnd().inSeconds());
        dragResizing = std::abs ((float) e.x - x2) <= 7.0f && (float) e.x >= x1 + 8.0f;
        return;
    }

    dragClip = nullptr;
    dragResizing = false;

    if (e.mods.isPopupMenu())
    {
        showTrackMenu (e);
        return;
    }

    if (callbacks.seek)
        callbacks.seek (snap (juce::jmax (0.0, model.xToSeconds ((float) (e.x - lane.getX())))));
}

void TrackRow::mouseDrag (const juce::MouseEvent& e)
{
    if (dragClip == nullptr || dragClip->getParent() != &track)
        return;

    const auto lane = getLaneBounds();

    if (! dragMoved && e.getDistanceFromDragStart() < 3)
        return;

    dragMoved = true;

    const double startSeconds = dragOriginalPosition.time.getStart().inSeconds();
    const double length = dragOriginalPosition.time.getLength().inSeconds();
    const double rawDelta = (double) (e.x - dragStartPoint.x) / model.pixelsPerSecond;

    auto position = dragOriginalPosition;

    if (dragResizing)
    {
        const double end = juce::jmax (startSeconds + 0.1, snap (startSeconds + length + rawDelta));
        position.time = te::TimeRange (te::TimePosition::fromSeconds (startSeconds),
                                       te::TimeDuration::fromSeconds (end - startSeconds));
    }
    else
    {
        const double start = juce::jmax (0.0, snap (startSeconds + rawDelta));
        position.time = te::TimeRange (te::TimePosition::fromSeconds (start),
                                       te::TimeDuration::fromSeconds (length));
    }

    dragClip->setPosition (position);
    repaintLane();
    juce::ignoreUnused (lane);
}

void TrackRow::mouseUp (const juce::MouseEvent&)
{
    if (dragClip != nullptr && dragMoved)
    {
        host.saveProject();

        if (callbacks.projectChanged)
            callbacks.projectChanged();
    }

    dragClip = nullptr;
    dragResizing = false;
    dragMoved = false;
}

void TrackRow::showTrackMenu (const juce::MouseEvent& e)
{
    juce::PopupMenu menu;
    menu.addItem (1, tr ("Dupliquer la piste"));
    menu.addItem (2, tr ("Importer un fichier audio…"));
    menu.addItem (3, tr ("Nouveau clip MIDI ici"));
    menu.addSeparator();
    menu.addItem (4, tr ("Renommer"));
    menu.addItem (5, tr ("Changer la couleur…"));
    menu.addSeparator();
    menu.addItem (6, tr ("Supprimer la piste"));

    if (auto choice = menu.showAt (this, e.x, e.y))
    {
        switch (choice)
        {
            case 1: if (callbacks.duplicateTrack) callbacks.duplicateTrack (track); break;
            case 2: if (callbacks.importAudio) callbacks.importAudio (track); break;
            case 3:
                if (callbacks.addMidiClip)
                {
                    const auto lane = getLaneBounds();
                    const double seconds = juce::jmax (0.0, model.xToSeconds ((float) (e.x - lane.getX())));
                    callbacks.addMidiClip (track, snap (seconds));
                }
                break;
            case 4: nameLabel.showEditor(); break;
            case 5: showColourMenu(); break;
            case 6: if (callbacks.remove) callbacks.remove (track); break;
            default: break;
        }
    }
}

void TrackRow::showColourMenu()
{
    juce::PopupMenu menu;
    menu.addItem (1, tr ("Bleu Firefox"));
    menu.addItem (2, tr ("Vert"));
    menu.addItem (3, tr ("Orange"));
    menu.addItem (4, tr ("Rouge"));
    menu.addItem (5, tr ("Violet"));
    menu.addItem (6, tr ("Gris"));

    if (auto choice = menu.show())
    {
        const juce::Colour colours[] = { palette::blue, palette::green, palette::amber,
                                         palette::red, juce::Colour (0xff8e6bff), palette::textDim };

        if (choice >= 1 && choice <= 6)
        {
            host.setTrackColour (track, colours[choice - 1]);
            host.saveProject();
            repaint();
        }
    }
}

void TrackRow::mouseDoubleClick (const juce::MouseEvent& e)
{
    if (auto* midi = dynamic_cast<te::MidiClip*> (clipAt (e)))
    {
        if (callbacks.openPianoRoll)
            callbacks.openPianoRoll (*midi);

        return;
    }

    const auto lane = getLaneBounds();

    if (lane.contains (e.getPosition()) && host.isMidiTrack (track) && callbacks.addMidiClip)
    {
        const double seconds = juce::jmax (0.0, model.xToSeconds ((float) (e.x - lane.getX())));
        callbacks.addMidiClip (track, snap (seconds));
    }
}

void TrackRow::mouseWheelMove (const juce::MouseEvent& e, const juce::MouseWheelDetails& wheel)
{
    const auto lane = getLaneBounds();

    if (lane.contains (e.getPosition()) && model.handleWheel (e, wheel, (float) (e.x - lane.getX())))
        return;

    Component::mouseWheelMove (e, wheel);
}
} // namespace fdgpe
