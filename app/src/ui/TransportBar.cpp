#include "TransportBar.h"

#include "FdLookAndFeel.h"
#include "Palette.h"
#include "Text.h"

namespace fdgpe
{
namespace
{
    // Barre compacte sur une seule rangée.
    constexpr int buttonHeight = 30;

    juce::String formatTime (double seconds)
    {
        const auto millis = juce::roundToInt (juce::jmax (0.0, seconds) * 1000.0);
        return juce::String::formatted ("%02d:%02d.%03d", (millis / 60000) % 100, (millis / 1000) % 60, millis % 1000);
    }

    void styleLabel (juce::Label& label, juce::Font font, juce::Colour colour, juce::Justification j)
    {
        label.setFont (font);
        label.setColour (juce::Label::textColourId, colour);
        label.setJustificationType (j);
        label.setInterceptsMouseClicks (false, false);
    }

    void setupButton (juce::TextButton& button, const juce::String& text, juce::Colour led = {})
    {
        button.setButtonText (text);

        if (led != juce::Colour())
            FdLookAndFeel::setLed (button, led);
    }
}

TransportBar::TransportBar (EngineHost& h, Callbacks cb)
    : host (h), callbacks (std::move (cb))
{
    setupButton (returnButton, "|<");
    setupButton (playButton, tr ("LECTURE"), palette::blue);
    setupButton (recButton, tr ("REC"), palette::red);
    setupButton (loopButton, tr ("BOUCLE"), palette::blue);
    setupButton (metroButton, tr ("MÉTRO"), palette::blue);
    setupButton (undoButton, tr ("ANNULER"));
    setupButton (redoButton, tr ("RÉTABLIR"));
    setupButton (tapButton, tr ("TAP"));
    setupButton (prevMarkerButton, "<M");
    setupButton (addMarkerButton, tr ("+ MARQUEUR"));
    setupButton (nextMarkerButton, "M>");
    setupButton (menuButton, tr ("MENU"));
    setupButton (libraryButton, tr ("NAVIGATEUR"));
    setupButton (projectButton, tr ("PROJET"));
    setupButton (tempoDownButton, "-");
    setupButton (tempoUpButton, "+");

    // Les libellés restent le nom accessible des boutons ; seuls les glyphes sont peints.
    FdLookAndFeel::setSymbol (playButton, FdLookAndFeel::ButtonSymbol::play);
    FdLookAndFeel::setSymbol (recButton, FdLookAndFeel::ButtonSymbol::rec);
    FdLookAndFeel::setSymbol (loopButton, FdLookAndFeel::ButtonSymbol::loop);
    FdLookAndFeel::setSymbol (returnButton, FdLookAndFeel::ButtonSymbol::skipBack);

    playButton.setTooltip (tr ("Lancer ou arrêter la lecture."));
    recButton.setTooltip (tr ("Démarrer ou arrêter l'enregistrement."));
    loopButton.setTooltip (tr ("Activer ou désactiver la boucle."));

    menuButton.setTooltip (tr ("Outils : bibliothèque, piano roll, tempo, export, audio, mixeur, magnétisme."));
    libraryButton.setTooltip (tr ("Ouvrir la bibliothèque de plugins."));

    returnButton.onClick = [this] { host.returnToStart(); };
    playButton.onClick   = [this] { host.togglePlay(); refresh(); };
    recButton.onClick    = [this]
    {
        if (host.isRecording() || ! callbacks.record)
            host.toggleRecord();
        else
            callbacks.record();

        refresh();
    };
    loopButton.onClick   = [this] { host.setLooping (! host.isLooping()); refresh(); };
    metroButton.onClick  = [this] { host.setMetronomeOn (! host.isMetronomeOn()); refresh(); };
    undoButton.onClick   = [this] { host.undo(); refresh(); };
    redoButton.onClick   = [this] { host.redo(); refresh(); };
    tapButton.onClick    = [this] { host.tapTempo(); refresh(); };

    prevMarkerButton.onClick  = [this] { if (callbacks.prevMarker) callbacks.prevMarker(); };
    addMarkerButton.onClick   = [this] { if (callbacks.addMarker) callbacks.addMarker(); };
    nextMarkerButton.onClick  = [this] { if (callbacks.nextMarker) callbacks.nextMarker(); };

    menuButton.onClick    = [this] { showMenu(); };
    libraryButton.onClick = [this] { if (callbacks.pluginBrowser) callbacks.pluginBrowser(); };
    projectButton.onClick = [this] { if (callbacks.projectMenu) callbacks.projectMenu(); };

    tempoDownButton.onClick = [this] { host.setTempoBpm (std::round (host.getTempoBpm()) - 1.0); refresh(); };
    tempoUpButton.onClick   = [this] { host.setTempoBpm (std::round (host.getTempoBpm()) + 1.0); refresh(); };

    styleLabel (positionLabel, juce::Font (juce::FontOptions (juce::Font::getDefaultSansSerifFontName(), 18.0f, juce::Font::bold)),
                palette::text, juce::Justification::centred);
    styleLabel (secondsLabel, juce::Font (juce::FontOptions (juce::Font::getDefaultSansSerifFontName(), 12.0f, juce::Font::plain)),
                palette::textDim, juce::Justification::centredLeft);
    styleLabel (tempoCaption, juce::Font (juce::FontOptions (12.0f, juce::Font::plain)), palette::textDim, juce::Justification::centredLeft);
    styleLabel (tempoLabel, juce::Font (juce::FontOptions (juce::Font::getDefaultSansSerifFontName(), 16.0f, juce::Font::bold)),
                palette::text, juce::Justification::centred);
    styleLabel (sigLabel, juce::Font (juce::FontOptions (14.0f, juce::Font::bold)), palette::blue, juce::Justification::centred);
    styleLabel (cpuLabel, juce::Font (juce::FontOptions (10.5f, juce::Font::bold)), palette::textDim, juce::Justification::centredRight);
    styleLabel (masterLabel, juce::Font (juce::FontOptions (10.0f, juce::Font::bold)), palette::textDim, juce::Justification::centredLeft);
    styleLabel (statusLabel, juce::Font (juce::FontOptions (11.5f, juce::Font::plain)), palette::textDim, juce::Justification::centredLeft);

    tempoCaption.setText (tr ("bpm"), juce::dontSendNotification);
    masterLabel.setText (tr ("MAÎTRE"), juce::dontSendNotification);

    masterSlider.setSliderStyle (juce::Slider::LinearHorizontal);
    masterSlider.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
    masterSlider.setRange (0.0, 1.0, 0.0);
    masterSlider.setValue ((double) host.getMasterVolume(), juce::dontSendNotification);
    masterSlider.onValueChange = [this] { host.setMasterVolume ((float) masterSlider.getValue()); };
    masterSlider.setDoubleClickReturnValue (true, 0.8);

    for (juce::Component* c : { (juce::Component*) &returnButton, (juce::Component*) &playButton, (juce::Component*) &recButton,
                                (juce::Component*) &loopButton, (juce::Component*) &metroButton, (juce::Component*) &undoButton,
                                (juce::Component*) &redoButton, (juce::Component*) &tapButton, (juce::Component*) &prevMarkerButton,
                                (juce::Component*) &addMarkerButton, (juce::Component*) &nextMarkerButton, (juce::Component*) &menuButton,
                                (juce::Component*) &libraryButton, (juce::Component*) &projectButton, (juce::Component*) &tempoDownButton,
                                (juce::Component*) &tempoUpButton, (juce::Component*) &positionLabel, (juce::Component*) &secondsLabel,
                                (juce::Component*) &tempoCaption, (juce::Component*) &tempoLabel, (juce::Component*) &sigLabel,
                                (juce::Component*) &cpuLabel, (juce::Component*) &masterLabel, (juce::Component*) &masterSlider })
        addAndMakeVisible (c);

    // Ces actions vivent dans le menu : la barre reste compacte, comme sur la maquette.
    tapButton.setVisible (false);
    prevMarkerButton.setVisible (false);
    addMarkerButton.setVisible (false);
    nextMarkerButton.setVisible (false);
    secondsLabel.setVisible (false);

    loadLogo();
    refresh();
}

void TransportBar::loadLogo()
{
    // Le logo est préparé par l'utilisateur. On cherche, dans l'ordre :
    //   ~/Documents/FD-gPE/logo.(png|svg)
    //   <dossier du projet>/assets/logo.(png|svg)
    //   <dossier du projet>/logo.(png|svg)
    //   <dossier de l'exécutable>/logo.(png|svg)
    juce::Array<juce::File> folders;
    folders.add (juce::File::getSpecialLocation (juce::File::userDocumentsDirectory).getChildFile ("FD-gPE"));
    folders.add (juce::File::getCurrentWorkingDirectory());
    folders.add (juce::File::getCurrentWorkingDirectory().getChildFile ("assets"));
    folders.add (juce::File::getSpecialLocation (juce::File::currentExecutableFile)
                     .getParentDirectory().getParentDirectory().getParentDirectory().getParentDirectory());

    for (const auto& folder : folders)
    {
        for (const char* name : { "logo.png", "logo.svg", "logo.jpg", "logo.jpeg" })
        {
            const auto file = folder.getChildFile (name);

            if (! file.existsAsFile())
                continue;

            if (file.hasFileExtension ("svg"))
            {
                if (auto drawable = juce::Drawable::createFromSVGFile (file))
                {
                    logo = juce::Image (juce::Image::ARGB, 512, 512, true);
                    juce::Graphics g (logo);
                    drawable->drawWithin (g, logo.getBounds().toFloat(),
                                          juce::RectanglePlacement::centred, 1.0f);
                }
            }
            else
            {
                const auto image = juce::ImageFileFormat::loadFrom (file);

                if (image.isValid())
                    logo = image;
            }

            if (logo.isValid())
                return;
        }
    }
}

void TransportBar::paint (juce::Graphics& g)
{
    auto r = getLocalBounds().toFloat();

    g.setColour (palette::panel);
    g.fillRect (r);
    g.setColour (palette::line);
    g.drawHorizontalLine (getHeight() - 1, 0.0f, r.getRight());

    // Logo : image fournie par l'utilisateur, sinon deux disques (texte + accent).
    auto logoArea = juce::Rectangle<float> (12.0f, 8.0f, 44.0f, (float) getHeight() - 16.0f);

    if (logo.isValid())
    {
        g.drawImageWithin (logo, (int) logoArea.getX(), (int) logoArea.getY(),
                           (int) logoArea.getWidth(), (int) logoArea.getHeight(),
                           juce::RectanglePlacement::centred | juce::RectanglePlacement::onlyReduceInSize);
    }
    else
    {
        const float d = 20.0f;
        const float cy = logoArea.getCentreY();
        g.setColour (palette::text);
        g.fillEllipse (logoArea.getX(), cy - d * 0.5f, d, d);
        g.setColour (palette::blue);
        g.fillEllipse (logoArea.getX() + 14.0f, cy - d * 0.5f, d, d);
    }
}

void TransportBar::resized()
{
    auto area = getLocalBounds().reduced (12, 0);
    const int gap = 4;
    const int centreY = getHeight() / 2;

    auto place = [&] (juce::Component& c, int width, int height = buttonHeight)
    {
        c.setBounds (area.getX(), centreY - height / 2, width, height);
        area.removeFromLeft (width + gap);
    };

    area.removeFromLeft (56);   // logo

    // Droite : CPU et volume maître.
    cpuLabel.setBounds (area.removeFromRight (56).withSizeKeepingCentre (56, 18));
    area.removeFromRight (gap);
    masterSlider.setBounds (area.removeFromRight (84).withSizeKeepingCentre (84, 24));
    masterLabel.setBounds (area.removeFromRight (0));
    area.removeFromRight (gap * 2);

    // Droite : outils.
    {
        auto right = [&] (juce::Component& c, int width)
        {
            c.setBounds (area.getRight() - width, centreY - buttonHeight / 2, width, buttonHeight);
            area.removeFromRight (width + gap);
        };

        right (projectButton, 70);
        right (libraryButton, 96);
        right (menuButton, 62);
        right (redoButton, 70);
        right (undoButton, 66);
    }

    // Gauche : transport.
    place (returnButton, 32);
    place (playButton, 38);
    place (recButton, 38);
    place (loopButton, 38);
    place (metroButton, 56);
    area.removeFromLeft (10);

    // Position, signature, tempo : au centre.
    positionLabel.setBounds (area.getX(), centreY - 12, 76, 24);
    area.removeFromLeft (76 + gap);
    sigLabel.setBounds (area.getX(), centreY - 12, 36, 24);
    area.removeFromLeft (36 + gap * 2);

    tempoDownButton.setBounds (area.getX(), centreY - 11, 22, 22);
    area.removeFromLeft (22);
    tempoLabel.setBounds (area.getX(), centreY - 12, 50, 24);
    area.removeFromLeft (50);
    tempoUpButton.setBounds (area.getX(), centreY - 11, 22, 22);
    area.removeFromLeft (22 + 2);
    tempoCaption.setBounds (area.getX(), centreY - 9, 30, 18);
}

void TransportBar::refresh()
{
    playButton.setToggleState (host.isPlaying() || host.isRecording(), juce::dontSendNotification);
    recButton.setToggleState (host.isRecording(), juce::dontSendNotification);
    loopButton.setToggleState (host.isLooping(), juce::dontSendNotification);
    metroButton.setToggleState (host.isMetronomeOn(), juce::dontSendNotification);
    undoButton.setEnabled (host.canUndo());
    redoButton.setEnabled (host.canRedo());

    const auto bb = host.getBarsBeats();
    positionLabel.setText (juce::String::formatted ("%d.%d", bb.bar, bb.beat), juce::dontSendNotification);
    secondsLabel.setText (formatTime (host.getPositionSeconds()), juce::dontSendNotification);
    tempoLabel.setText (juce::String (host.getTempoBpm(), 1), juce::dontSendNotification);

    const auto sig = host.getTimeSignature();
    sigLabel.setText (juce::String (sig.first) + "/" + juce::String (sig.second), juce::dontSendNotification);

    cpuLabel.setText ("CPU " + juce::String (juce::roundToInt (host.getCpuUsage() * 100.0f)) + " %", juce::dontSendNotification);
}

void TransportBar::setStatus (const juce::String& message)
{
    statusLabel.setText (message, juce::dontSendNotification);

    if (onStatus)
        onStatus (message);
}

void TransportBar::setSnapLabel (const juce::String& label, bool active)
{
    snapText = label;   // le libellé sert au menu « Magnétisme »
    snapActive = active;
}

bool TransportBar::isSnapActive() const
{
    return snapActive;
}

void TransportBar::showMenu()
{
    juce::PopupMenu menu;
    menu.addItem (1, tr ("Bibliothèque de plugins"));
    menu.addItem (2, tr ("Piano roll"));
    menu.addItem (3, tr ("Carte de tempo"));
    menu.addSeparator();
    menu.addItem (4, tr ("Export WAV"));
    menu.addItem (5, tr ("Réglages audio"));
    menu.addItem (6, tr ("Mixeur"));
    menu.addSeparator();
    menu.addItem (7, tr ("Magnétisme : ") + snapText, true, isSnapActive());
    menu.addSeparator();
    menu.addItem (8, tr ("Tap tempo"));
    menu.addItem (9, tr ("Marqueur précédent"));
    menu.addItem (10, tr ("Ajouter un marqueur"));
    menu.addItem (11, tr ("Marqueur suivant"));

    menu.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (menuButton),
                        [safe = juce::Component::SafePointer<TransportBar> (this)] (int result)
                        {
                            if (safe != nullptr && result != 0)
                                safe->handleMenuChoice (result);
                        });
}

void TransportBar::handleMenuChoice (int id)
{
    switch (id)
    {
        case 1: if (callbacks.pluginBrowser) callbacks.pluginBrowser(); break;
        case 2: if (callbacks.pianoRoll)     callbacks.pianoRoll();     break;
        case 3: if (callbacks.tempoMap)      callbacks.tempoMap();      break;
        case 4: if (callbacks.exportWav)     callbacks.exportWav();     break;
        case 5: if (callbacks.audioSettings) callbacks.audioSettings(); break;
        case 6: if (callbacks.mixer)         callbacks.mixer();         break;
        case 7: if (callbacks.cycleSnap)     callbacks.cycleSnap();     break;
        case 8: host.tapTempo(); refresh();                             break;
        case 9: if (callbacks.prevMarker)    callbacks.prevMarker();    break;
        case 10: if (callbacks.addMarker)    callbacks.addMarker();     break;
        case 11: if (callbacks.nextMarker)   callbacks.nextMarker();    break;
        default: break;
    }
}
} // namespace fdgpe
