#include "TransportBar.h"

#include "FdLookAndFeel.h"
#include "Palette.h"
#include "Text.h"

namespace fdgpe
{
namespace
{
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
    setupButton (pluginsButton, tr ("PLUGINS"));
    setupButton (pianoButton, tr ("PIANO ROLL"));
    setupButton (tempoButton, tr ("TEMPO"));
    setupButton (exportButton, tr ("EXPORTER"));
    setupButton (audioButton, tr ("AUDIO"));
    setupButton (mixerButton, tr ("MIXEUR"));
    setupButton (snapButton, tr ("SNAP 1/4"));
    setupButton (projectButton, tr ("PROJET"));
    setupButton (tempoDownButton, "-");
    setupButton (tempoUpButton, "+");

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

    pluginsButton.onClick = [this] { if (callbacks.pluginBrowser) callbacks.pluginBrowser(); };
    pianoButton.onClick   = [this] { if (callbacks.pianoRoll) callbacks.pianoRoll(); };
    tempoButton.onClick   = [this] { if (callbacks.tempoMap) callbacks.tempoMap(); };
    exportButton.onClick  = [this] { if (callbacks.exportWav) callbacks.exportWav(); };
    audioButton.onClick   = [this] { if (callbacks.audioSettings) callbacks.audioSettings(); };
    mixerButton.onClick   = [this] { if (callbacks.mixer) callbacks.mixer(); };
    snapButton.onClick    = [this] { if (callbacks.cycleSnap) callbacks.cycleSnap(); };
    projectButton.onClick = [this] { if (callbacks.projectMenu) callbacks.projectMenu(); };

    tempoDownButton.onClick = [this] { host.setTempoBpm (std::round (host.getTempoBpm()) - 1.0); refresh(); };
    tempoUpButton.onClick   = [this] { host.setTempoBpm (std::round (host.getTempoBpm()) + 1.0); refresh(); };

    styleLabel (positionLabel, juce::Font (juce::FontOptions (juce::Font::getDefaultMonospacedFontName(), 24.0f, juce::Font::bold)),
                palette::text, juce::Justification::centred);
    styleLabel (secondsLabel, juce::Font (juce::FontOptions (juce::Font::getDefaultMonospacedFontName(), 12.0f, juce::Font::plain)),
                palette::textDim, juce::Justification::centred);
    styleLabel (tempoCaption, juce::Font (juce::FontOptions (10.0f, juce::Font::bold)), palette::textDim, juce::Justification::centred);
    styleLabel (tempoLabel, juce::Font (juce::FontOptions (juce::Font::getDefaultMonospacedFontName(), 20.0f, juce::Font::bold)),
                palette::text, juce::Justification::centred);
    styleLabel (sigLabel, juce::Font (juce::FontOptions (14.0f, juce::Font::bold)), palette::blue, juce::Justification::centred);
    styleLabel (cpuLabel, juce::Font (juce::FontOptions (10.5f, juce::Font::bold)), palette::textDim, juce::Justification::centredRight);
    styleLabel (masterLabel, juce::Font (juce::FontOptions (10.0f, juce::Font::bold)), palette::textDim, juce::Justification::centredLeft);
    styleLabel (statusLabel, juce::Font (juce::FontOptions (11.5f, juce::Font::plain)), palette::textDim, juce::Justification::centredLeft);

    tempoCaption.setText (tr ("TEMPO"), juce::dontSendNotification);
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
                                (juce::Component*) &addMarkerButton, (juce::Component*) &nextMarkerButton, (juce::Component*) &pluginsButton,
                                (juce::Component*) &pianoButton, (juce::Component*) &tempoButton, (juce::Component*) &exportButton,
                                (juce::Component*) &audioButton, (juce::Component*) &mixerButton, (juce::Component*) &snapButton,
                                (juce::Component*) &projectButton, (juce::Component*) &tempoDownButton, (juce::Component*) &tempoUpButton,
                                (juce::Component*) &positionLabel, (juce::Component*) &secondsLabel, (juce::Component*) &tempoCaption,
                                (juce::Component*) &tempoLabel, (juce::Component*) &sigLabel, (juce::Component*) &cpuLabel,
                                (juce::Component*) &masterLabel, (juce::Component*) &masterSlider, (juce::Component*) &statusLabel })
        addAndMakeVisible (c);

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

    // Dégradé clair avec reflet : aspect "barre de navigateur"
    g.setGradientFill (juce::ColourGradient (palette::panelHi, 0.0f, 0.0f,
                                             palette::bgBottom, 0.0f, r.getBottom(), false));
    g.fillRect (r);

    g.setColour (juce::Colours::white.withAlpha (0.9f));
    g.drawHorizontalLine (0, 0.0f, r.getRight());
    g.setColour (palette::blue);
    g.drawHorizontalLine ((float) getHeight() - 1, 0.0f, r.getRight());

    // Zone logo
    auto logoArea = juce::Rectangle<float> (12.0f, 10.0f, 116.0f, (float) getHeight() - 20.0f);

    if (logo.isValid())
    {
        g.drawImageWithin (logo, (int) logoArea.getX(), (int) logoArea.getY(),
                           (int) logoArea.getWidth(), (int) logoArea.getHeight(),
                           juce::RectanglePlacement::centred | juce::RectanglePlacement::onlyReduceInSize);
    }
    else
    {
        g.setColour (palette::blue);
        g.fillRoundedRectangle (14.0f, 22.0f, 12.0f, 12.0f, 4.0f);
        g.setColour (palette::text);
        g.setFont (juce::Font (juce::FontOptions (22.0f, juce::Font::bold)));
        g.drawText ("FD gPE", 32, 10, 92, 28, juce::Justification::centredLeft);
        g.setColour (palette::textDim);
        g.setFont (juce::Font (juce::FontOptions (10.0f, juce::Font::bold)));
        g.drawText (tr ("STUDIO"), 33, 38, 90, 12, juce::Justification::centredLeft);
    }

    // séparateur entre les deux rangées
    g.setColour (palette::line);
    g.drawHorizontalLine (getHeight() - 45, 12.0f, r.getRight() - 12.0f);
}

void TransportBar::resized()
{
    auto area = getLocalBounds().reduced (12, 8);
    const int gap = 6;
    auto top = area.removeFromTop (42);
    auto bottom = area;

    auto place = [&] (juce::Component& c, int width, juce::Rectangle<int>& row)
    {
        c.setBounds (row.removeFromLeft (width).withHeight (row.getHeight()).withY (row.getY()));
        row.removeFromLeft (gap);
    };

    // Rangée 1 : transport et lecture
    place (returnButton, 40, top);
    place (playButton, 72, top);
    place (recButton, 58, top);
    place (loopButton, 66, top);
    place (metroButton, 62, top);
    top.removeFromLeft (6);

    auto position = top.removeFromLeft (138);
    positionLabel.setBounds (position.removeFromTop (28));
    secondsLabel.setBounds (position);
    top.removeFromLeft (gap);

    auto tempo = top.removeFromLeft (110);
    tempoCaption.setBounds (tempo.removeFromTop (12));
    auto tempoRow = tempo.withHeight (26).withY (tempo.getY() + 2);
    tempoDownButton.setBounds (tempoRow.removeFromLeft (26));
    tempoUpButton.setBounds (tempoRow.removeFromRight (26));
    tempoLabel.setBounds (tempoRow);
    top.removeFromLeft (gap);

    place (sigLabel, 44, top);
    top.removeFromLeft (4);

    cpuLabel.setBounds (top.removeFromRight (58));
    top.removeFromRight (gap);
    place (audioButton, 90, top);
    top.removeFromRight (gap);
    masterLabel.setBounds (top.removeFromRight (52).reduced (0, 12));
    masterSlider.setBounds (top.removeFromRight (92).reduced (0, 12));

    // Rangée 2 : édition et outils
    bottom.removeFromLeft (132);   // sous le logo
    bottom.removeFromTop (4);

    place (undoButton, 52, bottom);
    place (redoButton, 52, bottom);
    place (tapButton, 42, bottom);
    place (prevMarkerButton, 30, bottom);
    place (addMarkerButton, 72, bottom);
    place (nextMarkerButton, 30, bottom);
    place (pluginsButton, 76, bottom);
    place (pianoButton, 86, bottom);
    place (tempoButton, 66, bottom);
    place (mixerButton, 66, bottom);
    place (snapButton, 70, bottom);
    place (exportButton, 78, bottom);
    place (projectButton, 68, bottom);

    statusLabel.setBounds (bottom.reduced (0, 4));
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
    positionLabel.setText (juce::String::formatted ("%03d.%d", bb.bar, bb.beat), juce::dontSendNotification);
    secondsLabel.setText (formatTime (host.getPositionSeconds()), juce::dontSendNotification);
    tempoLabel.setText (juce::String (host.getTempoBpm(), 1), juce::dontSendNotification);

    const auto sig = host.getTimeSignature();
    sigLabel.setText (juce::String (sig.first) + "/" + juce::String (sig.second), juce::dontSendNotification);

    cpuLabel.setText ("CPU " + juce::String (juce::roundToInt (host.getCpuUsage() * 100.0f)) + " %", juce::dontSendNotification);
}

void TransportBar::setStatus (const juce::String& message)
{
    statusLabel.setText (message, juce::dontSendNotification);
}

void TransportBar::setSnapLabel (const juce::String& label)
{
    snapButton.setButtonText (label);
}
} // namespace fdgpe
