#pragma once

#include <functional>
#include <memory>
#include <vector>

#include <juce_core/juce_core.h>
#include <tracktion_engine/tracktion_engine.h>

namespace te = tracktion;

namespace fdgpe
{
/** Description d'un plugin disponible (interne Tracktion ou VST3 externe). */
struct PluginInfo
{
    juce::String name;
    juce::String category;
    juce::String manufacturer;
    juce::String type;          // xmlTypeName : "4osc", "reverb", "vst", ...
    juce::String identifier;
    bool instrument = false;
    bool builtIn = false;
    bool blacklisted = false;
    juce::PluginDescription description;   // utile seulement pour les plugins externes
};

/** Détecte le mode « processus enfant » du scanner Tracktion. À appeler avant
    toute fenêtre dans JUCEApplication::initialise(). */
inline bool handlePluginScanChildProcess (const juce::String& commandLine)
{
    return te::PluginManager::startChildProcessPluginScan (commandLine);
}

/** Autorise le scan des plugins dans un processus séparé. */
class FdEngineBehaviour final : public te::EngineBehaviour
{
public:
    bool canScanPluginsOutOfProcess() override { return true; }
};

/** Un point de la carte de tempo. */
struct TempoPoint
{
    double beat = 0.0;
    double bpm = 120.0;
    int numerator = 4;
    int denominator = 4;
};

/** Un changement de signature rythmique. */
struct TimeSigPoint
{
    double beat = 0.0;
    int numerator = 4;
    int denominator = 4;
};

/** Résultat d'un export audio. */
struct ExportOptions
{
    juce::File destination;
    double sampleRate = 44100.0;
    int bitDepth = 24;
    bool usePlugins = true;
    double startSeconds = 0.0;
    double endSeconds = -1.0;   // < 0 : longueur du projet
};

/** Un marqueur de projet. */
struct MarkerInfo
{
    double seconds = 0.0;
    juce::String name;
    int id = 0;
};

/** Une note MIDI, en temps mesuré en battements depuis le début du clip. */
struct NoteInfo
{
    double startBeat = 0.0;
    double lengthBeats = 1.0;
    int pitch = 60;
    int velocity = 100;
};

/** Enveloppe autour de Tracktion Engine : seul endroit qui parle au moteur.
    Elle fournit les fonctions DAW complètes : pistes, transport, tempo, MIDI,
    marqueurs, plugins et export. */
class EngineHost
{
public:
    EngineHost();
    ~EngineHost();

    EngineHost (const EngineHost&) = delete;
    EngineHost& operator= (const EngineHost&) = delete;

    te::Engine& getEngine() noexcept  { return engine; }
    te::Edit&   getEdit() noexcept    { return *edit; }

    //==============================================================================
    // Projet
    juce::File getProjectFile() const   { return projectFile; }
    juce::String getProjectName() const { return projectFile.getFileNameWithoutExtension(); }
    void saveProject();
    bool saveProjectAs (const juce::File&);
    bool newProject();
    bool openProject (const juce::File&);
    double getProjectLengthSeconds() const;

    //==============================================================================
    // Pistes
    std::vector<te::AudioTrack*> getTracks();
    te::AudioTrack* addAudioTrack();
    te::AudioTrack* addMidiTrack();
    te::AudioTrack* addDrumTrack();
    te::AudioTrack* duplicateTrack (te::AudioTrack&);
    void deleteTrack (te::AudioTrack&);
    void renameTrack (te::AudioTrack&, const juce::String&);
    void setTrackColour (te::AudioTrack&, const juce::Colour&);
    juce::Colour getTrackColour (const te::AudioTrack&) const;
    void importAudioFile (te::AudioTrack&, const juce::File&, double startSeconds);

    bool isMidiTrack (te::AudioTrack&) const;
    bool isDrumTrack (te::AudioTrack&) const;
    bool isAuxReturnTrack (te::AudioTrack&) const;
    int getAuxReturnBus (te::AudioTrack&) const;
    std::vector<te::AudioTrack*> getAuxReturnTracks();

    bool isTrackArmed (te::AudioTrack&);
    bool anyTrackArmed();
    void setTrackArmed (te::AudioTrack&, bool armed);

    //==============================================================================
    // Transport
    void togglePlay();
    void toggleRecord();
    void returnToStart();
    bool isPlaying() const;
    bool isRecording() const;

    bool isLooping() const;
    void setLooping (bool);
    void setLoopRange (double startSeconds, double endSeconds);
    std::pair<double, double> getLoopRange() const;

    bool isMetronomeOn() const;
    void setMetronomeOn (bool);

    double getTempoBpm() const;                 // BPM à la position courante
    void setTempoBpm (double bpm);              // modifie le marqueur couvrant la position
    void tapTempo();

    std::pair<int, int> getTimeSignature() const;
    void setTimeSignature (int numerator, int denominator);

    double getPositionSeconds() const;
    void setPositionSeconds (double seconds);
    double getPositionBeats() const;
    double getRecordStartSeconds() const noexcept   { return recordStartSeconds; }

    struct BarsBeats { int bar = 1; int beat = 1; double fraction = 0.0; };
    BarsBeats getBarsBeats() const;

    bool canUndo() const;
    bool canRedo() const;
    void undo();
    void redo();

    //==============================================================================
    // Clavier virtuel
    void sendNoteOn (int midiNote, float velocity);
    void sendNoteOff (int midiNote);
    // Routage vers une piste MIDI précise (clavier à l'écran / touches).
    void sendMidiToTrack (te::AudioTrack&, const juce::MidiMessage&);
    void sendNoteOn (te::AudioTrack&, int midiNote, float velocity);
    void sendNoteOff (te::AudioTrack&, int midiNote);
    void sendNoteOn (te::MidiClip&, int midiNote, float velocity);
    void sendNoteOff (te::MidiClip&, int midiNote);

    //==============================================================================
    // Carte de tempo
    std::vector<TempoPoint> getTempoMap() const;
    std::vector<TimeSigPoint> getTimeSigMap() const;
    void setTempoAtBeat (double beat, double bpm);
    void removeTempoAt (int index);
    void setTimeSigAtBeat (double beat, int numerator, int denominator);
    void removeTimeSigAt (int index);
    double secondsToBeat (double seconds) const;
    double beatToSeconds (double beat) const;
    double snapSeconds (double seconds, double divisionBeats) const;

    //==============================================================================
    // Marqueurs
    std::vector<MarkerInfo> getMarkers() const;
    void addMarker (const juce::String& name, double seconds);
    void removeMarker (int id);
    void goToMarker (bool next);

    //==============================================================================
    // Clips et MIDI
    te::MidiClip* createMidiClip (te::AudioTrack&, double startSeconds, double lengthSeconds);
    std::vector<te::MidiClip*> getMidiClips (te::AudioTrack&) const;
    te::Clip* getClipAt (te::AudioTrack&, double seconds) const;

    std::vector<NoteInfo> getNotes (te::MidiClip&) const;
    void addNote (te::MidiClip&, const NoteInfo&);
    void updateNote (te::MidiClip&, int index, const NoteInfo&);
    void removeNote (te::MidiClip&, int index);
    void quantize (te::MidiClip&, int division);  // division = 4 (1/4), 16 (1/16), ...

    void splitClip (te::Clip&, double seconds);
    void deleteClip (te::Clip&);
    void duplicateClip (te::Clip&);
    void moveClip (te::Clip&, double deltaSeconds);
    // Étirement audio (thread UI, audio seulement, hauteur conservée).
    bool stretchAudioClip (te::Clip&, double factor);
    // Édition MIDI sous les notes (thread UI, annulable).
    bool cutMidiNotes (te::MidiClip&, double seconds);
    bool insertMidiSpace (te::MidiClip&, double seconds, double beats);

    // Édition M2.2 : presse-papiers de clips (UI thread uniquement).
    void copyClip (te::Clip&);
    bool pasteClip (te::AudioTrack&, double seconds);

    //==============================================================================
    // Plugins : scan simple et gestion sur les pistes
    void scanForPlugins (const std::function<void (const juce::String&)>& progress = {});
    void addPluginFolder (const juce::File&);
    juce::StringArray getPluginFolders() const;
    void savePluginBlacklist();
    bool isPluginBlacklisted (const juce::String&) const;
    void setPluginBlacklisted (const juce::String&, bool);

    // Envois auxiliaires (Bus 1 fixe pour le premier slice M3.4).
    void ensureAuxBus (int bus, const juce::String& name);
    juce::String getAuxBusName (int bus) const;
    juce::String getAuxBusDisplayName (int bus) const;
    void setAuxBusName (int bus, const juce::String& name);
    te::AuxSendPlugin* getOrCreateAuxSend (te::AudioTrack&, int bus);
    te::AuxSendPlugin* getAuxSend (te::AudioTrack&, int bus) const;
    void removeAuxSend (te::AudioTrack&, int bus);
    te::AuxSendPlugin* setAuxSendBus (te::AudioTrack&, int oldBus, int newBus);
    te::AuxReturnPlugin* ensureAuxReturn (int bus);

    // Apparence : préférences enregistrées hors du projet.
    void saveTheme (bool dark, const juce::Colour& accent);
    std::pair<bool, juce::Colour> loadTheme() const;
    std::vector<PluginInfo> getAvailablePlugins() const;
    std::vector<te::Plugin::Ptr> getTrackPlugins (te::AudioTrack&) const;
    te::Plugin::Ptr addPluginToTrack (te::AudioTrack&, const PluginInfo&);

    // Sampler des pads de batterie (tout se passe sur le thread UI).
    te::Plugin::Ptr ensureDrumSampler (te::AudioTrack&);
    juce::String loadSampleForNote (te::AudioTrack&, int midiNote, const juce::File&);
    juce::String getSampleNameForNote (te::AudioTrack&, int midiNote) const;

    void removePluginFromTrack (te::AudioTrack&, te::Plugin&);
    void movePlugin (te::AudioTrack&, te::Plugin&, int delta);
    void setPluginEnabled (te::Plugin&, bool);
    static juce::String getPluginDisplayName (te::Plugin&);
    static bool isMixerPlugin (te::Plugin&);   // volume/pan et vu-mètre internes

    //==============================================================================
    // Export et audio
    bool exportWav (const juce::File& destination, const std::function<void (float)>& progress = {});
    bool exportAudio (const ExportOptions&, const std::function<void (float)>& progress = {});
    float getMasterVolume() const;
    void setMasterVolume (float sliderPos);

    juce::String getAudioDeviceName();
    float getCpuUsage();
    static juce::String getEngineVersion();

private:
    void setupInputDevices();
    void prepareEdit();
    void tagMissingTrackKinds();
    te::AudioTrack* addTrackOfKind (bool midi);
    int countTracksOfKind (bool midi);
    te::MidiInputDevice* keyboardDevice();
    void sendMidi (const juce::MidiMessage&);
    void applyQuantisation (te::MidiClip&);
    juce::PluginDescription findDescription (const PluginInfo&) const;
    juce::File getPluginBlacklistFile() const;
    void loadPluginBlacklist();

    te::Engine engine { "FD gPE", nullptr, std::make_unique<FdEngineBehaviour>() };
    std::unique_ptr<te::Edit> edit;
    juce::File projectFile;
    std::shared_ptr<te::MidiInputDevice> virtualKeyboard;
    double recordStartSeconds = 0.0;
    std::vector<double> tapTimes;
    int pendingQuantDivision = 16;
};
} // namespace fdgpe
