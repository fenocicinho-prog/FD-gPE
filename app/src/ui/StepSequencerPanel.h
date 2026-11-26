#pragma once

#include <algorithm>
#include <array>
#include <cmath>
#include <functional>

#include <juce_gui_basics/juce_gui_basics.h>

#include "../engine/EngineHost.h"
#include "Palette.h"
#include "Text.h"

namespace fdgpe
{
/** Séquenceur de pas : 6 instruments × 16 pas, enregistrés comme notes MIDI
    dans un clip d'une mesure. Tout se passe sur le thread UI. */
class StepSequencerPanel final : public juce::Component
{
public:
    explicit StepSequencerPanel (EngineHost& h, te::AudioTrack* target = nullptr)
        : host (h), targetTrack (target)
    {
        for (juce::TextButton* b : { &playButton, &clearButton, &closeButton })
            addAndMakeVisible (b);

        playButton.onClick = [this]
        {
            host.togglePlay();
            updatePlayhead();
        };
        clearButton.onClick = [this] { clearAll(); };
        closeButton.onClick = [this] { if (onClose) onClose(); };

        rebuildPatternFromClip();
    }

    std::function<void()> onClose;
    std::function<void()> onStructureChanged;

    void paint (juce::Graphics& g) override
    {
        g.fillAll (palette::panel);

        g.setColour (palette::blue);
        g.drawHorizontalLine (0, 0.0f, (float) getWidth());

        // Le titre peut utiliser toute la place avant EFFACER (96 px le coupait).
        const int titleWidth = juce::jmax (0, clearButton.getX() - 16);
        g.setFont (juce::Font (juce::FontOptions (11.0f, juce::Font::bold)));
        g.setColour (palette::text);
        g.drawText (titleWidth < 110 ? tr ("PAS") : tr ("SÉQUENCEUR DE PAS"),
                    8, 6, titleWidth, 22, juce::Justification::centredLeft);

        // Repère du pas courant : violet est un alias de blue, donc une simple
        // bordure se fond dans un pad actif. Bande discrète + petit témoin.
        if (currentStep >= 0)
        {
            const float x = (float) (gridX + currentStep * padStep - 2);
            g.setColour (palette::violet.withAlpha (0.14f));
            g.fillRoundedRectangle (x, (float) headerHeight - 2.0f, (float) padSize + 4.0f,
                                    (float) rows * rowHeight + 4.0f, 4.0f);
            g.setColour (palette::violet);
            g.fillEllipse ((float) (gridX + currentStep * padStep + 6), (float) headerHeight - 10.0f,
                           8.0f, 5.0f);
        }

        for (int row = 0; row < rows; ++row)
        {
            const int y = headerHeight + row * rowHeight;

            // Deux lignes bien séparées dans les 26 px de la rangée : le nom
            // de l'instrument reste sombre, le nom du son chargé passe en bleu.
            g.setColour (palette::text);
            g.setFont (juce::Font (juce::FontOptions (10.0f, juce::Font::bold)));
            g.drawText (rowName (row), 7, y + 1, labelWidth - 12, 12,
                        juce::Justification::centredLeft);

            // Le son chargé remplace l'invitation ; cliquer le nom ouvre le choix du WAV.
            const auto& sample = sampleNames[(std::size_t) row];
            juce::String shown = sample;

            if (shown.length() > 14)
                shown = shown.substring (0, 13) + juce::String::fromUTF8 ("…");

            g.setColour (sample.isNotEmpty() ? palette::blue : palette::textDim);
            g.setFont (juce::Font (juce::FontOptions (9.0f, juce::Font::plain)));
            g.drawText (sample.isNotEmpty() ? shown : tr ("cliquer pour charger"),
                        7, y + 13, labelWidth - 12, 12, juce::Justification::centredLeft);

            for (int step = 0; step < steps; ++step)
            {
                const auto r = padBounds (row, step);

                g.setColour (pattern[(std::size_t) row][(std::size_t) step]
                                 ? palette::blue : palette::laneAlt);
                g.fillRoundedRectangle (r, 3.0f);
                g.setColour (step == currentStep ? palette::violet : palette::line);
                g.drawRoundedRectangle (r, 3.0f, step == currentStep ? 1.6f : 1.0f);
            }
        }

        g.setColour (palette::textDim);
        g.setFont (juce::Font (juce::FontOptions (9.0f, juce::Font::plain)));
        g.drawText (tr ("Pads = pas. Cliquez un nom pour charger un WAV."),
                    8, getHeight() - 26, getWidth() - 110, 22, juce::Justification::centredLeft);
    }

    void resized() override
    {
        playButton.setBounds (getWidth() - 96, 3, 88, 28);
        clearButton.setBounds (getWidth() - 190, 3, 88, 28);
        closeButton.setBounds (getWidth() - 96, getHeight() - 34, 88, 28);
    }

    void mouseDown (const juce::MouseEvent& e) override
    {
        if (e.y < headerHeight || e.y >= headerHeight + rows * rowHeight)
            return;

        const int row = (e.y - headerHeight) / rowHeight;

        if (row < 0 || row >= rows)
            return;

        // Toute la colonne du libellé est cliquable ; la grille commence à gridX.
        // Pas de bande morte de 2 px qui activerait le premier pad par erreur.
        if (e.x < gridX)
        {
            loadSampleForRow (row);
            return;
        }

        const int step = (e.x - gridX) / padStep;

        if (step < 0 || step >= steps)
            return;

        if (e.x - gridX - step * padStep > padSize)
            return;

        toggleStep (row, step);
    }

    /** Appelé par le minuteur 20 Hz de MainComponent : suit la lecture et
        éteint la note d'aperçu. Thread UI uniquement, aucune E/S. */
    void updatePlayhead()
    {
        // La piste ou son clip peut avoir été supprimé depuis le panneau : on
        // éteint les pads restés allumés pour refléter l'état réel du projet.
        if (findClip() == nullptr && hasActiveSteps())
        {
            pattern = {};
            repaint();
        }

        if (pendingOffPitch >= 0)
        {
            // On relue la piste : un autre clavier peut avoir retargeté le device
            // depuis l'audition, et un pointeur conservé serait dangling.
            if (auto* track = findDrumTrack())
                host.sendNoteOff (*track, pendingOffPitch);
            else
                host.sendNoteOff (pendingOffPitch);
            pendingOffPitch = -1;
        }

        const double beats = std::fmod (juce::jmax (0.0, host.getPositionBeats()), 4.0);
        const int step = juce::jlimit (0, steps - 1, (int) (beats * 4.0));

        if (step != currentStep)
        {
            if (currentStep >= 0)
                repaint (columnBounds (currentStep));
            currentStep = step;
            repaint (columnBounds (step));
        }

        const bool playing = host.isPlaying();
        if (playing != wasPlaying)
        {
            wasPlaying = playing;
            playButton.setButtonText (playing ? tr ("STOP") : tr ("LECTURE"));
        }
    }

    void rebuildPatternFromClip()
    {
        pattern = {};
        currentStep = -1;

        auto* track = findDrumTrack();
        for (int i = 0; i < rows; ++i)
            sampleNames[(std::size_t) i] = track != nullptr
                                               ? host.getSampleNameForNote (*track, pitches[i])
                                               : juce::String {};

        auto* clip = findClip();
        if (clip == nullptr)
            return;

        const auto notes = host.getNotes (*clip);

        for (const auto& n : notes)
        {
            const int row = rowForPitch (n.pitch);
            const int step = juce::roundToInt (n.startBeat * 4.0);

            if (row >= 0 && step >= 0 && step < steps
                && std::abs (n.startBeat - (double) step * 0.25) < 0.05)
                pattern[(std::size_t) row][(std::size_t) step] = true;
        }
    }

private:
    static constexpr int rows = 6;
    static constexpr int steps = 16;
    static constexpr int labelWidth = 96;
    static constexpr int headerHeight = 34;
    static constexpr int rowHeight = 26;
    static constexpr int padSize = 20;
    static constexpr int padStep = 23;
    static constexpr int gridX = labelWidth + 6;
    static constexpr int pitches[rows] = { 36, 38, 42, 46, 45, 49 };

    static juce::String rowName (int row)
    {
        switch (row)
        {
            case 0: return tr ("GROSSE CAISSE");
            case 1: return tr ("CAISSE CLAIRE");
            case 2: return tr ("CHARLESTON");
            case 3: return tr ("CLAP");
            case 4: return tr ("TOM");
            case 5: return tr ("PERCUSSION");
            default: return {};
        }
    }

    static int rowForPitch (int pitch)
    {
        for (int i = 0; i < rows; ++i)
            if (pitches[i] == pitch)
                return i;

        return -1;
    }

    juce::Rectangle<float> padBounds (int row, int step) const
    {
        return { (float) (gridX + step * padStep), (float) (headerHeight + row * rowHeight + 3),
                 (float) padSize, (float) padSize };
    }

    /** Bande verticale d'une colonne, témoin compris : limite le repaint. */
    juce::Rectangle<int> columnBounds (int step) const
    {
        return { gridX + step * padStep - 3, headerHeight - 12, padSize + 6, rows * rowHeight + 16 };
    }

    te::AudioTrack* findDrumTrack()
    {
        if (targetTrack != nullptr && host.isDrumTrack (*targetTrack))
            return targetTrack;

        for (auto* track : host.getTracks())
            if (host.isDrumTrack (*track))
                return track;

        // Compatibilité : anciens projets où la piste batterie n'avait pas le flag.
        for (auto* track : host.getTracks())
            if (host.isMidiTrack (*track) && track->getName() == tr ("Batterie"))
                return track;

        return nullptr;
    }

    te::MidiClip* findClip()
    {
        auto* track = findDrumTrack();
        if (track == nullptr)
            return nullptr;

        for (auto* clip : host.getMidiClips (*track))
            if (std::abs (clip->getPosition().time.getStart().inSeconds()) < 0.01)
                return clip;

        return nullptr;
    }

    te::MidiClip* ensureClip()
    {
        auto* track = findDrumTrack();

        if (track == nullptr)
        {
            track = host.addDrumTrack();
            if (track == nullptr)
                return nullptr;

            if (onStructureChanged)
                onStructureChanged();
        }

        if (auto* clip = findClip())
            return clip;

        const double length = host.beatToSeconds (4.0) - host.beatToSeconds (0.0);
        auto* clip = host.createMidiClip (*track, 0.0, length);

        if (clip != nullptr && onStructureChanged)
            onStructureChanged();

        return clip;
    }

    void loadSampleForRow (int row)
    {
        juce::FileChooser chooser (tr ("Charger un son pour ") + rowName (row),
                                   juce::File::getSpecialLocation (juce::File::userMusicDirectory),
                                   "*.wav;*.aif;*.aiff", true);

        // On ne crée la piste/clip qu'après un choix effectif : annuler le
        // dialogue ne doit pas modifier le projet.
        if (! chooser.browseForFileToOpen())
            return;

        if (ensureClip() == nullptr)
            return;

        auto* track = findDrumTrack();
        if (track == nullptr)
            return;

        const auto error = host.loadSampleForNote (*track, pitches[row], chooser.getResult());

        if (error.isNotEmpty())
        {
            juce::AlertWindow::showMessageBoxAsync (juce::MessageBoxIconType::WarningIcon,
                                                    tr ("Sampler"), error);
            return;
        }

        sampleNames[(std::size_t) row] = host.getSampleNameForNote (*track, pitches[row]);
        audition (pitches[row]);
        repaint (0, headerHeight + row * rowHeight, gridX, rowHeight);
    }

    void audition (int pitch)
    {
        if (auto* track = findDrumTrack())
        {
            host.sendNoteOn (*track, pitch, 0.9f);
            pendingOffPitch = pitch;
        }
    }

    bool hasActiveSteps() const
    {
        for (const auto& row : pattern)
            for (bool on : row)
                if (on)
                    return true;

        return false;
    }

    void toggleStep (int row, int step)
    {
        auto* clip = ensureClip();
        if (clip == nullptr)
            return;

        // Le clip est la source de vérité : le motif en mémoire peut être périmé
        // si la piste a été supprimée puis recréée entre deux clics.
        const int pitch = pitches[row];
        const double beat = (double) step * 0.25;
        const auto notes = host.getNotes (*clip);
        int found = -1;

        for (int i = 0; i < (int) notes.size(); ++i)
        {
            if (notes[(std::size_t) i].pitch == pitch
                && std::abs (notes[(std::size_t) i].startBeat - beat) < 0.05)
            {
                found = i;
                break;
            }
        }

        if (found >= 0)
            host.removeNote (*clip, found);
        else
        {
            host.addNote (*clip, { beat, 0.25, pitch, 100 });
            audition (pitch);
        }

        pattern[(std::size_t) row][(std::size_t) step] = (found < 0);
        host.saveProject();
        repaint (padBounds (row, step).toNearestInt().expanded (3));
    }

    void clearAll()
    {
        auto* clip = findClip();
        if (clip == nullptr)
            return;

        while (! host.getNotes (*clip).empty())
            host.removeNote (*clip, 0);

        pattern = {};
        host.saveProject();
        repaint();
    }

    EngineHost& host;
    te::AudioTrack* targetTrack = nullptr;
    std::array<std::array<bool, steps>, rows> pattern {};
    std::array<juce::String, rows> sampleNames {};
    int currentStep = -1;
    int pendingOffPitch = -1;
    bool wasPlaying = false;
    juce::TextButton playButton { tr ("LECTURE") }, clearButton { tr ("EFFACER") }, closeButton { tr ("FERMER") };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (StepSequencerPanel)
};
} // namespace fdgpe
