#include "MainComponent.h"

#include <juce_audio_utils/juce_audio_utils.h>

namespace fdgpe
{
namespace
{
    // Les textes de l'interface sont en UTF-8 (accents).
    juce::String tr (const char* utf8)
    {
        return juce::String::fromUTF8 (utf8);
    }

    juce::String formatTime (double seconds)
    {
        const auto millis = juce::roundToInt (seconds * 1000.0);
        return juce::String::formatted ("%02d:%02d.%03d",
                                        (millis / 60000) % 100,
                                        (millis / 1000) % 60,
                                        millis % 1000);
    }

    void styleLabel (juce::Label& label, float height, bool bold, juce::Colour colour)
    {
        label.setFont (juce::Font (juce::FontOptions (height, bold ? juce::Font::bold : juce::Font::plain)));
        label.setColour (juce::Label::textColourId, colour);
        label.setJustificationType (juce::Justification::topLeft);
    }
}

MainComponent::MainComponent()
{
    titleLabel.setText ("FD gPE", juce::dontSendNotification);
    styleLabel (titleLabel, 36.0f, true, juce::Colours::white);

    subtitleLabel.setText (tr ("Jalon M0 : la fenêtre s'ouvre et le moteur démarre"),
                           juce::dontSendNotification);
    styleLabel (subtitleLabel, 16.0f, false, juce::Colour (0xff9aa3b2));

    styleLabel (statusLabel, 18.0f, false, juce::Colour (0xffe6e9ef));

    playButton.setButtonText (tr ("Lecture / Stop"));
    playButton.onClick = [this] { host.togglePlay(); refreshStatus(); };

    audioButton.setButtonText (tr ("Réglages audio"));
    audioButton.onClick = [this] { showAudioSettings(); };

    for (auto* button : { &playButton, &audioButton })
    {
        button->setColour (juce::TextButton::buttonColourId, juce::Colour (0xff2f6fed));
        button->setColour (juce::TextButton::textColourOffId, juce::Colours::white);
    }

    addAndMakeVisible (titleLabel);
    addAndMakeVisible (subtitleLabel);
    addAndMakeVisible (statusLabel);
    addAndMakeVisible (playButton);
    addAndMakeVisible (audioButton);

    setSize (900, 560);
    refreshStatus();
    startTimerHz (5);
}

MainComponent::~MainComponent()
{
    stopTimer();
    audioSettingsWindow.reset();
}

void MainComponent::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colour (0xff15171c));
}

void MainComponent::resized()
{
    auto area = getLocalBounds().reduced (32);

    titleLabel.setBounds (area.removeFromTop (48));
    subtitleLabel.setBounds (area.removeFromTop (28));
    area.removeFromTop (24);

    auto buttons = area.removeFromBottom (72);
    const auto half = (buttons.getWidth() - 16) / 2;
    playButton.setBounds (buttons.removeFromLeft (half));
    buttons.removeFromLeft (16);
    audioButton.setBounds (buttons);

    area.removeFromBottom (24);
    statusLabel.setBounds (area);
}

void MainComponent::timerCallback()
{
    refreshStatus();
}

void MainComponent::refreshStatus()
{
    const auto status = host.getAudioStatus();

    juce::String text;
    text << tr ("Moteur : ") << EngineHost::getEngineVersion() << "\n\n"
         << tr ("Périphérique audio : ")
         << (status.deviceName.isEmpty() ? tr ("aucun (voir Réglages audio)") : status.deviceName) << "\n"
         << tr ("Fréquence : ") << juce::String (status.sampleRate, 0) << " Hz     "
         << tr ("Tampon : ") << status.blockSize << tr (" échantillons") << "\n"
         << tr ("Charge CPU : ") << juce::roundToInt (status.cpuUsage * 100.0f) << " %\n"
         << tr ("Entrées MIDI détectées : ") << status.midiInputs << "\n\n"
         << tr ("Transport : ") << (host.isPlaying() ? tr ("lecture") : tr ("arrêté"))
         << "     " << formatTime (host.getPositionSeconds());

    statusLabel.setText (text, juce::dontSendNotification);
}

void MainComponent::showAudioSettings()
{
    if (audioSettingsWindow == nullptr)
    {
        juce::DialogWindow::LaunchOptions options;
        options.dialogTitle = tr ("Réglages audio");
        options.dialogBackgroundColour = juce::Colour (0xff15171c);
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
} // namespace fdgpe
