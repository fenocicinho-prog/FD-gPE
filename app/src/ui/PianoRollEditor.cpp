#include "PianoRollEditor.h"

#include <cstddef>

#include "FdLookAndFeel.h"
#include "Palette.h"
#include "Text.h"

namespace fdgpe
{
//==============================================================================
PianoRollEditor::Grid::Grid (PianoRollEditor& o, EngineHost& h, te::MidiClip& c)
    : owner (o), host (h), clip (c)
{
    setMouseCursor (juce::MouseCursor::PointingHandCursor);
}

void PianoRollEditor::Grid::refresh()
{
    notes = host.getNotes (clip);
    const double lengthBeats = juce::jmax (16.0, clip.getPosition().time.getLength().inSeconds()
                                                  * host.getTempoBpm() / 60.0 + 4.0);
    setSize (keyboardWidth + (int) (lengthBeats * pixelsPerBeat) + 40,
             (highestNote - lowestNote + 1) * rowHeight);
    repaint();
    owner.statusLabel.setText (juce::String (notes.size()) + tr (" notes dans ce clip."),
                               juce::dontSendNotification);
}

void PianoRollEditor::Grid::deleteSelectedNote()
{
    if (selectedNote >= 0)
    {
        host.removeNote (clip, selectedNote);
        selectedNote = -1;
        refresh();
    }
}

void PianoRollEditor::Grid::setPixelsPerBeat (double value)
{
    pixelsPerBeat = juce::jlimit (14.0, 140.0, value);
    refresh();
}

int PianoRollEditor::Grid::pitchAt (int y) const
{
    const int row = juce::jlimit (0, highestNote - lowestNote, y / rowHeight);
    return highestNote - row;
}

double PianoRollEditor::Grid::beatAt (int x) const
{
    return juce::jmax (0.0, (double) (x - keyboardWidth) / pixelsPerBeat);
}

int PianoRollEditor::Grid::noteAt (int x, int y) const
{
    const int pitch = pitchAt (y);
    const double beat = beatAt (x);

    for (int i = 0; i < (int) notes.size(); ++i)
    {
        const auto& n = notes[(std::size_t) i];

        if (n.pitch == pitch && beat >= n.startBeat - 0.02 && beat <= n.startBeat + n.lengthBeats + 0.02)
            return i;
    }

    return -1;
}

void PianoRollEditor::Grid::paint (juce::Graphics& g)
{
    g.fillAll (palette::panelHi);

    const int rows = highestNote - lowestNote + 1;

    // lignes de hauteur (une par demi-ton)
    for (int row = 0; row < rows; ++row)
    {
        const int pitch = highestNote - row;
        const bool blackKey = juce::MidiMessage::isMidiNoteBlack (pitch);
        const int y = row * rowHeight;

        if (blackKey)
        {
            g.setColour (palette::laneAlt);
            g.fillRect (keyboardWidth, y, getWidth() - keyboardWidth, rowHeight);
        }

        g.setColour (palette::line.withAlpha (0.35f));
        g.drawHorizontalLine (y, (float) keyboardWidth, (float) getWidth());

        if (pitch % 12 == 0)
        {
            g.setColour (palette::blue.withAlpha (0.22f));
            g.fillRect (keyboardWidth, y, getWidth() - keyboardWidth, rowHeight);
        }
    }

    // lignes de temps
    const int totalBeats = (getWidth() - keyboardWidth) / (int) pixelsPerBeat + 1;

    for (int beat = 0; beat <= totalBeats; ++beat)
    {
        const int x = keyboardWidth + (int) (beat * pixelsPerBeat);

        g.setColour (beat % 4 == 0 ? palette::blue.withAlpha (0.25f) : palette::line.withAlpha (0.5f));
        g.drawVerticalLine (x, 0.0f, (float) getHeight());
    }

    // notes
    for (int i = 0; i < (int) notes.size(); ++i)
    {
        const auto& n = notes[(std::size_t) i];
        const auto r = juce::Rectangle<float> (
            keyboardWidth + (float) (n.startBeat * pixelsPerBeat) + 1.0f,
            (highestNote - n.pitch) * rowHeight + 1.0f,
            juce::jmax (5.0f, (float) (n.lengthBeats * pixelsPerBeat) - 2.0f),
            rowHeight - 2.0f);

        const bool selected = (i == selectedNote);
        const float velocity = juce::jlimit (0.25f, 1.0f, (float) n.velocity / 127.0f);

        g.setColour (palette::blue.withAlpha (velocity));
        g.fillRoundedRectangle (r, 3.0f);
        g.setColour (selected ? palette::text : palette::blueBright);
        g.drawRoundedRectangle (r.reduced (0.5f), 3.0f, selected ? 2.0f : 1.0f);
        g.setColour (juce::Colours::white.withAlpha (0.65f));
        g.fillRect (r.getX() + 2.0f, r.getY() + 1.0f, juce::jmax (2.0f, r.getWidth() * velocity / 6.0f), 2.0f);
    }

    // aperçu pendant le glisser
    if (dragMode != DragMode::none)
    {
        const auto r = juce::Rectangle<float> (
            keyboardWidth + (float) (draft.startBeat * pixelsPerBeat) + 1.0f,
            (highestNote - draft.pitch) * rowHeight + 1.0f,
            juce::jmax (5.0f, (float) (draft.lengthBeats * pixelsPerBeat) - 2.0f),
            rowHeight - 2.0f);

        g.setColour (palette::blueBright.withAlpha (0.55f));
        g.fillRoundedRectangle (r, 3.0f);
        g.setColour (palette::blue);
        g.drawRoundedRectangle (r, 3.0f, 1.5f);
    }

    drawKeyboard (g);
}

void PianoRollEditor::Grid::drawKeyboard (juce::Graphics& g)
{
    g.setColour (palette::panel);
    g.fillRect (0, 0, keyboardWidth, getHeight());

    for (int row = 0; row < (highestNote - lowestNote + 1); ++row)
    {
        const int pitch = highestNote - row;
        const int y = row * rowHeight;
        const bool blackKey = juce::MidiMessage::isMidiNoteBlack (pitch);

        g.setColour (blackKey ? palette::outline : palette::panelHi);
        g.fillRect (0, y, keyboardWidth - 8, rowHeight);

        g.setColour (palette::line);
        g.drawHorizontalLine (y, 0.0f, (float) keyboardWidth);

        if (pitch % 12 == 0)
        {
            g.setColour (palette::blue);
            g.setFont (juce::Font (juce::FontOptions (9.0f, juce::Font::bold)));
            g.drawText ("C" + juce::String (pitch / 12 - 1), 2, y + 1, keyboardWidth - 10, rowHeight - 2,
                        juce::Justification::centredLeft);
        }
    }

    g.setColour (palette::blue);
    g.drawVerticalLine (keyboardWidth - 1, 0.0f, (float) getHeight());
}

void PianoRollEditor::Grid::mouseDown (const juce::MouseEvent& e)
{
    if (e.mods.isPopupMenu())
    {
        const int index = noteAt (e.x, e.y);

        if (index >= 0)
        {
            host.removeNote (clip, index);
            selectedNote = -1;
            refresh();
        }

        return;
    }

    if (e.x < keyboardWidth)
    {
        previewNote = pitchAt (e.y);
        host.sendNoteOn (previewNote, 0.8f);
        return;
    }

    const double snapped = std::round (beatAt (e.x) * 4.0) / 4.0;
    const int pitch = pitchAt (e.y);
    const int existing = noteAt (e.x, e.y);

    if (existing >= 0)
    {
        selectedNote = existing;
        draft = notes[(size_t) existing];
        draftIndex = existing;
        dragMode = DragMode::move;
        dragOrigin = e.getPosition();
    }
    else
    {
        draft = { snapped, 1.0, pitch, 100 };
        draftIndex = -1;
        dragMode = DragMode::create;
        dragOrigin = e.getPosition();
        selectedNote = -1;
    }

    repaint();
}

void PianoRollEditor::Grid::mouseDrag (const juce::MouseEvent& e)
{
    if (dragMode == DragMode::create)
    {
        const double end = std::round (beatAt (e.x) * 4.0) / 4.0;
        draft.lengthBeats = juce::jmax (0.25, end - draft.startBeat);
    }
    else if (dragMode == DragMode::move)
    {
        const double deltaBeats = std::round ((double) (e.x - dragOrigin.x) / pixelsPerBeat * 4.0) / 4.0;
        const int deltaPitch = (dragOrigin.y - e.y) / rowHeight;
        draft.startBeat = juce::jmax (0.0, draft.startBeat + deltaBeats);
        draft.pitch = juce::jlimit (lowestNote, highestNote, draft.pitch + deltaPitch);
        dragOrigin = e.getPosition();
    }

    repaint();
}

void PianoRollEditor::Grid::mouseUp (const juce::MouseEvent&)
{
    if (previewNote >= 0)
    {
        host.sendNoteOff (previewNote);
        previewNote = -1;
    }

    if (dragMode == DragMode::create)
        host.addNote (clip, draft);
    else if (dragMode == DragMode::move && draftIndex >= 0)
        host.updateNote (clip, draftIndex, draft);

    dragMode = DragMode::none;
    refresh();
}

void PianoRollEditor::Grid::mouseWheelMove (const juce::MouseEvent& e, const juce::MouseWheelDetails& wheel)
{
    if (e.mods.isCtrlDown())
    {
        setPixelsPerBeat (pixelsPerBeat * (1.0 + (double) wheel.deltaY * 0.6));
        return;
    }

    Component::mouseWheelMove (e, wheel);
}

//==============================================================================
PianoRollEditor::PianoRollEditor (EngineHost& h, te::MidiClip& c)
    : host (h), clip (c), grid (*this, h, c)
{
    titleLabel.setText (tr ("Piano roll") + " — " + clip.getName(), juce::dontSendNotification);
    titleLabel.setFont (juce::Font (juce::FontOptions (18.0f, juce::Font::bold)));
    titleLabel.setColour (juce::Label::textColourId, palette::text);

    hintLabel.setText (tr ("Clic glissé : créer une note · Glisser : déplacer · Clic droit : supprimer · Ctrl+molette : zoom"),
                       juce::dontSendNotification);
    hintLabel.setFont (juce::Font (juce::FontOptions (11.0f, juce::Font::plain)));
    hintLabel.setColour (juce::Label::textColourId, palette::textDim);

    statusLabel.setFont (juce::Font (juce::FontOptions (11.0f, juce::Font::bold)));
    statusLabel.setColour (juce::Label::textColourId, palette::blue);

    quantizeBox.addItem (tr ("Pas de quantification"), 1);
    quantizeBox.addItem ("1/4", 4);
    quantizeBox.addItem ("1/8", 8);
    quantizeBox.addItem ("1/16", 16);
    quantizeBox.addItem ("1/32", 32);
    quantizeBox.setSelectedId (16, juce::dontSendNotification);

    quantizeButton.setButtonText (tr ("QUANTIFIER"));
    deleteButton.setButtonText (tr ("TOUT EFFACER"));
    zoomInButton.setButtonText ("+");
    zoomOutButton.setButtonText ("-");
    closeButton.setButtonText (tr ("FERMER"));

    quantizeButton.onClick = [this] { quantize(); };
    deleteButton.onClick = [this]
    {
        clip.getSequence().clear (&host.getEdit().getUndoManager());
        grid.refresh();
    };
    zoomInButton.onClick = [this] { grid.setPixelsPerBeat (grid.getPixelsPerBeat() * 1.25); };
    zoomOutButton.onClick = [this] { grid.setPixelsPerBeat (grid.getPixelsPerBeat() * 0.8); };
    closeButton.onClick = [this]
    {
        host.saveProject();

        if (onClose)
            onClose();
        else if (auto* window = findParentComponentOfClass<juce::DialogWindow>())
            window->closeButtonPressed();
    };

    viewport.setViewedComponent (&grid, false);
    viewport.setScrollBarsShown (true, true);
    viewport.setScrollBarThickness (10);

    for (juce::Component* component : { (juce::Component*) &titleLabel, (juce::Component*) &hintLabel,
                                        (juce::Component*) &statusLabel, (juce::Component*) &quantizeBox,
                                        (juce::Component*) &quantizeButton, (juce::Component*) &deleteButton,
                                        (juce::Component*) &zoomInButton, (juce::Component*) &zoomOutButton,
                                        (juce::Component*) &closeButton, (juce::Component*) &viewport })
        addAndMakeVisible (component);

    setWantsKeyboardFocus (true);
    grid.refresh();
    setSize (900, 640);
}

void PianoRollEditor::paint (juce::Graphics& g)
{
    g.fillAll (palette::panel);
    g.setColour (palette::blue);
    g.drawHorizontalLine (0, 0.0f, (float) getWidth());
}

void PianoRollEditor::resized()
{
    auto area = getLocalBounds().reduced (16, 12);

    titleLabel.setBounds (area.removeFromTop (32));
    hintLabel.setBounds (area.removeFromTop (22));
    auto tools = area.removeFromTop (36);

    quantizeBox.setBounds (tools.removeFromLeft (180).reduced (0, 3));
    tools.removeFromLeft (8);
    quantizeButton.setBounds (tools.removeFromLeft (110).reduced (0, 3));
    deleteButton.setBounds (tools.removeFromLeft (110).reduced (0, 3));
    tools.removeFromLeft (8);
    zoomOutButton.setBounds (tools.removeFromLeft (34).reduced (0, 3));
    zoomInButton.setBounds (tools.removeFromLeft (34).reduced (0, 3));
    closeButton.setBounds (tools.removeFromRight (90).reduced (0, 3));

    statusLabel.setBounds (area.removeFromBottom (24));
    viewport.setBounds (area);
}

bool PianoRollEditor::keyPressed (const juce::KeyPress& key)
{
    if (key == juce::KeyPress::deleteKey || key == juce::KeyPress::backspaceKey)
    {
        grid.deleteSelectedNote();
        return true;
    }

    return false;
}

void PianoRollEditor::quantize()
{
    const int division = quantizeBox.getSelectedId();

    if (division > 1)
    {
        host.quantize (clip, division);
        grid.refresh();
        statusLabel.setText (tr ("Quantifié sur 1/") + juce::String (division), juce::dontSendNotification);
    }
}

void PianoRollEditor::refreshAll()
{
    grid.refresh();
}

void PianoRollEditor::show (EngineHost& host, te::MidiClip& clip)
{
    juce::DialogWindow::LaunchOptions options;
    options.dialogTitle = tr ("Piano roll — ") + clip.getName();
    options.dialogBackgroundColour = palette::panel;
    options.escapeKeyTriggersCloseButton = true;
    options.useNativeTitleBar = true;
    options.resizable = true;

    options.content.setOwned (new PianoRollEditor (host, clip));

    auto* window = options.launchAsync();
    window->setSize (920, 680);
    window->centreWithSize (920, 680);
}
} // namespace fdgpe
