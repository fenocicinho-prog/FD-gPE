#pragma once

#include <functional>
#include <memory>
#include <vector>

#include <juce_gui_basics/juce_gui_basics.h>

#include "../engine/EngineHost.h"
#include "TrackRow.h"

namespace fdgpe
{
/** Mixeur ancré en bas de la fenêtre : une tranche par piste (nom, effets,
    panoramique, fader avec vu-mètre, MUTE et SOLO), façon console. */
class MixerDock final : public juce::Component
{
public:
    static constexpr int dockHeight = 300;

    struct Callbacks
    {
        std::function<void (te::AudioTrack&)> addPlugin;
        std::function<void (te::Plugin&)> pluginEditor;
        std::function<void()> changed;
    };

    MixerDock (EngineHost&, Callbacks);
    ~MixerDock() override;

    /** Reconstruit les tranches d'après les pistes actuelles. */
    void rebuild();

    /** Retire toutes les tranches (à faire AVANT de supprimer une piste : les
        vu-mètres sont branchés dessus). */
    void clear();

    void refresh();
    void updateMeters();

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    class Strip;
    class MasterStrip;

    EngineHost& host;
    Callbacks callbacks;
    juce::Viewport viewport;
    juce::Component content;
    std::vector<std::unique_ptr<Strip>> strips;
    std::unique_ptr<MasterStrip> masterStrip;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MixerDock)
};
} // namespace fdgpe
