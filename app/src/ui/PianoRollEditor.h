#pragma once

#include <functional>
#include <memory>
#include <vector>

#include <juce_gui_basics/juce_gui_basics.h>

#include "../engine/EngineHost.h"

namespace fdgpe
{
/** Piano roll : grille notes x temps pour éditer les clips MIDI.
    - clic glissé sur une case vide : crée une note
    - glisser une note : la déplace (hauteur + temps)
    - clic droit sur une note : la supprime
    - Suppr : supprime la note sélectionnée
    - quantification 1/4, 1/8, 1/16, 1/32 */
class PianoRollEditor final : public juce::Component
{
public:
    PianoRollEditor (EngineHost&, te::MidiClip&);

    void paint (juce::Graphics&) override;
    void resized() override;
    bool keyPressed (const juce::KeyPress&) override;

    static void show (EngineHost&, te::MidiClip&);

private:
    class Grid final : public juce::Component
    {
    public:
        Grid (PianoRollEditor&, EngineHost&, te::MidiClip&);

        void paint (juce::Graphics&) override;
        void mouseDown (const juce::MouseEvent&) override;
        void mouseDrag (const juce::MouseEvent&) override;
        void mouseUp (const juce::MouseEvent&) override;
        void mouseWheelMove (const juce::MouseEvent&, const juce::MouseWheelDetails&) override;

        void refresh();
        void deleteSelectedNote();
        void setPixelsPerBeat (double);
        double getPixelsPerBeat() const noexcept { return pixelsPerBeat; }

    private:
        int pitchAt (int y) const;
        double beatAt (int x) const;
        int noteAt (int x, int y) const;
        void drawKeyboard (juce::Graphics&);

        static constexpr int keyboardWidth = 58;
        static constexpr int rowHeight = 15;
        static constexpr int lowestNote = 36;
        static constexpr int highestNote = 96;

        PianoRollEditor& owner;
        EngineHost& host;
        te::MidiClip& clip;
        std::vector<NoteInfo> notes;
        double pixelsPerBeat = 44.0;
        int selectedNote = -1;

        enum class DragMode { none, create, move } dragMode = DragMode::none;
        NoteInfo draft;
        int draftIndex = -1;
        int previewNote = -1;
        juce::Point<int> dragOrigin;
    };

    void quantize();
    void refreshAll();

    EngineHost& host;
    te::MidiClip& clip;
    Grid grid;
    juce::Viewport viewport;
    juce::Label titleLabel, hintLabel, statusLabel;
    juce::ComboBox quantizeBox;
    juce::TextButton quantizeButton, deleteButton, zoomInButton, zoomOutButton, closeButton;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PianoRollEditor)
};
} // namespace fdgpe
