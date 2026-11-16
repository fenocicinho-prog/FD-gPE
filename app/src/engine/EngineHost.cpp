#include "EngineHost.h"

namespace fdgpe
{
EngineHost::EngineHost()
{
    // Projet vide, non lié à un fichier pour l'instant (sauvegarde : jalon M1).
    edit = te::createEmptyEdit (engine, {});

    // Ouvre le contexte audio pour que le périphérique de sortie démarre.
    edit->getTransport().ensureContextAllocated();
}

EngineHost::~EngineHost()
{
    // L'Edit doit être détruit avant l'Engine (ordre inverse de déclaration
    // garanti, mais on le rend explicite).
    edit.reset();
}

void EngineHost::togglePlay()
{
    auto& transport = edit->getTransport();

    if (transport.isPlaying())
        transport.stop (false, false);
    else
        transport.play (false);
}

bool EngineHost::isPlaying() const
{
    return edit->getTransport().isPlaying();
}

double EngineHost::getPositionSeconds() const
{
    return edit->getTransport().getPosition().inSeconds();
}

EngineHost::AudioStatus EngineHost::getAudioStatus()
{
    auto& deviceManager = engine.getDeviceManager();

    AudioStatus status;

    if (auto* device = deviceManager.deviceManager.getCurrentAudioDevice())
        status.deviceName = device->getName();

    status.sampleRate = deviceManager.getSampleRate();
    status.blockSize  = deviceManager.getBlockSize();
    status.cpuUsage   = deviceManager.getCpuUsage();
    status.midiInputs = deviceManager.getNumMidiInDevices();

    return status;
}

juce::String EngineHost::getEngineVersion()
{
    return te::Engine::getVersion();
}
} // namespace fdgpe
