#include "EngineHost.h"

#include "../ui/Palette.h"
#include "../ui/Text.h"

#include <algorithm>
#include <cmath>

using namespace tracktion::literals;

namespace fdgpe
{
namespace
{
    const juce::Identifier kindId       { "fdgpeKind" };
    const juce::Identifier drumId       { "fdgpeDrum" };
    const juce::Identifier auxReturnId  { "fdgpeAuxReturn" };
    const juce::Identifier colourId     { "fdgpeColour" };
    const char* const keyboardDeviceName = "Clavier FD gPE";

    // Catalogue des plugins intégrés Tracktion, affine pour la production musicale.
    template <class PluginClass>
    void addBuiltIn (std::vector<PluginInfo>& out, bool instrument, const juce::String& category)
    {
        auto desc = te::PluginManager::createBuiltInPluginDescription<PluginClass> (instrument);

        PluginInfo info;
        info.name         = desc.name;
        info.category     = category;
        info.manufacturer = desc.manufacturerName;
        info.type         = desc.fileOrIdentifier;
        info.identifier   = desc.fileOrIdentifier;
        info.instrument   = instrument;
        info.builtIn      = true;
        info.description  = desc;
        out.push_back (info);
    }

}

//==============================================================================
EngineHost::EngineHost()
{
    // Scan hors processus et liste noire persistante, avant tout chargement de projet.
    engine.getPluginManager().setUsesSeparateProcessForScanning (true);
    loadPluginBlacklist();

    projectFile = juce::File::getSpecialLocation (juce::File::userDocumentsDirectory)
                      .getChildFile ("FD-gPE")
                      .getChildFile ("Projet.tracktionedit");
    projectFile.getParentDirectory().createDirectory();

    if (projectFile.existsAsFile())
        edit = te::loadEditFromFile (engine, projectFile);

    const bool isNewProject = (edit == nullptr);

    if (isNewProject)
        edit = te::createEmptyEdit (engine, projectFile);

    setupInputDevices();
    prepareEdit();

    if (isNewProject)
    {
        if (auto* first = te::getAudioTracks (*edit).getFirst())
        {
            first->state.setProperty (kindId, "audio", nullptr);
            first->setName ("Audio 1");
        }
        else
        {
            addAudioTrack();
        }

        addMidiTrack();
        saveProject();
    }
    else
    {
        tagMissingTrackKinds();
    }

    edit->restartPlayback();
}

void EngineHost::prepareEdit()
{
    for (auto* track : te::getAudioTracks (*edit))
    {
        if (track->state.getProperty (kindId).toString().isEmpty())
        {
            // Ancien projet ou piste ajoutée à la main : on devine le type.
            bool midi = track->getName().containsIgnoreCase ("midi");

            for (auto* clip : track->getClips())
                if (dynamic_cast<te::MidiClip*> (clip) != nullptr)
                    midi = true;

            track->state.setProperty (kindId, midi ? "midi" : "audio", nullptr);
        }

        if (isMidiTrack (*track))
        {
            // Les devices MIDI propres aux pistes sont réservés au routage entre
            // pistes. Les activer crée des TrackMidiInputDeviceNode en doublon
            // (assertion NodePlayerUtilities + plantage) : on les désactive.
            if (auto* midiDevice = dynamic_cast<te::VirtualMidiInputDevice*> (&track->getMidiInputDevice()))
                midiDevice->setEnabled (false);

            bool hasSynth = false;

            for (auto* plugin : track->pluginList.getPlugins())
                if (plugin != nullptr && plugin->isSynth())
                    hasSynth = true;

            // Répare les projets sauvés sans instrument : une piste MIDI muette
            // est une piste sans synthé (ou avec un synthé désactivé).
            if (! hasSynth)
                if (auto synth = edit->getPluginCache().createNewPlugin (te::FourOscPlugin::xmlTypeName, {}))
                    track->pluginList.insertPlugin (synth, 0, nullptr);
        }
    }

    // Les devices de piste sont désactivés avant la première construction du
    // graphe : sinon Tracktion crée des TrackMidiInputDeviceNode en doublon.
    edit->getTransport().ensureContextAllocated();
}

void EngineHost::tagMissingTrackKinds()
{
    prepareEdit();
}

EngineHost::~EngineHost()
{
    if (edit != nullptr)
    {
        auto& transport = edit->getTransport();

        if (transport.isPlaying() || transport.isRecording())
            transport.stop (true, false);

        saveProject();
    }

    edit.reset();
}

//==============================================================================
void EngineHost::setupInputDevices()
{
    auto& dm = engine.getDeviceManager();

    for (int i = 0; i < dm.getNumWaveInDevices(); ++i)
    {
        if (auto wip = dm.getWaveInDevice (i))
        {
            if (i == 0)
                wip->setChannelConfiguration (te::ChannelConfiguration::mono());

            wip->setMonitorMode (te::InputDevice::MonitorMode::off);
            wip->setEnabled (i == 0);
        }
    }

    for (auto& midiIn : dm.getMidiInDevices())
    {
        midiIn->setMonitorMode (te::InputDevice::MonitorMode::automatic);
        midiIn->setEnabled (true);
    }

    bool keyboardExists = false;

    for (auto& d : dm.getMidiInDevices())
        if (d->getName() == keyboardDeviceName)
            keyboardExists = true;

    if (! keyboardExists)
        dm.createVirtualMidiDevice (keyboardDeviceName);

    // createVirtualMidiDevice() ne met à jour la liste qu'après un rescan
    // asynchrone (timer de 5 ms de DeviceManager). keyboardDevice() retrouvera
    // le device et le configurera au premier appel, une fois la liste à jour.
}

//==============================================================================
void EngineHost::saveProject()
{
    te::EditFileOperations (*edit).save (false, true, false);
}

bool EngineHost::saveProjectAs (const juce::File& file)
{
    if (file.getFileName().isEmpty())
        return false;

    projectFile = file;
    projectFile.getParentDirectory().createDirectory();
    te::EditFileOperations (*edit).saveAs (file);
    return true;
}

bool EngineHost::newProject()
{
    auto folder = juce::File::getSpecialLocation (juce::File::userDocumentsDirectory)
                      .getChildFile ("FD-gPE");
    folder.createDirectory();

    auto file = folder.getChildFile ("Nouveau projet.tracktionedit");

    for (int i = 2; file.existsAsFile() && i < 1000; ++i)
        file = folder.getChildFile ("Nouveau projet " + juce::String (i) + ".tracktionedit");

    if (edit != nullptr)
    {
        auto& transport = edit->getTransport();

        if (transport.isPlaying() || transport.isRecording())
            transport.stop (true, false);

        saveProject();
        edit.reset();
    }

    projectFile = file;
    edit = te::createEmptyEdit (engine, projectFile);

    if (edit == nullptr)
        return false;

    prepareEdit();
    addAudioTrack();
    addMidiTrack();
    edit->restartPlayback();
    saveProject();
    return true;
}

bool EngineHost::openProject (const juce::File& file)
{
    if (! file.existsAsFile())
        return false;

    auto loaded = te::loadEditFromFile (engine, file);

    if (loaded == nullptr)
        return false;

    if (edit != nullptr)
    {
        auto& transport = edit->getTransport();

        if (transport.isPlaying() || transport.isRecording())
            transport.stop (true, false);

        saveProject();
    }

    edit = std::move (loaded);
    projectFile = file;
    prepareEdit();
    edit->restartPlayback();
    return true;
}

double EngineHost::getProjectLengthSeconds() const
{
    return edit->getLength().inSeconds();
}

//==============================================================================
std::vector<te::AudioTrack*> EngineHost::getTracks()
{
    std::vector<te::AudioTrack*> result;

    for (auto* t : te::getAudioTracks (*edit))
        if (! isAuxReturnTrack (*t))
            result.push_back (t);

    return result;
}

bool EngineHost::isMidiTrack (te::AudioTrack& track) const
{
    return track.state.getProperty (kindId).toString() == "midi";
}

bool EngineHost::isDrumTrack (te::AudioTrack& track) const
{
    return (bool) track.state.getProperty (drumId, false);
}

bool EngineHost::isAuxReturnTrack (te::AudioTrack& track) const
{
    return track.state.hasProperty (auxReturnId);
}

std::vector<te::AudioTrack*> EngineHost::getAuxReturnTracks()
{
    std::vector<te::AudioTrack*> result;

    for (auto* t : te::getAudioTracks (*edit))
        if (isAuxReturnTrack (*t))
            result.push_back (t);

    return result;
}

int EngineHost::countTracksOfKind (bool midi)
{
    int n = 0;

    for (auto* t : te::getAudioTracks (*edit))
        if (isMidiTrack (*t) == midi)
            ++n;

    return n;
}

te::AudioTrack* EngineHost::addTrackOfKind (bool midi)
{
    edit->ensureNumberOfAudioTracks (te::getAudioTracks (*edit).size() + 1);

    auto* track = te::getAudioTracks (*edit).getLast();

    if (track == nullptr)
        return nullptr;

    track->state.setProperty (kindId, midi ? "midi" : "audio", nullptr);
    track->setName ((midi ? "MIDI " : "Audio ") + juce::String (countTracksOfKind (midi)));

    if (midi)
    {
        if (auto synth = edit->getPluginCache().createNewPlugin (te::FourOscPlugin::xmlTypeName, {}))
            track->pluginList.insertPlugin (synth, 0, nullptr);
    }

    return track;
}

te::AudioTrack* EngineHost::addAudioTrack()  { return addTrackOfKind (false); }
te::AudioTrack* EngineHost::addMidiTrack()   { return addTrackOfKind (true); }

te::AudioTrack* EngineHost::addDrumTrack()
{
    auto* track = addTrackOfKind (true);

    if (track == nullptr)
        return nullptr;

    track->state.setProperty (drumId, true, nullptr);

    int n = 0;
    for (auto* t : te::getAudioTracks (*edit))
        if (isDrumTrack (*t))
            ++n;

    track->setName (tr ("Rythme ") + juce::String (juce::jmax (1, n)));
    ensureDrumSampler (*track);
    return track;
}

te::AudioTrack* EngineHost::duplicateTrack (te::AudioTrack& source)
{
    const bool midi = isMidiTrack (source);
    auto* copy = addTrackOfKind (midi);

    if (copy == nullptr)
        return nullptr;

    copy->setName (source.getName() + juce::String::fromUTF8 (" (copie)"));
    copy->state.setProperty (colourId, source.state.getProperty (colourId), nullptr);

    // Copie des clips avec de nouveaux identifiants.
    for (auto* clip : source.getClips())
    {
        auto state = clip->state.createCopy();
        edit->createNewItemID().writeID (state, nullptr);

        if (auto* newClip = copy->insertClipWithState (state))
        {
            auto position = clip->getPosition();
            position.time = te::TimeRange (position.time.getStart() + position.time.getLength(),
                                           position.time.getLength());
            newClip->setPosition (position);
        }
    }

    // Copie des plugins utilisateur (on laisse de côté volume/pan et vu-mètre).
    for (auto* plugin : source.pluginList)
    {
        if (plugin == nullptr || isMixerPlugin (*plugin))
            continue;

        auto state = plugin->state.createCopy();
        edit->createNewItemID().writeID (state, nullptr);
        copy->pluginList.insertPlugin (state, copy->pluginList.size());
    }

    return copy;
}

void EngineHost::importAudioFile (te::AudioTrack& track, const juce::File& file, double startSeconds)
{
    if (! file.existsAsFile())
        return;

    // La durée réelle est lue par Tracktion.
    te::AudioFile audioFile (engine, file);
    const auto seconds = audioFile.getLength();

    auto* clip = te::insertWaveClip (track, file.getFileNameWithoutExtension(), file,
                                     te::ClipPosition { te::TimeRange (te::TimePosition::fromSeconds (juce::jmax (0.0, startSeconds)),
                                                                       te::TimeDuration::fromSeconds (juce::jmax (0.25, seconds))),
                                                        te::TimeDuration() },
                                     te::DeleteExistingClips::no).get();

    juce::ignoreUnused (clip);
    saveProject();
}

void EngineHost::deleteTrack (te::AudioTrack& track)
{
    edit->deleteTrack (&track);
}

void EngineHost::renameTrack (te::AudioTrack& track, const juce::String& name)
{
    if (name.isNotEmpty())
        track.setName (name);
}

void EngineHost::setTrackColour (te::AudioTrack& track, const juce::Colour& colour)
{
    track.state.setProperty (colourId, (juce::int64) colour.getARGB(), nullptr);
}

juce::Colour EngineHost::getTrackColour (const te::AudioTrack& track) const
{
    // Piste sans couleur choisie : on en propose une selon sa position, pour
    // que les clips se distinguent d'un coup d'œil (jaune clair, vert clair, bleu…).
    if (! track.state.hasProperty (colourId))
    {
        int index = 0;

        for (auto* t : te::getAudioTracks (*edit))
        {
            if (t == &track)
                break;

            ++index;
        }

        return palette::trackColour (index);
    }

    const auto argb = (juce::uint32) (juce::int64) track.state.getProperty (colourId, (juce::int64) palette::blue.getARGB());
    return juce::Colour (argb);
}

//==============================================================================
bool EngineHost::isTrackArmed (te::AudioTrack& track)
{
    for (auto* instance : edit->getAllInputDevices())
        if (te::isOnTargetTrack (*instance, track, 0) && instance->isRecordingEnabled (track.itemID))
            return true;

    return false;
}

bool EngineHost::anyTrackArmed()
{
    for (auto* t : te::getAudioTracks (*edit))
        if (isTrackArmed (*t))
            return true;

    return false;
}

void EngineHost::setTrackArmed (te::AudioTrack& track, bool armed)
{
    auto& undo = edit->getUndoManager();

    for (auto* other : te::getAudioTracks (*edit))
        for (auto* instance : edit->getAllInputDevices())
            if (te::isOnTargetTrack (*instance, *other, 0))
                instance->setRecordingEnabled (other->itemID, false);

    if (armed)
    {
        const bool midi = isMidiTrack (track);

        for (auto* instance : edit->getAllInputDevices())
        {
            auto& device = instance->getInputDevice();

            const bool matches = midi ? (dynamic_cast<te::VirtualMidiInputDevice*> (&device) != nullptr
                                         && ! device.isTrackDevice())
                                      : device.getDeviceType() == te::InputDevice::waveDevice;

            if (! matches)
                continue;

            [[ maybe_unused ]] auto result = instance->setTarget (track.itemID, true, &undo, 0);
            instance->setRecordingEnabled (track.itemID, true);

            if (! midi)
                break;
        }
    }

    auto& transport = edit->getTransport();

    if (! transport.isPlaying() && ! transport.isRecording())
        edit->restartPlayback();
}

//==============================================================================
void EngineHost::togglePlay()
{
    auto& transport = edit->getTransport();

    if (transport.isRecording())
    {
        // false = on garde les fichiers enregistrés (paramètre discardRecordings).
        transport.stop (false, false);
        saveProject();
    }
    else if (transport.isPlaying())
    {
        transport.stop (false, false);
    }
    else
    {
        transport.play (false);
    }
}

void EngineHost::toggleRecord()
{
    auto& transport = edit->getTransport();

    if (transport.isRecording())
    {
        // false = on garde les fichiers enregistrés (paramètre discardRecordings).
        transport.stop (false, false);
        saveProject();
    }
    else
    {
        recordStartSeconds = transport.getPosition().inSeconds();
        transport.record (false);
    }
}

void EngineHost::returnToStart()
{
    edit->getTransport().setPosition (te::TimePosition::fromSeconds (0.0));
}

bool EngineHost::isPlaying() const     { return edit->getTransport().isPlaying(); }
bool EngineHost::isRecording() const   { return edit->getTransport().isRecording(); }

bool EngineHost::isLooping() const     { return edit->getTransport().looping; }

void EngineHost::setLooping (bool shouldLoop)
{
    auto& transport = edit->getTransport();

    if (shouldLoop && transport.getLoopRange().getLength().inSeconds() < 0.1)
    {
        const auto start = transport.getPosition();
        const auto length = te::TimeDuration::fromSeconds (4.0 * 4.0 * 60.0 / getTempoBpm());
        transport.setLoopRange (te::TimeRange (start, length));
    }

    transport.looping = shouldLoop;
}

void EngineHost::setLoopRange (double startSeconds, double endSeconds)
{
    if (endSeconds <= startSeconds)
        return;

    edit->getTransport().setLoopRange (te::TimeRange (te::TimePosition::fromSeconds (startSeconds),
                                                      te::TimePosition::fromSeconds (endSeconds)));
}

std::pair<double, double> EngineHost::getLoopRange() const
{
    const auto range = edit->getTransport().getLoopRange();
    return { range.getStart().inSeconds(), range.getEnd().inSeconds() };
}

bool EngineHost::isMetronomeOn() const   { return edit->clickTrackEnabled.get(); }

void EngineHost::setMetronomeOn (bool on)
{
    edit->clickTrackEnabled = on;
    edit->clickTrackRecordingOnly = false;
}

double EngineHost::getTempoBpm() const
{
    return edit->tempoSequence.getBpmAt (edit->getTransport().getPosition());
}

void EngineHost::setTempoBpm (double bpm)
{
    const auto pos = edit->getTransport().getPosition();
    edit->tempoSequence.getTempoAt (pos).setBpm (juce::jlimit (20.0, 300.0, bpm));
}

void EngineHost::tapTempo()
{
    const auto now = juce::Time::getMillisecondCounterHiRes();
    tapTimes.erase (std::remove_if (tapTimes.begin(), tapTimes.end(),
                                    [now] (double t) { return now - t > 3000.0; }), tapTimes.end());
    tapTimes.push_back (now);

    if (tapTimes.size() < 2)
        return;

    double total = 0.0;

    for (size_t i = 1; i < tapTimes.size(); ++i)
        total += tapTimes[i] - tapTimes[i - 1];

    const double average = total / (double) (tapTimes.size() - 1);

    if (average > 0.0)
        setTempoBpm (60000.0 / average);

    if (tapTimes.size() > 6)
        tapTimes.erase (tapTimes.begin());
}

std::pair<int, int> EngineHost::getTimeSignature() const
{
    const auto& ts = edit->tempoSequence.getTimeSigAt (edit->getTransport().getPosition());
    return { (int) ts.numerator.get(), (int) ts.denominator.get() };
}

void EngineHost::setTimeSignature (int numerator, int denominator)
{
    auto& ts = edit->tempoSequence.getTimeSigAt (edit->getTransport().getPosition());
    ts.numerator = juce::jlimit (1, 32, numerator);
    ts.denominator = juce::jlimit (1, 32, denominator);
}

double EngineHost::getPositionSeconds() const
{
    return edit->getTransport().getPosition().inSeconds();
}

void EngineHost::setPositionSeconds (double seconds)
{
    edit->getTransport().setPosition (te::TimePosition::fromSeconds (juce::jmax (0.0, seconds)));
}

double EngineHost::getPositionBeats() const
{
    return edit->tempoSequence.toBeats (edit->getTransport().getPosition()).inBeats();
}

EngineHost::BarsBeats EngineHost::getBarsBeats() const
{
    const auto bb = edit->tempoSequence.toBarsAndBeats (edit->getTransport().getPosition());
    return { bb.bars + 1, bb.getWholeBeats() + 1, bb.beats.inBeats() - std::floor (bb.beats.inBeats()) };
}

bool EngineHost::canUndo() const    { return edit->getUndoManager().canUndo(); }
bool EngineHost::canRedo() const    { return edit->getUndoManager().canRedo(); }
void EngineHost::undo()             { edit->undo(); }
void EngineHost::redo()             { edit->redo(); }

//==============================================================================
te::MidiInputDevice* EngineHost::keyboardDevice()
{
    if (virtualKeyboard == nullptr)
    {
        for (auto& device : engine.getDeviceManager().getMidiInDevices())
        {
            if (device->getName() != keyboardDeviceName)
                continue;

            virtualKeyboard = device;
            break;
        }
    }

    if (virtualKeyboard != nullptr)
    {
        // « on » et non « automatic » : avec « automatic » Tracktion ne crée le
        // noeud d'entrée que si la piste est armée, donc le clavier reste muet
        // quand on joue sans enregistrer.
        if (virtualKeyboard->getMonitorMode() != te::InputDevice::MonitorMode::on)
            virtualKeyboard->setMonitorMode (te::InputDevice::MonitorMode::on);

        if (! virtualKeyboard->isEnabled())
            virtualKeyboard->setEnabled (true);
    }

    return virtualKeyboard.get();
}

void EngineHost::sendMidi (const juce::MidiMessage& message)
{
    if (auto* device = dynamic_cast<te::VirtualMidiInputDevice*> (keyboardDevice()))
        device->handleIncomingMidiMessage (message, te::MPESourceID());
}

void EngineHost::sendNoteOn (int midiNote, float velocity)
{
    sendMidi (juce::MidiMessage::noteOn (1, midiNote, velocity));
}

void EngineHost::sendNoteOff (int midiNote)
{
    sendMidi (juce::MidiMessage::noteOff (1, midiNote));
}

void EngineHost::sendMidiToTrack (te::AudioTrack& track, const juce::MidiMessage& message)
{
    auto* device = keyboardDevice();

    if (device == nullptr)
        return;

    // Le device clavier global est le seul dont les notes atteignent la chaîne
    // de plugins d'une piste (les devices propres aux pistes servent au routage
    // entre pistes). On l'affecte à la piste jouée pour créer le noeud d'entrée
    // live, même si la piste est vide et le transport arrêté.
    if (auto* context = edit->getCurrentPlaybackContext())
    {
        if (auto* instance = context->getInputFor (device))
        {
            if (! te::isOnTargetTrack (*instance, track, 0))
                [[ maybe_unused ]] auto result = instance->setTarget (track.itemID, true, nullptr, 0);
        }
    }

    if (auto* virtualDevice = dynamic_cast<te::VirtualMidiInputDevice*> (device))
        virtualDevice->handleIncomingMidiMessage (message, te::MPESourceID());
}

void EngineHost::sendNoteOn (te::AudioTrack& track, int midiNote, float velocity)
{
    sendMidiToTrack (track, juce::MidiMessage::noteOn (1, midiNote, velocity));
}

void EngineHost::sendNoteOff (te::AudioTrack& track, int midiNote)
{
    sendMidiToTrack (track, juce::MidiMessage::noteOff (1, midiNote));
}

void EngineHost::sendNoteOn (te::MidiClip& clip, int midiNote, float velocity)
{
    if (auto* track = dynamic_cast<te::AudioTrack*> (clip.getParent()))
        sendNoteOn (*track, midiNote, velocity);
}

void EngineHost::sendNoteOff (te::MidiClip& clip, int midiNote)
{
    if (auto* track = dynamic_cast<te::AudioTrack*> (clip.getParent()))
        sendNoteOff (*track, midiNote);
}

//==============================================================================
std::vector<TempoPoint> EngineHost::getTempoMap() const
{
    std::vector<TempoPoint> result;

    for (auto* tempo : edit->tempoSequence.getTempos())
    {
        TempoPoint point;
        point.beat = tempo->getStartBeat().inBeats();
        point.bpm  = tempo->getBpm();

        const auto& ts = edit->tempoSequence.getTimeSigAt (tempo->getStartBeat());
        point.numerator   = ts.numerator.get();
        point.denominator = ts.denominator.get();
        result.push_back (point);
    }

    return result;
}

std::vector<TimeSigPoint> EngineHost::getTimeSigMap() const
{
    std::vector<TimeSigPoint> result;

    for (auto* ts : edit->tempoSequence.getTimeSigs())
    {
        if (ts == nullptr)
            continue;

        TimeSigPoint point;
        point.beat        = ts->getStartBeat().inBeats();
        point.numerator   = ts->numerator.get();
        point.denominator = ts->denominator.get();
        result.push_back (point);
    }

    return result;
}

double EngineHost::secondsToBeat (double seconds) const
{
    return edit->tempoSequence.toBeats (te::TimePosition::fromSeconds (seconds)).inBeats();
}

double EngineHost::beatToSeconds (double beat) const
{
    return edit->tempoSequence.toTime (te::BeatPosition::fromBeats (beat)).inSeconds();
}

double EngineHost::snapSeconds (double seconds, double divisionBeats) const
{
    if (divisionBeats <= 0.0)
        return seconds;

    const double beat = secondsToBeat (seconds);
    const double snapped = std::round (beat / divisionBeats) * divisionBeats;
    return juce::jmax (0.0, beatToSeconds (snapped));
}

void EngineHost::setTempoAtBeat (double beat, double bpm)
{
    auto& sequence = edit->tempoSequence;

    for (auto* tempo : sequence.getTempos())
    {
        if (std::abs (tempo->getStartBeat().inBeats() - beat) < 0.001)
        {
            tempo->setBpm (juce::jlimit (20.0, 300.0, bpm));
            return;
        }
    }

    sequence.insertTempo (te::BeatPosition::fromBeats (beat), juce::jlimit (20.0, 300.0, bpm), 0.0f);
}

void EngineHost::removeTempoAt (int index)
{
    if (index > 0 && index < edit->tempoSequence.getNumTempos())
        edit->tempoSequence.removeTempo (index, false);
}

void EngineHost::setTimeSigAtBeat (double beat, int numerator, int denominator)
{
    auto& sequence = edit->tempoSequence;
    const auto beatPos = te::BeatPosition::fromBeats (beat);

    for (auto* ts : sequence.getTimeSigs())
    {
        if (std::abs (ts->getStartBeat().inBeats() - beat) < 0.001)
        {
            ts->numerator   = juce::jlimit (1, 32, numerator);
            ts->denominator = juce::jlimit (1, 32, denominator);
            return;
        }
    }

    if (auto added = sequence.insertTimeSig (beatPos))
    {
        added->numerator   = juce::jlimit (1, 32, numerator);
        added->denominator = juce::jlimit (1, 32, denominator);
    }
}

void EngineHost::removeTimeSigAt (int index)
{
    if (index > 0 && index < edit->tempoSequence.getNumTimeSigs())
        edit->tempoSequence.removeTimeSig (index);
}

//==============================================================================
std::vector<MarkerInfo> EngineHost::getMarkers() const
{
    std::vector<MarkerInfo> result;

    for (auto marker : edit->getMarkerManager().getMarkers())
    {
        if (marker == nullptr)
            continue;

        MarkerInfo info;
        info.id      = marker->getMarkerID();
        info.name    = marker->getName();
        info.seconds = marker->getPosition().time.getStart().inSeconds();
        result.push_back (info);
    }

    std::sort (result.begin(), result.end(), [] (const MarkerInfo& a, const MarkerInfo& b)
    {
        return a.seconds < b.seconds;
    });

    return result;
}

void EngineHost::addMarker (const juce::String& name, double seconds)
{
    auto marker = edit->getMarkerManager().createMarker (
        -1, te::TimePosition::fromSeconds (juce::jmax (0.0, seconds)), {}, nullptr);

    if (marker != nullptr)
        marker->setName (name.isEmpty() ? "Marqueur" : name);
}

void EngineHost::removeMarker (int id)
{
    if (auto* marker = edit->getMarkerManager().getMarkerByID (id))
        marker->removeFromParent();
}

void EngineHost::goToMarker (bool next)
{
    const auto markers = getMarkers();
    const double pos = getPositionSeconds();

    if (next)
    {
        for (const auto& m : markers)
            if (m.seconds > pos + 0.01)
            {
                setPositionSeconds (m.seconds);
                return;
            }
    }
    else
    {
        for (auto it = markers.rbegin(); it != markers.rend(); ++it)
            if (it->seconds < pos - 0.01)
            {
                setPositionSeconds (it->seconds);
                return;
            }
    }
}

//==============================================================================
te::MidiClip* EngineHost::createMidiClip (te::AudioTrack& track, double startSeconds, double lengthSeconds)
{
    const auto range = te::TimeRange (te::TimePosition::fromSeconds (juce::jmax (0.0, startSeconds)),
                                      te::TimePosition::fromSeconds (juce::jmax (0.1, startSeconds + lengthSeconds)));

    return te::insertMIDIClip (track, "MIDI", range).get();
}

std::vector<te::MidiClip*> EngineHost::getMidiClips (te::AudioTrack& track) const
{
    std::vector<te::MidiClip*> result;

    for (auto* clip : track.getClips())
        if (auto* midi = dynamic_cast<te::MidiClip*> (clip))
            result.push_back (midi);

    return result;
}

te::Clip* EngineHost::getClipAt (te::AudioTrack& track, double seconds) const
{
    for (auto* clip : track.getClips())
    {
        const auto time = clip->getPosition().time;

        if (seconds >= time.getStart().inSeconds() - 0.001 && seconds <= time.getEnd().inSeconds() + 0.001)
            return clip;
    }

    return nullptr;
}

std::vector<NoteInfo> EngineHost::getNotes (te::MidiClip& clip) const
{
    std::vector<NoteInfo> result;

    for (auto* note : clip.getSequence().getNotes())
    {
        NoteInfo info;
        info.startBeat   = note->getStartBeat().inBeats();
        info.lengthBeats = note->getLengthBeats().inBeats();
        info.pitch       = note->getNoteNumber();
        info.velocity    = note->getVelocity();
        result.push_back (info);
    }

    return result;
}

void EngineHost::addNote (te::MidiClip& clip, const NoteInfo& info)
{
    clip.getSequence().addNote (juce::jlimit (0, 127, info.pitch),
                                te::BeatPosition::fromBeats (juce::jmax (0.0, info.startBeat)),
                                te::BeatDuration::fromBeats (juce::jmax (0.05, info.lengthBeats)),
                                juce::jlimit (1, 127, info.velocity),
                                0, &edit->getUndoManager());
}

void EngineHost::updateNote (te::MidiClip& clip, int index, const NoteInfo& info)
{
    const auto notes = clip.getSequence().getNotes();

    if (index < 0 || index >= notes.size())
        return;

    auto& undo = edit->getUndoManager();
    undo.beginNewTransaction();

    auto* note = notes[index];
    note->setStartAndLength (te::BeatPosition::fromBeats (juce::jmax (0.0, info.startBeat)),
                             te::BeatDuration::fromBeats (juce::jmax (0.05, info.lengthBeats)),
                             &undo);
    note->setNoteNumber (juce::jlimit (0, 127, info.pitch), &undo);
    note->setVelocity (juce::jlimit (1, 127, info.velocity), &undo);

    // Une seule annulation pour toute la modification de la note.
    undo.beginNewTransaction();
}

void EngineHost::removeNote (te::MidiClip& clip, int index)
{
    const auto notes = clip.getSequence().getNotes();

    if (index >= 0 && index < notes.size())
        clip.getSequence().removeNote (*notes[index], &edit->getUndoManager());
}

void EngineHost::applyQuantisation (te::MidiClip& clip)
{
    te::QuantisationType quantisation;
    quantisation.setType ("1/" + juce::String (pendingQuantDivision));

    auto& undo = edit->getUndoManager();
    undo.beginNewTransaction();

    for (auto* note : clip.getSequence().getNotes())
        note->setStartAndLength (quantisation.roundBeatToNearest (note->getStartBeat()),
                                 note->getLengthBeats(), &undo);

    // Ctrl+Z annule la quantification entière, pas une note à la fois.
    undo.beginNewTransaction();
}

void EngineHost::quantize (te::MidiClip& clip, int division)
{
    pendingQuantDivision = juce::jlimit (1, 32, division);
    applyQuantisation (clip);
}

void EngineHost::splitClip (te::Clip& clip, double seconds)
{
    auto& undo = edit->getUndoManager();
    undo.beginNewTransaction();
    te::split (clip, te::TimePosition::fromSeconds (seconds));
}

void EngineHost::deleteClip (te::Clip& clip)
{
    edit->getUndoManager().beginNewTransaction();
    clip.removeFromParent();
    // La sauvegarde est faite par l'appelant (une seule fois par action).
}

void EngineHost::duplicateClip (te::Clip& clip)
{
    edit->getUndoManager().beginNewTransaction();

    auto* track = dynamic_cast<te::AudioTrack*> (clip.getParent());

    if (track == nullptr)
        return;

    auto state = clip.state.createCopy();
    edit->createNewItemID().writeID (state, nullptr);

    if (auto* copy = track->insertClipWithState (state))
    {
        const auto old = clip.getPosition();
        te::ClipPosition position;
        position.offset = old.offset;
        position.time = te::TimeRange (te::TimePosition::fromSeconds (old.time.getEnd().inSeconds() + 0.05),
                                       old.time.getLength());
        copy->setPosition (position);
    }
}

void EngineHost::moveClip (te::Clip& clip, double deltaSeconds)
{
    auto position = clip.getPosition();
    position.time = te::TimeRange (te::TimePosition::fromSeconds (position.time.getStart().inSeconds() + deltaSeconds),
                                   position.time.getLength());
    clip.setPosition (position);
}

bool EngineHost::stretchAudioClip (te::Clip& clip, double factor)
{
    // Le time-stretch ne concerne que l'audio : un clip MIDI est laissé intact.
    auto* audio = dynamic_cast<te::AudioClipBase*> (&clip);

    if (audio == nullptr)
        return false;

    // Toutes les validations avant la transaction : rien n'est modifié si le
    // facteur est invalide.
    if (! std::isfinite (factor) || factor <= 0.01)
        return false;

    // ARA (Melodyne…) : son moteur gère ses propres temps/hauteurs. Non testable
    // ici, on refuse plutôt que de risquer une position de clip incohérente.
    if (te::TimeStretcher::isARA (audio->getTimeStretchMode()))
        return false;

    edit->getUndoManager().beginNewTransaction();

    // rescale() n'agit sur un clip audio que hors auto-tempo.
    if (audio->getAutoTempo())
        audio->setAutoTempo (false);

    // Sans mode explicite, un changement de vitesse changerait la hauteur.
    // On force le mode par défaut (SoundTouch ici) quand le clip est en
    // `disabled` : la durée varie, la tonalité reste la même.
    if (audio->getTimeStretchMode() == te::TimeStretcher::disabled)
        audio->setTimeStretchMode (te::TimeStretcher::defaultMode);

    // Pivot au début du clip : le début reste fixe, seule la durée change.
    clip.rescale (clip.getPosition().time.getStart(), factor);
    return true;
}

bool EngineHost::cutMidiNotes (te::MidiClip& clip, double seconds)
{
    const auto time = clip.getPosition().time;
    const double start = time.getStart().inSeconds();
    const double end = time.getEnd().inSeconds();

    // Toutes les validations avant la transaction : rien n'est modifié si la
    // tête de lecture est en dehors du clip. Les clips bouclés sont refusés :
    // getContentBeatAtTime() ne replie pas sur la boucle.
    if (! std::isfinite (seconds) || seconds <= start || seconds >= end || clip.isLooping())
        return false;

    const double cutBeat = clip.getContentBeatAtTime (te::TimePosition::fromSeconds (seconds)).inBeats();
    auto& undo = edit->getUndoManager();
    undo.beginNewTransaction();

    // Copie de la liste : removeNote() modifie la séquence pendant la boucle.
    const auto notes = clip.getSequence().getNotes();

    for (int i = notes.size() - 1; i >= 0; --i)
    {
        auto* note = notes[i];
        const double noteStart = note->getStartBeat().inBeats();
        const double noteEnd = noteStart + note->getLengthBeats().inBeats();

        if (noteStart >= cutBeat)
        {
            clip.getSequence().removeNote (*note, &undo);
        }
        else if (noteEnd > cutBeat)
        {
            note->setStartAndLength (te::BeatPosition::fromBeats (noteStart),
                                     te::BeatDuration::fromBeats (cutBeat - noteStart),
                                     &undo);
        }
    }

    // Le motif s'arrête à la tête de lecture.
    auto pos = clip.getPosition();
    pos.time = pos.time.withEnd (te::TimePosition::fromSeconds (seconds));
    clip.setPosition (pos);
    return true;
}

bool EngineHost::insertMidiSpace (te::MidiClip& clip, double seconds, double beats)
{
    const auto time = clip.getPosition().time;
    const double start = time.getStart().inSeconds();
    const double end = time.getEnd().inSeconds();

    if (! std::isfinite (seconds) || ! std::isfinite (beats) || beats <= 0.0
        || seconds <= start || seconds >= end || clip.isLooping())
        return false;

    const double cutBeat = clip.getContentBeatAtTime (te::TimePosition::fromSeconds (seconds)).inBeats();

    // Un beat n'a pas toujours la même durée : on vérifie l'espace au point de
    // coupe, puis la fin du clip suit le dernier beat décalé (tempo variable).
    const auto cutTime = clip.getTimeOfContentBeat (te::BeatPosition::fromBeats (cutBeat));
    const auto shiftedTime = clip.getTimeOfContentBeat (te::BeatPosition::fromBeats (cutBeat + beats));
    const double spaceSeconds = shiftedTime.inSeconds() - cutTime.inSeconds();

    const double endBeat = clip.getContentBeatAtTime (te::TimePosition::fromSeconds (end)).inBeats();
    const auto newEnd = clip.getTimeOfContentBeat (te::BeatPosition::fromBeats (endBeat + beats));

    if (! std::isfinite (spaceSeconds) || spaceSeconds <= 0.0
        || ! std::isfinite (newEnd.inSeconds()) || newEnd.inSeconds() <= end)
        return false;

    auto& undo = edit->getUndoManager();
    undo.beginNewTransaction();

    // Copie de la liste : addNote() modifie la séquence pendant la boucle.
    const auto notes = clip.getSequence().getNotes();

    for (auto* note : notes)
    {
        const double noteStart = note->getStartBeat().inBeats();
        const double noteEnd = noteStart + note->getLengthBeats().inBeats();

        if (noteStart >= cutBeat)
        {
            note->setStartAndLength (te::BeatPosition::fromBeats (noteStart + beats),
                                     te::BeatDuration::fromBeats (noteEnd - noteStart),
                                     &undo);
        }
        else if (noteEnd > cutBeat)
        {
            // Note à cheval : on copie d'abord la note entière (couleur,
            // vélocité de note off, mute), puis on sépare tête et queue.
            auto* tail = clip.getSequence().addNote (*note, &undo);
            note->setStartAndLength (te::BeatPosition::fromBeats (noteStart),
                                     te::BeatDuration::fromBeats (cutBeat - noteStart),
                                     &undo);
            tail->setStartAndLength (te::BeatPosition::fromBeats (cutBeat + beats),
                                     te::BeatDuration::fromBeats (noteEnd - cutBeat),
                                     &undo);
        }
    }

    // Le clip suit le décalage : sa fin recule d'un beat dans son propre repère.
    auto pos = clip.getPosition();
    pos.time = pos.time.withEnd (newEnd);
    clip.setPosition (pos);
    return true;
}

void EngineHost::copyClip (te::Clip& clip)
{
    te::SelectableList selected;
    selected.add (&clip);

    auto content = std::make_unique<te::Clipboard::Clips>();
    content->addSelectedClips (selected, te::Edit::getMaximumEditTimeRange(),
                               te::Clipboard::Clips::AutomationLocked::no);
    te::Clipboard::getInstance()->setContent (std::move (content));
}

bool EngineHost::pasteClip (te::AudioTrack& track, double seconds)
{
    const auto* content = te::Clipboard::getInstance()->getContentWithType<te::Clipboard::Clips>();

    if (content == nullptr || content->clips.empty())
        return false;

    auto& undo = edit->getUndoManager();
    undo.beginNewTransaction();

    te::EditInsertPoint insertPoint (*edit);
    te::Clipboard::ContentType::EditPastingOptions options (*edit, insertPoint);
    options.startTrack = &track;
    options.startTime = te::TimePosition::fromSeconds (seconds);

    const bool ok = content->pasteIntoEdit (options);

    // Termine la transaction : Ctrl+Z annule exactement ce collage.
    undo.beginNewTransaction();
    return ok;
}

//==============================================================================
void EngineHost::addPluginFolder (const juce::File& folder)
{
    if (! folder.isDirectory())
        return;

    auto folders = getPluginFolders();

    if (! folders.contains (folder.getFullPathName()))
    {
        folders.add (folder.getFullPathName());
        engine.getPropertyStorage().getPropertiesFile().setValue ("fdgpePluginFolders", folders.joinIntoString (";"));
    }

    scanForPlugins();
}

juce::StringArray EngineHost::getPluginFolders() const
{
    const auto value = engine.getPropertyStorage().getPropertiesFile().getValue ("fdgpePluginFolders", "");
    juce::StringArray result;
    result.addTokens (value, ";", "");
    result.removeEmptyStrings();
    return result;
}

juce::File EngineHost::getPluginBlacklistFile() const
{
    return juce::File::getSpecialLocation (juce::File::userDocumentsDirectory)
        .getChildFile ("FD-gPE")
        .getChildFile ("plugin-blacklist.txt");
}

void EngineHost::loadPluginBlacklist()
{
    const auto ids = juce::StringArray::fromLines (getPluginBlacklistFile().loadFileAsString());
    auto& list = engine.getPluginManager().knownPluginList;

    for (const auto& id : ids)
        if (id.isNotEmpty() && ! list.getBlacklistedFiles().contains (id))
            list.addToBlacklist (id);
}

void EngineHost::savePluginBlacklist()
{
    const auto file = getPluginBlacklistFile();
    file.getParentDirectory().createDirectory();
    file.replaceWithText (engine.getPluginManager().knownPluginList.getBlacklistedFiles()
                              .joinIntoString ("\n"));
}

bool EngineHost::isPluginBlacklisted (const juce::String& identifier) const
{
    return engine.getPluginManager().knownPluginList.getBlacklistedFiles().contains (identifier);
}

void EngineHost::setPluginBlacklisted (const juce::String& identifier, bool blocked)
{
    auto& list = engine.getPluginManager().knownPluginList;

    if (blocked)
    {
        list.addToBlacklist (identifier);
    }
    else
    {
        list.removeFromBlacklist (identifier);
        // Sinon le prochain scan réinscrirait le plugin depuis ce fichier.
        engine.getTemporaryFileManager().getTempFile ("FdPluginScanDeadMansPedal").deleteFile();
    }

    savePluginBlacklist();
}

void EngineHost::scanForPlugins (const std::function<void (const juce::String&)>& progress)
{
    auto& pm = engine.getPluginManager();

    for (auto* format : pm.pluginFormatManager.getFormats())
    {
        if (format == nullptr || ! format->canScanForPlugins())
            continue;

        juce::FileSearchPath searchPath;
        searchPath.addPath (format->getDefaultLocationsToSearch());

        for (const auto& folder : getPluginFolders())
            searchPath.addIfNotAlreadyThere (juce::File (folder));

        juce::PluginDirectoryScanner scanner (
            pm.knownPluginList, *format, searchPath, true,
            engine.getTemporaryFileManager().getTempFile ("FdPluginScanDeadMansPedal"), false);

        juce::String currentName;

        while (scanner.scanNextFile (true, currentName))
            if (progress)
                progress (currentName);
    }
}

std::vector<PluginInfo> EngineHost::getAvailablePlugins() const
{
    std::vector<PluginInfo> result;

    // Synthés et instruments intégrés
    addBuiltIn<te::FourOscPlugin>       (result, true,  "Synthé");
    addBuiltIn<te::SamplerPlugin>       (result, true,  "Sampler");

    // Effets intégrés
    addBuiltIn<te::EqualiserPlugin>     (result, false, "Égaliseur");
    addBuiltIn<te::CompressorPlugin>    (result, false, "Compresseur");
    addBuiltIn<te::ReverbPlugin>        (result, false, "Réverbération");
    addBuiltIn<te::DelayPlugin>         (result, false, "Délai");
    addBuiltIn<te::ChorusPlugin>        (result, false, "Chorus");
    addBuiltIn<te::PhaserPlugin>        (result, false, "Phaser");
    addBuiltIn<te::LowPassPlugin>       (result, false, "Filtre");
    addBuiltIn<te::PitchShiftPlugin>    (result, false, "Pitch");
    addBuiltIn<te::MidiModifierPlugin>  (result, false, "MIDI");

    // Plugins externes déjà scannés
    for (const auto& desc : engine.getPluginManager().knownPluginList.getTypes())
    {
        PluginInfo info;
        info.name         = desc.descriptiveName.isNotEmpty() ? desc.descriptiveName : desc.name;
        info.category     = desc.category.isNotEmpty() ? desc.category : (desc.isInstrument ? "Instrument" : "Effet");
        info.manufacturer = desc.manufacturerName;
        info.type         = te::ExternalPlugin::xmlTypeName;
        info.identifier   = desc.createIdentifierString();
        info.instrument   = desc.isInstrument;
        info.builtIn      = false;
        info.blacklisted  = isPluginBlacklisted (info.identifier);
        info.description  = desc;
        result.push_back (info);
    }

    // Plugins blacklistés : ils n'apparaissent plus dans getTypes(), on les
    // affiche quand même pour permettre une réhabilitation depuis l'interface.
    for (const auto& identifier : engine.getPluginManager().knownPluginList.getBlacklistedFiles())
    {
        if (identifier.isEmpty())
            continue;

        PluginInfo info;
        info.name        = identifier;
        info.category    = tr ("Bloqué");
        info.type        = te::ExternalPlugin::xmlTypeName;
        info.identifier  = identifier;
        info.instrument  = false;
        info.builtIn     = false;
        info.blacklisted = true;
        result.push_back (info);
    }

    return result;
}

juce::PluginDescription EngineHost::findDescription (const PluginInfo& info) const
{
    for (const auto& desc : engine.getPluginManager().knownPluginList.getTypes())
        if (desc.createIdentifierString() == info.identifier
            || (desc.name == info.name && desc.fileOrIdentifier == info.description.fileOrIdentifier))
            return desc;

    return info.description;
}

bool EngineHost::isMixerPlugin (te::Plugin& plugin)
{
    const auto type = plugin.getPluginType();
    return type == te::VolumeAndPanPlugin::xmlTypeName
        || type == te::LevelMeterPlugin::xmlTypeName;
}

std::vector<te::Plugin::Ptr> EngineHost::getTrackPlugins (te::AudioTrack& track) const
{
    std::vector<te::Plugin::Ptr> result;

    for (auto* plugin : track.pluginList.getPlugins())
        if (plugin != nullptr && ! isMixerPlugin (*plugin))
            result.push_back (plugin);

    return result;
}

void EngineHost::ensureAuxBus (int bus, const juce::String& name)
{
    if (edit->getAuxBusName (bus) != name)
        edit->setAuxBusName (bus, name);
}

juce::String EngineHost::getAuxBusName (int bus) const
{
    return edit->getAuxBusName (bus);
}

te::AuxSendPlugin* EngineHost::getOrCreateAuxSend (te::AudioTrack& track, int bus)
{
    ensureAuxBus (bus, tr ("Bus ") + juce::String (bus + 1));
    ensureAuxReturn (bus);

    if (auto* send = track.getAuxSendPlugin (bus, te::AuxPosition::byBus))
        return send;

    auto plugin = edit->getPluginCache().createNewPlugin (te::AuxSendPlugin::xmlTypeName, {});

    if (plugin == nullptr)
        return nullptr;

    track.pluginList.insertPlugin (plugin, track.pluginList.size(), nullptr);

    if (auto* send = dynamic_cast<te::AuxSendPlugin*> (plugin.get()))
    {
        send->busNumber = bus;
        return send;
    }

    return nullptr;
}

te::AuxSendPlugin* EngineHost::getAuxSend (te::AudioTrack& track, int bus) const
{
    return track.getAuxSendPlugin (bus, te::AuxPosition::byBus);
}

void EngineHost::removeAuxSend (te::AudioTrack& track, int bus)
{
    if (auto* send = track.getAuxSendPlugin (bus, te::AuxPosition::byBus))
        send->removeFromParent();
}

te::AuxReturnPlugin* EngineHost::ensureAuxReturn (int bus)
{
    // Migration douce : retire un ancien retour Bus placé sur le master.
    if (auto* master = edit->getMasterTrack())
    {
        for (auto* ret : master->pluginList.getPluginsOfType<te::AuxReturnPlugin>())
        {
            if (ret->busNumber == bus)
            {
                ret->removeFromParent();
                break;
            }
        }
    }

    te::AudioTrack* returnTrack = nullptr;

    for (auto* t : te::getAudioTracks (*edit))
    {
        if (isAuxReturnTrack (*t) && (int) t->state.getProperty (auxReturnId, -1) == bus)
        {
            returnTrack = t;
            break;
        }
    }

    if (returnTrack == nullptr)
    {
        edit->ensureNumberOfAudioTracks (te::getAudioTracks (*edit).size() + 1);
        returnTrack = te::getAudioTracks (*edit).getLast();

        if (returnTrack == nullptr)
            return nullptr;

        returnTrack->state.setProperty (auxReturnId, bus, nullptr);
        returnTrack->state.setProperty (kindId, "audio", nullptr);
        returnTrack->setName (tr ("Retour Bus ") + juce::String (bus + 1));
        returnTrack->setColour (juce::Colour (0xff5a8a6a));
    }

    for (auto* ret : returnTrack->pluginList.getPluginsOfType<te::AuxReturnPlugin>())
        if (ret->busNumber == bus)
            return ret;

    auto plugin = edit->getPluginCache().createNewPlugin (te::AuxReturnPlugin::xmlTypeName, {});

    if (plugin == nullptr)
        return nullptr;

    returnTrack->pluginList.insertPlugin (plugin, 0, nullptr);

    if (auto* ret = dynamic_cast<te::AuxReturnPlugin*> (plugin.get()))
    {
        ret->busNumber = bus;
        return ret;
    }

    return nullptr;
}

void EngineHost::saveTheme (bool dark, const juce::Colour& accent)
{
    auto& props = engine.getPropertyStorage().getPropertiesFile();
    props.setValue ("fdgpeThemeDark", dark ? "1" : "0");
    props.setValue ("fdgpeAccent", accent.toString());
    props.save();
}

std::pair<bool, juce::Colour> EngineHost::loadTheme() const
{
    auto& props = engine.getPropertyStorage().getPropertiesFile();

    // Deux accents prédéfinis : vert clair (par défaut) et jaune clair.
    constexpr juce::uint32 vertClair  = 0xff5fd3b0;
    constexpr juce::uint32 jauneClair = 0xffd9d28a;

    // Migration v2 : nouvelle interface sombre à plat. Au premier lancement
    // après la mise à jour, on passe en sombre + vert clair ; l'utilisateur
    // peut ensuite rechoisir clair/jaune dans « Apparence… ».
    if (props.getValue ("fdgpeThemeV2", "0") != "1")
    {
        props.setValue ("fdgpeThemeV2", "1");
        props.setValue ("fdgpeThemeDark", "1");
        props.setValue ("fdgpeAccent", juce::Colour (vertClair).toString());
        props.save();
    }

    const bool dark = props.getValue ("fdgpeThemeDark", "1") == "1";
    auto accent = juce::Colour::fromString (props.getValue ("fdgpeAccent", ""));
    const auto argb = accent.getARGB();

    if (argb != vertClair && argb != jauneClair)
        accent = juce::Colour (vertClair);

    return { dark, accent };
}

te::Plugin::Ptr EngineHost::ensureDrumSampler (te::AudioTrack& track)
{
    const auto plugins = getTrackPlugins (track);

    for (auto& plugin : plugins)
        if (dynamic_cast<te::SamplerPlugin*> (plugin.get()) != nullptr)
            return plugin;

    // On crée le sampler AVANT de retirer le synthé : si la création échoue,
    // la piste reste jouable et l'état en mémoire reste conforme au projet.
    auto sampler = edit->getPluginCache().createNewPlugin (te::SamplerPlugin::xmlTypeName, {});

    if (sampler == nullptr)
        return nullptr;

    for (auto& plugin : plugins)
        if (auto* synth = dynamic_cast<te::FourOscPlugin*> (plugin.get()))
        {
            removePluginFromTrack (track, *synth);
            break;
        }

    track.pluginList.insertPlugin (sampler, 0, nullptr);
    return sampler;
}

juce::String EngineHost::loadSampleForNote (te::AudioTrack& track, int midiNote, const juce::File& file)
{
    if (! file.existsAsFile())
        return tr ("Fichier audio introuvable.");

    auto samplerPtr = ensureDrumSampler (track);
    auto* sampler = dynamic_cast<te::SamplerPlugin*> (samplerPtr.get());

    if (sampler == nullptr)
        return tr ("Impossible de créer le sampler.");

    // Un seul son par note : on remplace l'ancien s'il existe déjà.
    const int note = juce::jlimit (0, 127, midiNote);

    for (int i = 0; i < sampler->getNumSounds(); ++i)
        if (sampler->getKeyNote (i) == note)
        {
            sampler->removeSound (i);
            break;
        }

    const auto error = sampler->addSound (file.getFullPathName(),
                                          file.getFileNameWithoutExtension(),
                                          0.0, 0.0, 0.0f);

    if (error.isNotEmpty())
    {
        // Le sampler a peut-être déjà remplacé le synthé : on aligne le disque.
        saveProject();
        return error;
    }

    const int index = sampler->getNumSounds() - 1;
    sampler->setSoundParams (index, note, note, note);
    sampler->flushPendingUpdates();
    saveProject();
    return {};
}

juce::String EngineHost::getSampleNameForNote (te::AudioTrack& track, int midiNote) const
{
    for (auto& plugin : getTrackPlugins (track))
        if (auto* sampler = dynamic_cast<te::SamplerPlugin*> (plugin.get()))
            for (int i = 0; i < sampler->getNumSounds(); ++i)
                if (sampler->getKeyNote (i) == midiNote)
                    return sampler->getSoundName (i);

    return {};
}

te::Plugin::Ptr EngineHost::addPluginToTrack (te::AudioTrack& track, const PluginInfo& info)
{
    te::Plugin::Ptr plugin;

    if (info.builtIn)
        plugin = edit->getPluginCache().createNewPlugin (info.type, {});
    else
        plugin = edit->getPluginCache().createNewPlugin (te::ExternalPlugin::xmlTypeName, findDescription (info));

    if (plugin != nullptr)
    {
        // Un instrument se place en tête de chaîne (avant les effets).
        const int index = info.instrument ? 0 : track.pluginList.size();
        track.pluginList.insertPlugin (plugin, index, nullptr);
        saveProject();
    }

    return plugin;
}

void EngineHost::removePluginFromTrack (te::AudioTrack& track, te::Plugin& plugin)
{
    if (track.pluginList.contains (&plugin))
        plugin.removeFromParent();
}

void EngineHost::movePlugin (te::AudioTrack& track, te::Plugin& plugin, int delta)
{
    const int index = track.pluginList.indexOf (&plugin);

    if (index < 0)
        return;

    const int target = juce::jlimit (0, track.pluginList.size() - 1, index + delta);

    if (target != index)
        track.pluginList.insertPlugin (te::Plugin::Ptr (&plugin), target, nullptr);
}

void EngineHost::setPluginEnabled (te::Plugin& plugin, bool enabled)
{
    plugin.setEnabled (enabled);
}

juce::String EngineHost::getPluginDisplayName (te::Plugin& plugin)
{
    return plugin.getDisplayName();
}

//==============================================================================
bool EngineHost::exportWav (const juce::File& destination, const std::function<void (float)>& progress)
{
    ExportOptions options;
    options.destination = destination;
    return exportAudio (options, progress);
}

bool EngineHost::exportAudio (const ExportOptions& options, const std::function<void (float)>& progress)
{
    if (options.destination.getFileName().isEmpty())
        return false;

    const double end = options.endSeconds > options.startSeconds
                           ? options.endSeconds
                           : juce::jmax (options.startSeconds + 1.0, getProjectLengthSeconds());

    te::Renderer::Parameters params (*edit);
    params.destFile = options.destination;
    params.audioFormat = options.destination.hasFileExtension ("flac")
                             ? engine.getAudioFileFormatManager().getFlacFormat()
                             : engine.getAudioFileFormatManager().getWavFormat();
    params.sampleRateForAudio = juce::jlimit (8000.0, 192000.0, options.sampleRate);
    params.bitDepth = juce::jlimit (16, 32, options.bitDepth);
    params.usePlugins = options.usePlugins;
    params.time = te::TimeRange (te::TimePosition::fromSeconds (juce::jmax (0.0, options.startSeconds)),
                                 te::TimePosition::fromSeconds (end));

    const auto result = te::Renderer::renderToFile ("Export audio", params);

    if (progress)
        progress (1.0f);

    return result.existsAsFile();
}

float EngineHost::getMasterVolume() const
{
    if (auto master = edit->getMasterVolumePlugin())
        return master->getSliderPos();

    return 0.8f;
}

void EngineHost::setMasterVolume (float sliderPos)
{
    edit->setMasterVolumeSliderPos (juce::jlimit (0.0f, 1.0f, sliderPos));
}

juce::String EngineHost::getAudioDeviceName()
{
    if (auto* device = engine.getDeviceManager().deviceManager.getCurrentAudioDevice())
        return device->getName();

    return {};
}

float EngineHost::getCpuUsage()
{
    return engine.getDeviceManager().getCpuUsage();
}

juce::String EngineHost::getEngineVersion()
{
    return te::Engine::getVersion();
}
} // namespace fdgpe
