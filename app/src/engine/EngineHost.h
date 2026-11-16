#pragma once

#include <memory>

#include <juce_core/juce_core.h>
#include <tracktion_engine/tracktion_engine.h>

namespace te = tracktion;

namespace fdgpe
{
/** Enveloppe autour de Tracktion Engine : possède le moteur et le projet
    ("Edit") courant. Tout le reste du DAW passe par cette classe pour parler
    au moteur, afin de garder Tracktion isolé à un seul endroit. */
class EngineHost
{
public:
    EngineHost();
    ~EngineHost();

    EngineHost (const EngineHost&) = delete;
    EngineHost& operator= (const EngineHost&) = delete;

    te::Engine& getEngine() noexcept  { return engine; }
    te::Edit&   getEdit() noexcept    { return *edit; }

    void togglePlay();
    bool isPlaying() const;
    double getPositionSeconds() const;

    struct AudioStatus
    {
        juce::String deviceName;
        double sampleRate = 0.0;
        int blockSize = 0;
        float cpuUsage = 0.0f;
        int midiInputs = 0;
    };

    AudioStatus getAudioStatus();

    static juce::String getEngineVersion();

private:
    te::Engine engine { "FD gPE" };
    std::unique_ptr<te::Edit> edit;
};
} // namespace fdgpe
