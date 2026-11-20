#include "MainComponent.h"

#include <algorithm>

#include "FdLookAndFeel.h"
#include "Palette.h"
#include "PianoRollEditor.h"
#include "PluginBrowser.h"
#include "TempoDialog.h"
#include "Text.h"

namespace fdgpe
{
namespace
{
/** Le LookAndFeel copie les couleurs de la palette à la construction : il faut
    les réappliquer après chaque changement de thème, avant les repaints. */
void refreshLookAndFeel()
{
    if (auto* lf = dynamic_cast<FdLookAndFeel*> (&juce::LookAndFeel::getDefaultLookAndFeel()))
        lf->refreshPalette();
}

/** Petit panneau d'apparence : mode clair/sombre et couleur d'accent. Les
    couleurs sont appliquées à chaud sur `palette::theme` (thread UI). */
class ThemePanel final : public juce::Component
{
public:
    explicit ThemePanel (EngineHost& h) : host (h)
    {
        const auto saved = host.loadTheme();
        dark = saved.first;
        accent = saved.second;

        addAndMakeVisible (modeBox);
        modeBox.addItem (tr ("CLAIR"), 1);
        modeBox.addItem (tr ("SOMBRE"), 2);
        modeBox.setSelectedId (dark ? 2 : 1, juce::dontSendNotification);
        modeBox.onChange = [this]
        {
            dark = modeBox.getSelectedId() == 2;
            applyAndRefresh (true, true);
        };

        // Deux accents prédéfinis, sans curseurs R/V/B : jaune fané et vert clair.
        const juce::uint32 presets[] = { 0xffc99a2f, 0xff62bd78 };

        for (int i = 0; i < 2; ++i)
        {
            addAndMakeVisible (accentButtons[i]);
            accentButtons[i].setButtonText (tr (i == 0 ? "JAUNE FANÉ" : "VERT CLAIR"));
            accentButtons[i].setClickingTogglesState (true);
            accentButtons[i].setRadioGroupId (9001, juce::dontSendNotification);
            accentButtons[i].setToggleState (accent.getARGB() == presets[i], juce::dontSendNotification);
            const auto preset = presets[i];
            accentButtons[i].onClick = [this, preset]
            {
                accent = juce::Colour (preset);
                applyAndRefresh (true, true);
            };
        }

        addAndMakeVisible (accentCaption);
        accentCaption.setText (tr ("COULEUR D'ACCENT"), juce::dontSendNotification);
        accentCaption.setFont (juce::Font (juce::FontOptions (11.0f, juce::Font::bold)));
        accentCaption.setColour (juce::Label::textColourId, palette::textDim);

        addAndMakeVisible (closeButton);
        closeButton.onClick = [this]
        {
            // Le destructeur sauvegarde (aussi pour un clic hors du CallOutBox
            // ou Échap) : une seule écriture disque suffit.
            if (auto* box = findParentComponentOfClass<juce::CallOutBox>())
                box->dismiss();
        };

        // Le CallOutBox prend la taille de son contenu : sans setSize, rien ne s'affiche.
        // 200 px de haut laissent des cibles de 44 px sans chevauchement :
        // pastille 6-26, liste 28-72, légende 74-92, accents 96-140, FERMER 150-194.
        setSize (300, 200);
    }

    /** Clic hors du CallOutBox et Échap le ferment sans passer par `closeButton` :
        on sauvegarde donc aussi à la destruction (les changements sont appliqués
        immédiatement, il n'y a rien à annuler). */
    ~ThemePanel() override
    {
        host.saveTheme (dark, accent);
    }

    void paint (juce::Graphics& g) override
    {
        g.fillAll (palette::panel);
        g.setColour (palette::violet);
        g.drawRect (getLocalBounds(), 1);

        g.setColour (accent);
        g.fillRoundedRectangle (8.0f, 6.0f, (float) getWidth() - 16.0f, 20.0f, 4.0f);
        g.setColour (palette::violet);
        g.drawRoundedRectangle (8.0f, 6.0f, (float) getWidth() - 16.0f, 20.0f, 4.0f, 1.5f);
    }

    void resized() override
    {
        modeBox.setBounds (8, 28, getWidth() - 16, 44);
        accentCaption.setBounds (8, 74, getWidth() - 16, 18);
        accentButtons[0].setBounds (8, 96, 138, 44);
        accentButtons[1].setBounds (154, 96, 138, 44);
        closeButton.setBounds (getWidth() - 104, getHeight() - 50, 96, 44);
    }

private:
    static void refreshTree (juce::Component& c)
    {
        // Les composants qui ont appelé setColour() gardent une copie de
        // l'ancien thème. On retire ces surcharges : ils retombent sur le
        // LookAndFeel, qui vient d'être rafraîchi avec la nouvelle palette.
        // On ne retire que les identifiants que FdLookAndFeel définit : les
        // surcharges propres aux composants (témoin LED, couleur de piste,
        // touches du clavier) doivent rester telles quelles.
        static const int colourIds[] = {
            juce::Label::textColourId, juce::Label::textWhenEditingColourId,
            juce::Label::backgroundColourId, juce::Label::backgroundWhenEditingColourId,
            juce::TextEditor::backgroundColourId, juce::TextEditor::textColourId,
            juce::TextEditor::outlineColourId, juce::TextEditor::focusedOutlineColourId,
            juce::ComboBox::backgroundColourId, juce::ComboBox::textColourId,
            juce::ComboBox::outlineColourId
        };

        for (int id : colourIds)
            if (c.isColourSpecified (id))
                c.removeColour (id);

        c.repaint();

        for (int i = 0; i < c.getNumChildComponents(); ++i)
            if (auto* child = c.getChildComponent (i))
                refreshTree (*child);
    }

    void applyAndRefresh (bool save, bool fullRefresh)
    {
        palette::theme.apply (dark, accent);
        refreshLookAndFeel();

        if (save)
            host.saveTheme (dark, accent);

        if (fullRefresh)
        {
            if (auto* top = getTopLevelComponent())
                refreshTree (*top);

            accentCaption.setColour (juce::Label::textColourId, palette::textDim);
        }
        else
        {
            repaint();
        }
    }

    EngineHost& host;
    bool dark = false;
    juce::Colour accent { palette::blue };
    juce::ComboBox modeBox;
    juce::Label accentCaption;
    juce::TextButton accentButtons[2];
    juce::TextButton closeButton { tr ("FERMER") };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ThemePanel)
};

int noteForKeyCode (int keyCode) noexcept
{
    switch (keyCode)
    {
        case 'A': return 60; // C4
        case 'W': return 61;
        case 'S': return 62;
        case 'E': return 63;
        case 'D': return 64;
        case 'F': return 65;
        case 'T': return 66;
        case 'G': return 67;
        case 'Y': return 68;
        case 'H': return 69;
        case 'U': return 70;
        case 'J': return 71;
        case 'K': return 72; // C5
        default: return -1;
    }
}

void clearClipSelection (const std::vector<std::unique_ptr<TrackRow>>& rows)
{
    for (auto& row : rows)
        row->selectClip (nullptr);
}

te::Clip* getSelectedClip (const std::vector<std::unique_ptr<TrackRow>>& rows)
{
    for (auto& row : rows)
        if (auto* clip = row->getSelectedClip())
            return clip;

    return nullptr;
}

/** Petit mixeur modeless : une colonne par piste avec volume, panoramique et MUTE. */
class MixerPanel final : public juce::Component
{
public:
    explicit MixerPanel (EngineHost& h) : host (h)
    {
        for (auto* track : host.getTracks())
        {
            auto channel = std::make_unique<Channel>();
            channel->track = track;
            channel->name.setText (track->getName(), juce::dontSendNotification);
            channel->name.setColour (juce::Label::textColourId, palette::text);
            channel->name.setFont (juce::Font (juce::FontOptions (12.0f, juce::Font::bold)));
            channel->name.setJustificationType (juce::Justification::centred);

            channel->volume.setSliderStyle (juce::Slider::LinearVertical);
            channel->volume.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
            channel->volume.setRange (0.0, 1.0, 0.0);
            channel->volume.setDoubleClickReturnValue (true, 0.8);

            channel->pan.setSliderStyle (juce::Slider::LinearHorizontal);
            channel->pan.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
            channel->pan.setRange (-1.0, 1.0, 0.0);
            channel->pan.setDoubleClickReturnValue (true, 0.0);

            FdLookAndFeel::setLed (channel->mute, palette::amber);
            channel->mute.setToggleState (track->isMuted (false), juce::dontSendNotification);

            if (auto* vp = track->getVolumePlugin())
            {
                channel->volume.setValue (vp->getSliderPos(), juce::dontSendNotification);
                channel->pan.setValue (vp->getPan(), juce::dontSendNotification);
            }

            Channel* raw = channel.get();

            channel->volume.onValueChange = [this, raw]
            {
                if (exists (raw->track))
                    if (auto* vp = raw->track->getVolumePlugin())
                        vp->setSliderPos ((float) raw->volume.getValue());
            };

            channel->pan.onValueChange = [this, raw]
            {
                if (exists (raw->track))
                    if (auto* vp = raw->track->getVolumePlugin())
                        vp->setPan ((float) raw->pan.getValue());
            };

            channel->mute.onClick = [this, raw]
            {
                if (exists (raw->track))
                    raw->track->setMute (! raw->track->isMuted (false));
            };

            addAndMakeVisible (channel->name);
            addAndMakeVisible (channel->volume);
            addAndMakeVisible (channel->pan);
            addAndMakeVisible (channel->mute);
            channels.push_back (std::move (channel));
        }

        setSize (juce::jmax (360, 32 + (int) channels.size() * 132), 320);
    }

    void paint (juce::Graphics& g) override
    {
        g.fillAll (palette::panel);
        g.setColour (palette::blue);
        g.drawHorizontalLine (0, 0.0f, (float) getWidth());

        if (channels.empty())
        {
            g.setColour (palette::textDim);
            g.setFont (juce::Font (juce::FontOptions (13.0f)));
            g.drawText (tr ("Aucune piste."), getLocalBounds(), juce::Justification::centred);
        }
    }

    void resized() override
    {
        int x = 16;

        for (auto& channel : channels)
        {
            channel->name.setBounds (x, 14, 116, 22);
            channel->volume.setBounds (x + 12, 48, 30, 180);
            channel->pan.setBounds (x + 52, 234, 64, 24);
            channel->mute.setBounds (x + 52, 266, 64, 26);
            x += 132;
        }
    }

private:
    struct Channel
    {
        te::AudioTrack* track = nullptr;
        juce::Label name;
        juce::Slider volume, pan;
        juce::TextButton mute { "M" };
    };

    bool exists (te::AudioTrack* track) const
    {
        const auto tracks = host.getTracks();
        return std::find (tracks.begin(), tracks.end(), track) != tracks.end();
    }

    EngineHost& host;
    std::vector<std::unique_ptr<Channel>> channels;
};
} // namespace

MainComponent::MainComponent()
    : themeReady ([this]
      {
          const auto saved = host.loadTheme();
          palette::theme.apply (saved.first, saved.second);
          return 0;
      }()),
      transport (host, {
          [this] { startRecording(); },
          [this] { showAudioSettings(); },
          [this] { PluginBrowser::show (host, getFocusedTrack()); },
          [this] { openPianoRoll(); },
          [this] { TempoDialog::show (host); },
          [this] { exportProject(); },
          [this] { showProjectMenu(); },
          [this] { addMarker(); },
          [this] { host.goToMarker (true); seek (host.getPositionSeconds()); },
          [this] { host.goToMarker (false); seek (host.getPositionSeconds()); },
          [this] { showMixer(); },
          [this] { cycleSnap(); }
      }),
      ruler (host, model, [this] (double s) { seek (s); }),
      overlay (model)
{
    // Le thème est déjà appliqué par `themeReady`, avant la construction des
    // composants ; on synchronise seulement le LookAndFeel avant la 1re peinture.
    refreshLookAndFeel();

    model.changed = [this]
    {
        // Un défilement/zoom manuel prend la main sur le suivi automatique.
        followPlayhead = false;
        viewChanged();
    };
    transport.setSnapLabel ("1/4", true);

    addAndMakeVisible (transport);
    addAndMakeVisible (ruler);

    viewport.setViewedComponent (&rowsContainer, false);
    viewport.setScrollBarsShown (true, false);
    viewport.setScrollBarThickness (10);
    addAndMakeVisible (viewport);

    addTrackButton.setTooltip (tr ("Ajouter une piste (audio ou MIDI)"));
    FdLookAndFeel::setFontSize (addTrackButton, 22.0f);
    projectButton.setButtonText (tr ("PROJET…"));
    projectButton.onClick = [this] { showProjectMenu(); };

    addTrackButton.onClick = [this]
    {
        juce::PopupMenu menu;
        menu.addItem (1, tr ("Piste audio"));
        menu.addItem (2, tr ("Piste MIDI"));

        menu.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (addTrackButton),
                            [safe = juce::Component::SafePointer<MainComponent> (this)] (int result)
                            {
                                if (safe == nullptr)
                                    return;

                                if (result == 1)
                                    safe->createAudioTrack();
                                else if (result == 2)
                                    safe->createMidiTrack();
                            });
    };

    addAndMakeVisible (addTrackButton);
    addAndMakeVisible (projectButton);

    addAndMakeVisible (overlay);

    setWantsKeyboardFocus (true);
    setSize (1120, 700);

    rebuildRows();
    transport.setStatus (tr ("Prêt. Espace = lecture, A-K = notes, P = plugins, R = piano roll, T = tempo."));
    startTimerHz (20);
}

MainComponent::~MainComponent()
{
    stopTimer();
    releaseHeldKeys();
    keyboardWindow.reset();
    audioSettingsWindow.reset();
    rows.clear();
}

//==============================================================================
void MainComponent::paint (juce::Graphics& g)
{
    auto r = getLocalBounds().toFloat();

    g.setGradientFill (juce::ColourGradient (palette::bgTop, 0.0f, 0.0f, palette::bgBottom, 0.0f, r.getBottom(), false));
    g.fillRect (r);

    // zone "PISTES" à gauche de la règle
    auto titleArea = juce::Rectangle<int> (1, ruler.getY(), TrackRow::headerWidth, ruler.getHeight());
    g.setGradientFill (juce::ColourGradient (palette::panelHi, 0.0f, (float) titleArea.getY(),
                                             palette::panel, 0.0f, (float) titleArea.getBottom(), false));
    g.fillRect (titleArea);
    g.setColour (palette::textDim);
    g.setFont (juce::Font (juce::FontOptions (11.5f, juce::Font::bold)));
    g.drawText ("PISTES", titleArea.reduced (12, 0), juce::Justification::centredLeft);
    g.setColour (palette::blueSoft.withAlpha (0.55f));
    g.drawHorizontalLine (titleArea.getBottom() - 1, (float) titleArea.getX(), (float) titleArea.getRight());

    // cadre bleu autour de la fenêtre
    g.setColour (palette::blueSoft);
    g.drawRect (getLocalBounds(), 1);
}

void MainComponent::resized()
{
    auto area = getLocalBounds().reduced (1);

    transport.setBounds (area.removeFromTop (108));

    auto addBar = area.removeFromBottom (52);
    addTrackButton.setBounds (addBar.removeFromLeft (68).reduced (8, 4));
    projectButton.setBounds (addBar.removeFromLeft (130).reduced (8, 4));

    auto rulerRow = area.removeFromTop (30);
    rulerRow.removeFromLeft (TrackRow::headerWidth);
    ruler.setBounds (rulerRow);

    viewport.setBounds (area);

    overlay.setBounds (ruler.getX(), ruler.getY(), area.getRight() - ruler.getX(), area.getBottom() - ruler.getY());
    overlay.toFront (false);

    if (stepPanel != nullptr && stepPanel->isVisible())
    {
        const int w = juce::jmax (240, juce::jmin (480, viewport.getWidth() - 8));
        stepPanel->setBounds (viewport.getRight() - w - 8, viewport.getBottom() - 248, w, 240);

        // L'overlay vient d'être remis au premier plan : le panneau doit rester
        // au-dessus, sinon ses pads sont invisibles et leurs clics interceptés.
        stepPanel->toFront (false);
    }

    layoutRows();
}

//==============================================================================
bool MainComponent::keyPressed (const juce::KeyPress& key)
{
    const auto mods = key.getModifiers();
    const int code = key.getKeyCode();
    // Sous Linux, JUCE renvoie les lettres en minuscules ('z'=122) alors que
    // les raccourcis sont écrits en majuscules ('Z'=90). On normalise uniquement
    // pour les comparaisons de lettres ; `code` reste la touche physique.
    const int letterCode = (int) juce::CharacterFunctions::toUpperCase ((juce::juce_wchar) code);

    if (key == juce::KeyPress::spaceKey)
    {
        host.togglePlay();
        transport.refresh();
        return true;
    }

    if (mods.isCommandDown())
    {
        switch (letterCode)
        {
            case 'S':
                if (mods.isShiftDown())
                    saveProjectAs();
                else
                {
                    host.saveProject();
                    transport.setStatus (tr ("Projet enregistré."));
                }
                return true;
            case 'N':
                newProject();
                return true;
            case 'O':
                openProject();
                return true;
            case 'I':
                importAudio();
                return true;
            case 'D':
                duplicateTrack();
                return true;
            case 'C':
                if (auto* clip = getSelectedClip (rows))
                {
                    host.copyClip (*clip);
                    transport.setStatus (tr ("Clip copié."));
                }
                return true;
            case 'V':
                if (auto* track = getFocusedTrack())
                {
                    clearClipSelection (rows);

                    if (host.pasteClip (*track, host.getPositionSeconds()))
                    {
                        host.saveProject();
                        viewChanged();
                        transport.setStatus (tr ("Clip collé à la tête de lecture."));
                    }
                }
                return true;
            case 'Z':
                if (mods.isShiftDown()) host.redo();
                else                   host.undo();
                if (inlinePianoRoll != nullptr)
                    inlinePianoRoll->refreshAll();
                transport.refresh();
                refreshRows();
                return true;
            case 'Y':
                host.redo();
                if (inlinePianoRoll != nullptr)
                    inlinePianoRoll->refreshAll();
                transport.refresh();
                refreshRows();
                return true;
            case 'E':
                exportProject();
                return true;
            default:
                break;
        }
    }

    if (! mods.isCommandDown() && ! mods.isAltDown() && ! mods.isCtrlDown())
    {
        if (key == juce::KeyPress::deleteKey || key == juce::KeyPress::backspaceKey)
        {
            if (auto* clip = getSelectedClip (rows))
            {
                clearClipSelection (rows);
                host.deleteClip (*clip);
                host.saveProject();
                viewChanged();
                transport.setStatus (tr ("Clip supprimé."));
                return true;
            }
        }

        switch (letterCode)
        {
            case 'S':
                if (auto* clip = getSelectedClip (rows))
                {
                    clearClipSelection (rows);
                    host.splitClip (*clip, host.getPositionSeconds());
                    host.saveProject();
                    viewChanged();
                    transport.setStatus (tr ("Clip coupé à la tête de lecture."));
                    return true;
                }
                break;
            case 'P':
                PluginBrowser::show (host, getFocusedTrack());
                return true;
            case 'R':
                openPianoRoll();
                return true;
            case 'T':
                TempoDialog::show (host);
                return true;
            case 'M':
                addMarker();
                return true;
            case 'N':
                createMidiTrack();
                return true;
            default:
                break;
        }

        if (! mods.isShiftDown() && noteForKeyCode (letterCode) >= 0)
        {
            handleNoteKey (key, true);
            return true;
        }
    }

    return false;
}

bool MainComponent::keyStateChanged (bool isKeyDown)
{
    if (! isKeyDown)
        releaseHeldKeys();

    return false;
}

void MainComponent::handleNoteKey (const juce::KeyPress& key, bool down)
{
    const int note = noteForKeyCode (key.getKeyCode());
    if (note < 0)
        return;

    const int code = key.getKeyCode();

    if (down)
    {
        if (std::find (activeNoteKeys.begin(), activeNoteKeys.end(), code) == activeNoteKeys.end())
        {
            activeNoteKeys.push_back (code);

            if (auto* track = getFocusedMidiTrack())
            {
                activeNoteTracks[code] = track;
                host.sendNoteOn (*track, note, 0.85f);
            }
            else
            {
                host.sendNoteOn (note, 0.85f);
            }
        }
    }
    else
    {
        // Le note-off part vers la piste qui a reçu le note-on, même si le
        // focus a changé pendant que la touche était enfoncée.
        auto it = activeNoteTracks.find (code);

        if (it != activeNoteTracks.end())
        {
            if (trackStillExists (it->second))
                host.sendNoteOff (*it->second, note);
            else
                host.sendNoteOff (note);

            activeNoteTracks.erase (it);
        }
        else
        {
            host.sendNoteOff (note);
        }

        activeNoteKeys.erase (std::remove (activeNoteKeys.begin(), activeNoteKeys.end(), code), activeNoteKeys.end());
    }
}

bool MainComponent::trackStillExists (te::AudioTrack* track)
{
    if (track == nullptr)
        return false;

    const auto tracks = host.getTracks();
    return std::find (tracks.begin(), tracks.end(), track) != tracks.end();
}

void MainComponent::releaseHeldKeys()
{
    // Ne coupe que les touches réellement relâchées : sinon une note tenue serait
    // interrompue dès la prochaine tournée du timer.
    for (auto it = activeNoteKeys.begin(); it != activeNoteKeys.end(); )
    {
        if (! juce::KeyPress::isKeyCurrentlyDown (*it))
        {
            auto target = activeNoteTracks.find (*it);

            if (target != activeNoteTracks.end())
            {
                if (trackStillExists (target->second))
                    host.sendNoteOff (*target->second, noteForKeyCode (*it));
                else
                    host.sendNoteOff (noteForKeyCode (*it));

                activeNoteTracks.erase (target);
            }
            else
            {
                host.sendNoteOff (noteForKeyCode (*it));
            }

            it = activeNoteKeys.erase (it);
        }
        else
        {
            ++it;
        }
    }
}

//==============================================================================
void MainComponent::closeTrackEditors()
{
    // Ces fenêtres gardent une référence sur une piste ou un clip : elles doivent
    // disparaître avant toute reconstruction ou suppression de piste.
    keyboardWindow.reset();
    inlinePianoRoll.reset();
}

void MainComponent::rebuildRows()
{
    // Les lignes sont détruites AVANT toute suppression de piste (leurs vu-mètres
    // sont branchés sur les pistes).
    closeTrackEditors();
    rows.clear();

    TrackRow::Callbacks callbacks;
    callbacks.seek          = [this] (double s) { seek (s); };
    callbacks.remove        = [this] (te::AudioTrack& t) { removeTrack (t); };
    callbacks.changed       = [this] { refreshRows(); };
    callbacks.addPlugin     = [this] (te::AudioTrack& t) { PluginBrowser::show (host, &t); };
    callbacks.pluginEditor  = [this] (te::Plugin& p) { showPluginEditor (p); };
    callbacks.openPianoRoll = [this] (te::MidiClip& c) { openPianoRoll (c); };
    callbacks.openKeyboard  = [this] (te::AudioTrack& t) { openKeyboard (t); };

    callbacks.splitClip = [this] (te::Clip& clip)
    {
        clearClipSelection (rows);
        host.splitClip (clip, host.getPositionSeconds());
        host.saveProject();
        viewChanged();
        transport.setStatus (tr ("Clip coupé à la tête de lecture."));
    };

    callbacks.duplicateClip = [this] (te::Clip& clip)
    {
        clearClipSelection (rows);
        host.duplicateClip (clip);
        host.saveProject();
        viewChanged();
        transport.setStatus (tr ("Clip dupliqué."));
    };

    callbacks.deleteClip = [this] (te::Clip& clip)
    {
        clearClipSelection (rows);
        host.deleteClip (clip);
        host.saveProject();
        viewChanged();
        transport.setStatus (tr ("Clip supprimé."));
    };

    callbacks.selected = [this] (te::AudioTrack&, te::Clip* clip)
    {
        // Une seule sélection dans tout le projet.
        for (auto& row : rows)
            if (row->getSelectedClip() != clip)
                row->selectClip (nullptr);
    };

    callbacks.duplicateTrack = [this] (te::AudioTrack& track) { duplicateTrack (&track); };
    callbacks.importAudio   = [this] (te::AudioTrack& track) { importAudio (&track); };
    callbacks.projectChanged = [this] { viewChanged(); refreshRows(); };

    callbacks.addMidiClip = [this] (te::AudioTrack& track, double seconds)
    {
        const auto sig = host.getTimeSignature();
        const double beatsPerBar = (double) juce::jmax (1, sig.first) * 4.0 / (double) juce::jmax (1, sig.second);
        const double secondsPerBar = beatsPerBar * 60.0 / juce::jmax (1.0, host.getTempoBpm());

        if (auto* clip = host.createMidiClip (track, seconds, secondsPerBar * 4.0))
        {
            host.saveProject();
            viewChanged();
            transport.setStatus (tr ("Clip MIDI créé. Double-clic pour l'éditer."));
            juce::ignoreUnused (clip);
        }
    };

    for (auto* track : host.getTracks())
    {
        auto row = std::make_unique<TrackRow> (host, *track, model, callbacks);
        rowsContainer.addAndMakeVisible (*row);
        rows.push_back (std::move (row));
    }

    layoutRows();
}

void MainComponent::layoutRows()
{
    const int width = juce::jmax (100, viewport.getMaximumVisibleWidth());

    rowsContainer.setSize (width, (int) rows.size() * TrackRow::rowHeight);

    int y = 0;

    for (auto& row : rows)
    {
        row->setBounds (0, y, width, TrackRow::rowHeight);
        y += TrackRow::rowHeight;
    }
}

void MainComponent::refreshRows()
{
    for (auto& row : rows)
        row->refreshControls();
}

void MainComponent::viewChanged()
{
    ruler.repaint();

    for (auto& row : rows)
        row->repaintLane();

    overlay.refreshAll();
}

te::AudioTrack* MainComponent::getFocusedTrack()
{
    for (auto& row : rows)
        if (row->getSelectedClip() != nullptr)
            return &row->getTrack();

    if (! rows.empty())
        return &rows.front()->getTrack();

    return nullptr;
}

te::AudioTrack* MainComponent::getFocusedMidiTrack()
{
    // Priorité à la piste MIDI dont un clip est sélectionné, puis à la première
    // piste MIDI : c'est la cible des notes jouées au clavier.
    for (auto& row : rows)
        if (host.isMidiTrack (row->getTrack()) && row->getSelectedClip() != nullptr)
            return &row->getTrack();

    for (auto& row : rows)
        if (host.isMidiTrack (row->getTrack()))
            return &row->getTrack();

    return nullptr;
}

te::MidiClip* MainComponent::getFocusedMidiClip()
{
    for (auto& row : rows)
        if (auto* clip = dynamic_cast<te::MidiClip*> (row->getSelectedClip()))
            return clip;

    for (auto& row : rows)
    {
        auto& track = row->getTrack();

        if (! host.isMidiTrack (track))
            continue;

        const auto clips = host.getMidiClips (track);

        if (! clips.empty())
            return clips.front();
    }

    return nullptr;
}

void MainComponent::seek (double seconds)
{
    host.setPositionSeconds (seconds);
    followPlayhead = true;

    // Si la cible est hors écran, on recentre la vue dessus : cela permet
    // de revenir en arrière après que la tête a dépassé la zone visible.
    const double visible = (double) juce::jmax (1, ruler.getWidth()) / model.pixelsPerSecond;

    if (seconds < model.startSeconds || seconds > model.startSeconds + visible)
        model.startSeconds = juce::jmax (0.0, seconds - visible * 0.25);

    viewChanged();
    overlay.setPosition (host.getPositionSeconds());
    transport.refresh();
}

void MainComponent::startRecording()
{
    followPlayhead = true;

    if (! host.anyTrackArmed())
    {
        juce::AlertWindow::showMessageBoxAsync (
            juce::MessageBoxIconType::InfoIcon,
            tr ("Aucune piste armée"),
            tr ("Pour enregistrer, appuie d'abord sur ARM sur la piste voulue :\n"
                "piste audio = entrée du micro ou de la carte son, piste MIDI = clavier à l'écran ou clavier MIDI."));
        return;
    }

    host.toggleRecord();
    transport.setStatus (tr ("Enregistrement en cours..."));
}

void MainComponent::createAudioTrack()
{
    host.addAudioTrack();
    host.saveProject();
    rebuildRows();
    transport.setStatus (tr ("Piste audio ajoutée."));
}

void MainComponent::createMidiTrack()
{
    host.addMidiTrack();
    host.saveProject();
    rebuildRows();
    transport.setStatus (tr ("Piste MIDI ajoutée."));
}

void MainComponent::removeTrack (te::AudioTrack& track)
{
    auto* trackPtr = &track;

    // Suppression différée : on ne détruit pas une ligne depuis son propre bouton.
    juce::MessageManager::callAsync ([this, safe = juce::Component::SafePointer<MainComponent> (this), trackPtr]
    {
        if (safe == nullptr)
            return;

        const auto tracks = host.getTracks();

        if (std::find (tracks.begin(), tracks.end(), trackPtr) == tracks.end())
            return;

        closeTrackEditors();
        rows.clear();                  // d'abord les vu-mètres
        host.deleteTrack (*trackPtr);
        host.saveProject();
        rebuildRows();
        transport.setStatus (tr ("Piste supprimée."));
    });
}

void MainComponent::showAudioSettings()
{
    if (audioSettingsWindow == nullptr)
    {
        juce::DialogWindow::LaunchOptions options;
        options.dialogTitle = tr ("Réglages audio");
        options.dialogBackgroundColour = palette::panel;
        options.escapeKeyTriggersCloseButton = true;
        options.useNativeTitleBar = true;
        options.resizable = false;

        auto* selector = new juce::AudioDeviceSelectorComponent (
            host.getEngine().getDeviceManager().deviceManager,
            0, 512, 1, 512, false, false, true, true);
        selector->setSize (480, 560);
        options.content.setOwned (selector);

        audioSettingsWindow.reset (options.create());
    }

    audioSettingsWindow->setVisible (true);
    audioSettingsWindow->toFront (true);
}

void MainComponent::showMixer()
{
    juce::DialogWindow::LaunchOptions options;
    options.dialogTitle = tr ("Mixeur");
    options.dialogBackgroundColour = palette::panel;
    options.escapeKeyTriggersCloseButton = true;
    options.useNativeTitleBar = true;
    options.resizable = true;

    auto* mixer = new MixerPanel (host);
    options.content.setOwned (mixer);

    auto* window = options.launchAsync();
    const int width = mixer->getWidth();
    const int height = mixer->getHeight();
    window->setSize (width + 16, height + 44);
    window->centreWithSize (width + 16, height + 44);
}

void MainComponent::cycleSnap()
{
    static const double values[] = { 0.0, 0.25, 0.5, 1.0, 2.0, 4.0 };
    static const char* labels[] = { "OFF", "1/16", "1/8", "1/4", "1/2", "1 MESURE" };

    int index = 0;

    for (int i = 0; i < 6; ++i)
        if (juce::exactlyEqual (model.snapBeats, values[i]))
            index = i;

    index = (index + 1) % 6;
    model.snapBeats = values[index];
    transport.setSnapLabel (tr (labels[index]), values[index] > 0.0);
    transport.setStatus (tr ("Magnétisme : ") + tr (labels[index]));
}

//==============================================================================
void MainComponent::showProjectMenu()
{
    juce::PopupMenu menu;
    menu.addItem (1, tr ("Nouveau projet"));
    menu.addItem (2, tr ("Ouvrir un projet…"));
    menu.addItem (3, tr ("Enregistrer sous…"));
    menu.addSeparator();
    menu.addItem (4, tr ("Importer un fichier audio…"));
    menu.addItem (5, tr ("Dupliquer la piste sélectionnée"));
    menu.addSeparator();
    menu.addItem (6, tr ("Exporter le mixage en WAV…"));
    menu.addItem (7, tr ("Exporter avec options…"));
    menu.addSeparator();
    menu.addItem (8, tr ("Séquenceur de pas…"));
    menu.addSeparator();
    menu.addItem (9, tr ("Apparence…"));

    switch (menu.showAt (&projectButton, 0, projectButton.getHeight()))
    {
        case 1: newProject();          break;
        case 2: openProject();         break;
        case 3: saveProjectAs();       break;
        case 4: importAudio();         break;
        case 5: duplicateTrack();      break;
        case 6: exportProject();       break;
        case 7: exportProjectAdvanced(); break;
        case 8: showStepSequencer();     break;
        case 9: showThemePanel();        break;
        default: break;
    }
}

void MainComponent::newProject()
{
    closeTrackEditors();
    rows.clear();
    releaseHeldKeys();

    if (! host.newProject())
    {
        rebuildRows();
        juce::AlertWindow::showMessageBoxAsync (juce::MessageBoxIconType::WarningIcon,
                                                tr ("Nouveau projet"), tr ("Impossible de créer un nouveau projet."));
        return;
    }

    rebuildRows();
    model.startSeconds = 0.0;
    viewChanged();
    transport.refresh();
    transport.setStatus (tr ("Nouveau projet créé."));
}

void MainComponent::openProject()
{
    juce::FileChooser chooser (tr ("Ouvrir un projet FD gPE"),
                               juce::File::getSpecialLocation (juce::File::userDocumentsDirectory)
                                   .getChildFile ("FD-gPE"),
                               "*.tracktionedit",
                               true);

    if (! chooser.browseForFileToOpen())
        return;

    const auto file = chooser.getResult();
    closeTrackEditors();
    rows.clear();
    releaseHeldKeys();

    if (! host.openProject (file))
    {
        rebuildRows();
        juce::AlertWindow::showMessageBoxAsync (juce::MessageBoxIconType::WarningIcon,
                                                tr ("Ouvrir un projet"), tr ("Impossible d'ouvrir ce projet."));
        return;
    }

    rebuildRows();
    model.startSeconds = 0.0;
    viewChanged();
    transport.refresh();
    transport.setStatus (tr ("Projet ouvert : ") + host.getProjectName());
}

void MainComponent::saveProjectAs()
{
    juce::FileChooser chooser (tr ("Enregistrer le projet sous…"),
                               host.getProjectFile(),
                               "*.tracktionedit",
                               true);

    if (! chooser.browseForFileToSave (true))
        return;

    const auto file = chooser.getResult().withFileExtension ("tracktionedit");
    file.getParentDirectory().createDirectory();

    if (host.saveProjectAs (file))
        transport.setStatus (tr ("Projet enregistré sous : ") + file.getFileName());
    else
        juce::AlertWindow::showMessageBoxAsync (juce::MessageBoxIconType::WarningIcon,
                                                tr ("Enregistrer sous"), tr ("Impossible d'enregistrer le projet."));
}

void MainComponent::importAudio (te::AudioTrack* track)
{
    if (track == nullptr)
        track = getFocusedTrack();

    if (track == nullptr)
    {
        juce::AlertWindow::showMessageBoxAsync (juce::MessageBoxIconType::InfoIcon,
                                                tr ("Importer un audio"), tr ("Ajoutez d'abord une piste audio."));
        return;
    }

    juce::FileChooser chooser (tr ("Importer un fichier audio"),
                               juce::File::getSpecialLocation (juce::File::userHomeDirectory),
                               "*.wav;*.aif;*.aiff;*.flac;*.ogg;*.mp3",
                               true);

    if (! chooser.browseForFileToOpen())
        return;

    host.importAudioFile (*track, chooser.getResult(), host.getPositionSeconds());
    viewChanged();
    transport.setStatus (tr ("Audio importé sur ") + track->getName());
}

void MainComponent::duplicateTrack (te::AudioTrack* track)
{
    if (track == nullptr)
        track = getFocusedTrack();

    if (track == nullptr)
        return;

    closeTrackEditors();
    rows.clear();
    const auto* copy = host.duplicateTrack (*track);
    rebuildRows();
    transport.setStatus (copy != nullptr ? tr ("Piste dupliquée.")
                                         : tr ("Impossible de dupliquer cette piste."));
}

//==============================================================================
void MainComponent::openPianoRoll()
{
    if (auto* clip = getFocusedMidiClip())
    {
        openPianoRoll (*clip);
        return;
    }

    te::AudioTrack* midiTrack = nullptr;

    for (auto& row : rows)
    {
        if (host.isMidiTrack (row->getTrack()))
        {
            midiTrack = &row->getTrack();
            break;
        }
    }

    if (midiTrack == nullptr)
    {
        juce::AlertWindow::showMessageBoxAsync (
            juce::MessageBoxIconType::InfoIcon,
            tr ("Aucune piste MIDI"),
            tr ("Ajoutez d'abord une piste MIDI avec le bouton « + »."));
        return;
    }

    const auto sig = host.getTimeSignature();
    const double beatsPerBar = (double) juce::jmax (1, sig.first) * 4.0 / (double) juce::jmax (1, sig.second);
    const double secondsPerBar = beatsPerBar * 60.0 / juce::jmax (1.0, host.getTempoBpm());
    auto* clip = host.createMidiClip (*midiTrack, host.getPositionSeconds(), secondsPerBar * 4.0);

    if (clip == nullptr)
    {
        juce::AlertWindow::showMessageBoxAsync (
            juce::MessageBoxIconType::WarningIcon,
            tr ("Création impossible"),
            tr ("Impossible de créer un clip MIDI à cette position."));
        return;
    }

    host.saveProject();
    viewChanged();
    openPianoRoll (*clip);
}

void MainComponent::openKeyboard (te::AudioTrack& track)
{
    keyboardWindow.reset();

    juce::DialogWindow::LaunchOptions options;
    options.dialogTitle = tr ("Clavier — ") + track.getName();
    options.dialogBackgroundColour = palette::panel;
    options.escapeKeyTriggersCloseButton = true;
    options.useNativeTitleBar = true;
    options.resizable = false;
    options.componentToCentreAround = this;
    options.content.setOwned (new MidiKeyboardPanel (host, track));

    // create() et non launchAsync() : launchAsync() passe la fenêtre en mode
    // modal (enterModalState), ce qui bloque le transport principal et rend
    // dangereux le reset() lorsqu'on rouvre le clavier.
    auto* window = options.create();
    window->setVisible (true);
    window->toFront (false);
    keyboardWindow.reset (window);
    transport.setStatus (tr ("Clavier : cliquez sur les touches pour jouer la piste."));
}

void MainComponent::openPianoRoll (te::MidiClip& clip)
{
    // Éditeur intégré, placé au niveau de la piste MIDI : plus de fenêtre
    // séparée « en bas ». Une seule instance à la fois.
    inlinePianoRoll.reset();

    auto editor = std::make_unique<PianoRollEditor> (host, clip);
    // On diffère la destruction : le bouton « Fermer » vit dans l'éditeur,
    // détruire ce dernier depuis son propre onClick serait un use-after-free.
    editor->onClose = [this,
                       safeMain  = juce::Component::SafePointer<MainComponent> (this),
                       safeEditor = juce::Component::SafePointer<PianoRollEditor> (editor.get())]
    {
        juce::MessageManager::callAsync ([safeMain, safeEditor]
        {
            if (safeMain == nullptr || safeEditor == nullptr)
                return;

            // Une fermeture en attente ne doit pas fermer un piano roll ouvert
            // entre-temps, ni toucher à un éditeur déjà détruit.
            if (safeEditor.getComponent() == safeMain->inlinePianoRoll.get())
                safeMain->inlinePianoRoll.reset();
        });
    };

    int y = viewport.getY() + 8;

    for (std::size_t i = 0; i < rows.size(); ++i)
    {
        bool found = false;

        for (auto* c : host.getMidiClips (rows[i]->getTrack()))
            if (c == &clip)
                found = true;

        if (found)
        {
            y = viewport.getY() + (int) i * TrackRow::rowHeight;
            break;
        }
    }

    const int h = juce::jmin (640, juce::jmax (240, viewport.getHeight() - 8));
    const int w = juce::jmin (900, juce::jmax (360, getWidth() - 24));

    editor->setBounds (juce::jmax (0, getWidth() - w - 12),
                       juce::jlimit (viewport.getY(), juce::jmax (viewport.getY(), viewport.getBottom() - h), y),
                       w, h);

    inlinePianoRoll = std::move (editor);
    addAndMakeVisible (inlinePianoRoll.get());
    transport.setStatus (tr ("Piano roll : double-clic pour créer une note, glisser pour déplacer, clic droit pour supprimer."));
}

void MainComponent::showPluginEditor (te::Plugin& plugin)
{
    auto editor = plugin.createEditor();

    if (editor == nullptr)
    {
        juce::AlertWindow::showMessageBoxAsync (
            juce::MessageBoxIconType::InfoIcon,
            tr ("Plugin"),
            tr ("Ce plugin n'a pas d'interface graphique."));
        return;
    }

    juce::DialogWindow::LaunchOptions options;
    options.dialogTitle = EngineHost::getPluginDisplayName (plugin);
    options.dialogBackgroundColour = palette::panel;
    options.escapeKeyTriggersCloseButton = true;
    options.useNativeTitleBar = true;
    options.resizable = true;

    const int w = juce::jlimit (420, 1400, editor->getWidth() + 24);
    const int h = juce::jlimit (260, 1000, editor->getHeight() + 48);
    options.content.setOwned (editor.release());

    auto* window = options.launchAsync();
    window->setSize (w, h);
    window->centreWithSize (w, h);
}

void MainComponent::showThemePanel()
{
    juce::CallOutBox::launchAsynchronously (std::make_unique<ThemePanel> (host),
                                            projectButton.getBounds(), this);
}

void MainComponent::showStepSequencer()
{
    if (stepPanel == nullptr)
    {
        stepPanel = std::make_unique<StepSequencerPanel> (host);
        stepPanel->onClose = [this] { stepPanel->setVisible (false); };
        stepPanel->onStructureChanged = [this]
        {
            rebuildRows();
            viewChanged();
        };
        addAndMakeVisible (stepPanel.get());
    }

    stepPanel->rebuildPatternFromClip();
    stepPanel->setVisible (true);
    stepPanel->toFront (false);
    resized();
    transport.setStatus (tr ("Séquenceur : cliquez les pads, puis LECTURE pour entendre le motif."));
}

void MainComponent::exportProject()
{
    juce::FileChooser chooser (tr ("Exporter le mixage en WAV"),
                               juce::File::getSpecialLocation (juce::File::userDocumentsDirectory)
                                   .getChildFile ("FD-gPE")
                                   .getChildFile ("mixage.wav"),
                               "*.wav",
                               true);

    if (! chooser.browseForFileToSave (true))
        return;

    const auto destination = chooser.getResult().withFileExtension ("wav");
    destination.getParentDirectory().createDirectory();

    transport.setStatus (tr ("Export WAV en cours..."));
    const bool ok = host.exportWav (destination);
    transport.setStatus (ok ? tr ("Export terminé : ") + destination.getFileName()
                            : tr ("Échec de l'export WAV."));

    juce::AlertWindow::showMessageBoxAsync (
        ok ? juce::MessageBoxIconType::InfoIcon : juce::MessageBoxIconType::WarningIcon,
        ok ? tr ("Export terminé") : tr ("Export impossible"),
        ok ? destination.getFullPathName() : tr ("Vérifiez le dossier de destination et réessayez."));
}

void MainComponent::exportProjectAdvanced()
{
    juce::PopupMenu rateMenu;
    rateMenu.addItem (1, "44 100 Hz");
    rateMenu.addItem (2, "48 000 Hz");
    rateMenu.addItem (3, "96 000 Hz");

    const int rateChoice = rateMenu.showAt (&projectButton, 0, projectButton.getHeight());
    if (rateChoice == 0)
        return;

    juce::PopupMenu bitMenu;
    bitMenu.addItem (1, "16 bits");
    bitMenu.addItem (2, "24 bits");
    bitMenu.addItem (3, "32 bits");

    const int bitChoice = bitMenu.showAt (&projectButton, 0, projectButton.getHeight());
    if (bitChoice == 0)
        return;

    juce::FileChooser chooser (tr ("Exporter avec options"),
                               juce::File::getSpecialLocation (juce::File::userDocumentsDirectory)
                                   .getChildFile ("FD-gPE")
                                   .getChildFile ("mixage.wav"),
                               "*.wav",
                               true);

    if (! chooser.browseForFileToSave (true))
        return;

    ExportOptions options;
    options.destination = chooser.getResult().withFileExtension ("wav");
    options.sampleRate  = rateChoice == 1 ? 44100.0 : (rateChoice == 2 ? 48000.0 : 96000.0);
    options.bitDepth    = bitChoice == 1 ? 16 : (bitChoice == 2 ? 24 : 32);
    options.startSeconds = 0.0;
    options.endSeconds   = -1.0;

    options.destination.getParentDirectory().createDirectory();
    transport.setStatus (tr ("Export WAV en cours..."));
    const bool ok = host.exportAudio (options);

    juce::AlertWindow::showMessageBoxAsync (
        ok ? juce::MessageBoxIconType::InfoIcon : juce::MessageBoxIconType::WarningIcon,
        ok ? tr ("Export terminé") : tr ("Export impossible"),
        ok ? options.destination.getFullPathName() : tr ("Vérifiez le dossier de destination et réessayez."));

    transport.setStatus (ok ? tr ("Export terminé.") : tr ("Échec de l'export WAV."));
}

void MainComponent::addMarker()
{
    auto* content = new juce::Component();
    auto* nameEditor = new juce::TextEditor();
    auto* okButton = new juce::TextButton (tr ("AJOUTER"));
    auto* cancelButton = new juce::TextButton (tr ("ANNULER"));

    nameEditor->setText (tr ("Marqueur ") + juce::String (host.getMarkers().size() + 1));
    nameEditor->setMultiLine (false);
    nameEditor->setReturnKeyStartsNewLine (false);

    content->addAndMakeVisible (nameEditor);
    content->addAndMakeVisible (okButton);
    content->addAndMakeVisible (cancelButton);
    content->setSize (380, 112);

    nameEditor->setBounds (16, 16, 348, 32);
    okButton->setBounds (196, 62, 82, 32);
    cancelButton->setBounds (288, 62, 76, 32);

    juce::Component::SafePointer<MainComponent> safe (this);

    okButton->onClick = [content, nameEditor, safe]
    {
        if (safe != nullptr)
        {
            safe->host.addMarker (nameEditor->getText().trim(), safe->host.getPositionSeconds());
            safe->transport.setStatus (tr ("Marqueur ajouté."));
        }

        if (auto* window = dynamic_cast<juce::DialogWindow*> (content->getTopLevelComponent()))
            window->closeButtonPressed();
    };

    cancelButton->onClick = [content]
    {
        if (auto* window = dynamic_cast<juce::DialogWindow*> (content->getTopLevelComponent()))
            window->closeButtonPressed();
    };

    juce::DialogWindow::LaunchOptions options;
    options.dialogTitle = tr ("Ajouter un marqueur");
    options.dialogBackgroundColour = palette::panel;
    options.escapeKeyTriggersCloseButton = true;
    options.useNativeTitleBar = true;
    options.resizable = false;
    options.content.setOwned (content);
    options.launchAsync();
}

//==============================================================================
void MainComponent::timerCallback()
{
    ++tick;

    if (stepPanel != nullptr && stepPanel->isVisible())
        stepPanel->updatePlayhead();

    releaseHeldKeys();

    const bool playing = host.isPlaying() || host.isRecording();
    const bool recording = host.isRecording();
    const double position = host.getPositionSeconds();

    // Un nouveau transport reprend le suivi ; un défilement manuel le coupe.
    if (playing && ! wasPlaying)
        followPlayhead = true;

    // À l'arrêt du REC, on repeint les pistes : c'est là que le clip
    // enregistré devient visible (paintLane relit track.getClips()).
    if (wasRecording && ! recording)
        viewChanged();

    wasPlaying = playing;
    wasRecording = recording;

    // la vue suit la tête de lecture (sauf si l'utilisateur a défilé à la main)
    if (playing && followPlayhead)
    {
        const double visible = (double) ruler.getWidth() / model.pixelsPerSecond;

        if (position < model.startSeconds || position > model.startSeconds + visible * 0.92)
        {
            model.startSeconds = juce::jmax (0.0, position - visible * 0.08);
            viewChanged();
        }
    }

    overlay.setPosition (position);

    for (auto& row : rows)
    {
        row->updateMeter();

        if (host.isRecording())
            row->repaintLane();     // le bloc rouge grandit
    }

    if (tick % 3 == 0)
    {
        transport.refresh();
        refreshRows();
    }

    // Après un enregistrement, le clip apparaît un instant plus tard : on redessine les pistes régulièrement.
    if (tick % 10 == 0)
        for (auto& row : rows)
            row->repaintLane();
}
} // namespace fdgpe
