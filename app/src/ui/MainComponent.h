#pragma once

#include <memory>
#include <unordered_map>
#include <vector>

#include <juce_audio_utils/juce_audio_utils.h>
#include <juce_gui_basics/juce_gui_basics.h>

#include "../engine/EngineHost.h"
#include "TimelineModel.h"
#include "MidiKeyboardPanel.h"
#include "PianoRollEditor.h"
#include "TimelineRuler.h"
#include "TrackRow.h"
#include "TransportBar.h"

namespace fdgpe
{
/** Fenêtre principale : transport, pistes audio et MIDI, règle, clavier à
    l'écran et accès aux outils (plugins, piano roll, tempo, export). */
class MainComponent final : public juce::Component,
                            private juce::Timer
{
public:
    MainComponent();
    ~MainComponent() override;

    void paint (juce::Graphics&) override;
    void resized() override;
    bool keyPressed (const juce::KeyPress&) override;
    bool keyStateChanged (bool isKeyDown) override;

private:
    void timerCallback() override;

    void rebuildRows();
    void layoutRows();
    void refreshRows();
    void viewChanged();
    void startRecording();
    void createAudioTrack();
    void createMidiTrack();
    void removeTrack (te::AudioTrack&);
    void showAudioSettings();
    void showMixer();
    void cycleSnap();
    void showProjectMenu();
    void newProject();
    void openProject();
    void saveProjectAs();
    void importAudio (te::AudioTrack* track = nullptr);
    void duplicateTrack (te::AudioTrack* track = nullptr);
    void seek (double seconds);
    void openPianoRoll();
    void openPianoRoll (te::MidiClip&);
    void openKeyboard (te::AudioTrack&);
    void exportProject();
    void exportProjectAdvanced();
    void addMarker();
    void showPluginEditor (te::Plugin&);
    void handleNoteKey (const juce::KeyPress&, bool down);
    void releaseHeldKeys();
    bool trackStillExists (te::AudioTrack*);
    te::AudioTrack* getFocusedTrack();
    te::AudioTrack* getFocusedMidiTrack();
    te::MidiClip* getFocusedMidiClip();

    EngineHost host;
    TimelineModel model;

    TransportBar transport;
    TimelineRuler ruler;
    juce::Viewport viewport;
    juce::Component rowsContainer;
    std::vector<std::unique_ptr<TrackRow>> rows;
    PlayheadOverlay overlay;

    juce::TextButton addTrackButton { "+" }, projectButton;

    std::unique_ptr<juce::DialogWindow> audioSettingsWindow;
    std::unique_ptr<juce::DialogWindow> keyboardWindow;
    std::unique_ptr<PianoRollEditor> inlinePianoRoll;
    std::vector<int> activeNoteKeys;
    std::unordered_map<int, te::AudioTrack*> activeNoteTracks;   // code touche → piste qui a reçu la note
    int tick = 0;
    bool followPlayhead = true;   // le suivi automatique s'arrête dès que l'utilisateur défile
    bool wasPlaying = false;
    bool wasRecording = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainComponent)
};
} // namespace fdgpe
