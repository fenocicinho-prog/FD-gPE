#pragma once

#include <juce_audio_utils/juce_audio_utils.h>

#include "../engine/EngineHost.h"
#include "Palette.h"

namespace fdgpe
{
/** Petit clavier jouable, attaché à une piste MIDI. Les notes partent
    directement vers le device MIDI de cette piste (thread UI uniquement). */
class MidiKeyboardPanel final : public juce::Component,
                                private juce::MidiKeyboardState::Listener
{
public:
    MidiKeyboardPanel (EngineHost& h, te::AudioTrack& t)
        : host (h),
          track (t),
          keyboard (state, juce::MidiKeyboardComponent::verticalKeyboardFacingLeft)
    {
        state.addListener (this);
        keyboard.setAvailableRange (36, 96);
        keyboard.setLowestVisibleKey (48);
        keyboard.setKeyWidth (22.0f);
        keyboard.setScrollButtonsVisible (true);

        keyboard.setColour (juce::MidiKeyboardComponent::keyDownOverlayColourId, palette::blue.withAlpha (0.65f));
        keyboard.setColour (juce::MidiKeyboardComponent::mouseOverKeyOverlayColourId, palette::blue.withAlpha (0.25f));
        keyboard.setColour (juce::MidiKeyboardComponent::shadowColourId, palette::outline);
        keyboard.setColour (juce::MidiKeyboardComponent::upDownButtonBackgroundColourId, palette::panelHi);
        keyboard.setColour (juce::MidiKeyboardComponent::upDownButtonArrowColourId, palette::blue);

        addAndMakeVisible (keyboard);
        setSize (150, 420);
    }

    ~MidiKeyboardPanel() override
    {
        // Coupe les notes tenues avant de débrancher le listener.
        state.allNotesOff (1);
        state.removeListener (this);
    }

    void resized() override
    {
        keyboard.setBounds (getLocalBounds());
    }

private:
    void handleNoteOn (juce::MidiKeyboardState*, int, int midiNoteNumber, float velocity) override
    {
        host.sendNoteOn (track, midiNoteNumber, velocity);
    }

    void handleNoteOff (juce::MidiKeyboardState*, int, int midiNoteNumber, float) override
    {
        host.sendNoteOff (track, midiNoteNumber);
    }

    EngineHost& host;
    te::AudioTrack& track;
    juce::MidiKeyboardState state;
    juce::MidiKeyboardComponent keyboard;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MidiKeyboardPanel)
};
} // namespace fdgpe
