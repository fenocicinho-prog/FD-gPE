#pragma once

#include <functional>
#include <memory>
#include <unordered_map>
#include <vector>

#include <juce_gui_basics/juce_gui_basics.h>

#include "../engine/EngineHost.h"
#include "TimelineModel.h"

namespace fdgpe
{
/** Petit bandeau de plugins insérés sur une piste. Chaque plugin est une tuile :
    clic = éditeur, clic sur le point = activer/désactiver, clic sur X = retirer,
    la dernière tuile "+" ouvre le navigateur. */
class PluginStrip final : public juce::Component
{
public:
    struct Callbacks
    {
        std::function<void()> add;
        std::function<void (te::Plugin&)> editor;
        std::function<void()> changed;
    };

    PluginStrip (EngineHost&, te::AudioTrack&, Callbacks);

    void paint (juce::Graphics&) override;
    void resized() override;
    void mouseMove (const juce::MouseEvent&) override;
    void mouseExit (const juce::MouseEvent&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDoubleClick (const juce::MouseEvent&) override;

    void refresh();

private:
    juce::Rectangle<int> tileBounds (int index) const;
    int tileCount() const;

    EngineHost& host;
    te::AudioTrack& track;
    Callbacks callbacks;
    std::vector<juce::String> names;
    std::vector<bool> enabled;
    int hovered = -1;
};

/** Une piste : en-tête (nom, ARM, MUTE, SOLO, volume, pan, vu-mètre, plugins)
    et ligne de temps avec les clips. */
class TrackRow final : public juce::Component
{
public:
    static constexpr int headerWidth = 320;
    static constexpr int rowHeight = 148;

    struct Callbacks
    {
        std::function<void (double)> seek;
        std::function<void (te::AudioTrack&)> remove;
        std::function<void()> changed;
        std::function<void (te::AudioTrack&)> addPlugin;
        std::function<void (te::Plugin&)> pluginEditor;
        std::function<void (te::MidiClip&)> openPianoRoll;
        std::function<void (te::AudioTrack&)> openKeyboard;
        std::function<void (te::Clip&)> splitClip;
        std::function<void (te::Clip&)> duplicateClip;
        std::function<void (te::Clip&)> deleteClip;
        std::function<void (te::AudioTrack&, te::Clip*)> selected;
        std::function<void (te::AudioTrack&)> duplicateTrack;
        std::function<void (te::AudioTrack&)> importAudio;
        std::function<void (te::AudioTrack&, double)> addMidiClip;
        std::function<void()> projectChanged;
    };

    TrackRow (EngineHost&, te::AudioTrack&, TimelineModel&, Callbacks);
    ~TrackRow() override;

    void paint (juce::Graphics&) override;
    void resized() override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseUp (const juce::MouseEvent&) override;
    void mouseDoubleClick (const juce::MouseEvent&) override;
    void mouseWheelMove (const juce::MouseEvent&, const juce::MouseWheelDetails&) override;

    void refreshControls();
    void updateMeter();
    void repaintLane();
    void selectClip (te::Clip*);
    te::Clip* getSelectedClip() const noexcept  { return selectedClip; }

    te::AudioTrack& getTrack() noexcept { return track; }

private:
    void paintLane (juce::Graphics&, juce::Rectangle<int> lane);
    void paintMeter (juce::Graphics&);
    juce::Rectangle<int> getLaneBounds() const;
    te::Clip* clipAt (const juce::MouseEvent&) const;
    void showClipMenu (te::Clip&, const juce::MouseEvent&);
    void showTrackMenu (const juce::MouseEvent&);
    void showColourMenu();
    void drawWaveform (juce::Graphics&, te::Clip&, juce::Rectangle<float>);
    double snap (double seconds) const;

    EngineHost& host;
    te::AudioTrack& track;
    TimelineModel& model;
    Callbacks callbacks;

    juce::Label kindLabel, nameLabel, volLabel, panLabel;
    juce::TextButton armButton { "ARM" }, muteButton { "MUTE" }, soloButton { "SOLO" }, deleteButton { "X" };
    juce::TextButton keyboardButton { "CLAVIER" };
    juce::Slider volumeSlider, panSlider;
    PluginStrip pluginStrip;
    juce::Rectangle<int> meterBounds;
    te::Clip* selectedClip = nullptr;

    te::LevelMeasurer::Client meterClient;
    float meterLevel = 0.0f;

    // Glisser/déplacer et redimensionner les clips.
    te::Clip* dragClip = nullptr;
    bool dragResizing = false;
    bool dragMoved = false;
    juce::Point<int> dragStartPoint;
    te::ClipPosition dragOriginalPosition;

    // Formes d'onde des clips audio, chargées à la demande.
    std::unordered_map<te::Clip*, std::unique_ptr<te::SmartThumbnail>> thumbnails;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (TrackRow)
};
} // namespace fdgpe
