#include "MixerDock.h"

#include <cmath>

#include "FdLookAndFeel.h"
#include "Palette.h"
#include "Text.h"

namespace fdgpe
{
//==============================================================================
class MixerDock::Strip final : public juce::Component
{
public:
    static constexpr int width = 76;

    Strip (EngineHost& h, te::AudioTrack& t, Callbacks cb)
        : host (h), track (t),
          pluginStrip (h, t,
          {
              [this, cb] { if (cb.addPlugin) cb.addPlugin (track); },
              [cb] (te::Plugin& p) { if (cb.pluginEditor) cb.pluginEditor (p); },
              [cb] { if (cb.changed) cb.changed(); }
          })
    {
        nameLabel.setText (track.getName(), juce::dontSendNotification);
        nameLabel.setFont (juce::Font (juce::FontOptions (11.0f, juce::Font::bold)));
        nameLabel.setJustificationType (juce::Justification::centred);
        nameLabel.setInterceptsMouseClicks (false, false);

        volume.setSliderStyle (juce::Slider::LinearVertical);
        volume.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
        volume.setRange (0.0, 1.0, 0.0);
        volume.setDoubleClickReturnValue (true, (double) te::decibelsToVolumeFaderPosition (0.0f));
        volume.onValueChange = [this]
        {
            if (auto* vp = track.getVolumePlugin())
                vp->setSliderPos ((float) volume.getValue());
        };

        pan.setSliderStyle (juce::Slider::LinearHorizontal);
        pan.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
        pan.setRange (-1.0, 1.0, 0.0);
        pan.setDoubleClickReturnValue (true, 0.0);
        pan.onValueChange = [this]
        {
            if (auto* vp = track.getVolumePlugin())
                vp->setPan ((float) pan.getValue());
        };

        sendSlider.setSliderStyle (juce::Slider::LinearHorizontal);
        sendSlider.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
        sendSlider.setRange (-60.0, 6.0, 0.1);
        sendSlider.setDoubleClickReturnValue (true, -60.0);
        sendSlider.setTooltip (tr ("Envoi Bus 1"));
        sendSlider.onValueChange = [this]
        {
            if (auto* send = host.getOrCreateAuxSend (track, 0))
            {
                const float db = (float) sendSlider.getValue();
                send->setMute (db <= -59.9f);
                send->setGainDb (db <= -59.9f ? -60.0f : db);
            }
        };

        muteButton.onClick = [this] { track.setMute (! track.isMuted (false)); refresh(); };
        soloButton.onClick = [this] { track.setSolo (! track.isSolo (false));  refresh(); };

        pluginStrip.setVertical (true);

        for (juce::Component* c : { (juce::Component*) &nameLabel, (juce::Component*) &volume, (juce::Component*) &pan,
                                    (juce::Component*) &sendSlider, (juce::Component*) &muteButton, (juce::Component*) &soloButton,
                                    (juce::Component*) &pluginStrip })
            addAndMakeVisible (c);

        if (auto* meter = track.getLevelMeterPlugin())
            meter->measurer.addClient (meterClient);

        refresh();
    }

    ~Strip() override
    {
        if (auto* meter = track.getLevelMeterPlugin())
            meter->measurer.removeClient (meterClient);
    }

    void refresh()
    {
        muteButton.setToggleState (track.isMuted (false), juce::dontSendNotification);
        soloButton.setToggleState (track.isSolo (false), juce::dontSendNotification);

        if (auto* vp = track.getVolumePlugin())
        {
            if (! volume.isMouseButtonDown())
                volume.setValue (vp->getSliderPos(), juce::dontSendNotification);

            if (! pan.isMouseButtonDown())
                pan.setValue (vp->getPan(), juce::dontSendNotification);
        }

        if (! sendSlider.isMouseButtonDown())
        {
            if (auto* send = host.getAuxSend (track, 0))
                sendSlider.setValue (send->isMute() ? -60.0 : (double) send->getGainDb(), juce::dontSendNotification);
            else
                sendSlider.setValue (-60.0, juce::dontSendNotification);
        }

        if (nameLabel.getText() != track.getName())
            nameLabel.setText (track.getName(), juce::dontSendNotification);

        pluginStrip.refresh();
    }

    void updateMeter()
    {
        float db = -100.0f;

        for (int ch = 0; ch < meterClient.getNumChannelsUsed(); ++ch)
            db = juce::jmax (db, meterClient.getAndClearAudioLevel (ch).dB);

        const float target = juce::jlimit (0.0f, 1.0f, juce::jmap (db, -60.0f, 0.0f, 0.0f, 1.0f));
        const float previous = meterLevel;

        meterLevel = target > meterLevel ? target : meterLevel * 0.80f;

        if (std::abs (meterLevel - previous) > 0.02f)
            repaint (meterBounds.expanded (2));
    }

    void paint (juce::Graphics& g) override
    {
        auto r = getLocalBounds().toFloat();

        g.setColour (palette::strip);
        g.fillRect (r);

        // Bande de la couleur de piste en haut de la tranche.
        g.setColour (host.getTrackColour (track));
        g.fillRect (r.getX(), 0.0f, r.getWidth(), 3.0f);

        g.setColour (palette::line);
        g.drawVerticalLine (getWidth() - 1, 0.0f, r.getBottom());

        g.setColour (palette::textFaint);
        g.setFont (juce::Font (juce::FontOptions (9.0f, juce::Font::bold)));
        g.drawText (tr ("EFFETS"), 8, 26, 60, 11, juce::Justification::centredLeft);

        // Vu-mètre.
        const auto m = meterBounds.toFloat();
        const int segments = 28;
        const float gap = 1.0f;
        const float segH = (m.getHeight() - gap * (float) (segments - 1)) / (float) segments;
        const int lit = juce::roundToInt (meterLevel * (float) segments);

        for (int i = 0; i < segments; ++i)
        {
            const float y = m.getBottom() - (float) (i + 1) * segH - (float) i * gap;
            const auto colour = i < 20 ? palette::blue : (i < 24 ? palette::amber : palette::red);

            g.setColour (i < lit ? colour : palette::line);
            g.fillRect (m.getX(), y, m.getWidth(), segH);
        }
    }

    void resized() override
    {
        const int h = getHeight();
        const int half = (width - 16) / 2;

        nameLabel.setBounds (2, 6, width - 4, 16);
        pluginStrip.setBounds (6, 38, width - 12, 60);
        sendSlider.setBounds (6, 100, width - 12, 16);
        pan.setBounds (6, 118, width - 12, 16);

        muteButton.setBounds (6, h - 28, half, 22);
        soloButton.setBounds (6 + half + 4, h - 28, half, 22);

        // Fader long : toute la hauteur restante, pour un réglage fin du volume.
        const int faderTop = 142;
        const int faderBottom = h - 36;

        meterBounds = { 12, faderTop + 4, 8, faderBottom - faderTop - 8 };
        volume.setBounds (28, faderTop, 40, faderBottom - faderTop);
    }

private:
    EngineHost& host;
    te::AudioTrack& track;
    PluginStrip pluginStrip;
    juce::Label nameLabel;
    juce::Slider volume, pan, sendSlider;
    juce::TextButton muteButton { "M" }, soloButton { "S" };
    juce::Rectangle<int> meterBounds;
    te::LevelMeasurer::Client meterClient;
    float meterLevel = 0.0f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Strip)
};

//==============================================================================
class MixerDock::MasterStrip final : public juce::Component
{
public:
    static constexpr int width = 76;

    explicit MasterStrip (EngineHost& h) : host (h)
    {
        nameLabel.setText (tr ("MASTER"), juce::dontSendNotification);
        nameLabel.setFont (juce::Font (juce::FontOptions (11.0f, juce::Font::bold)));
        nameLabel.setJustificationType (juce::Justification::centred);
        nameLabel.setInterceptsMouseClicks (false, false);

        volume.setSliderStyle (juce::Slider::LinearVertical);
        volume.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
        volume.setRange (0.0, 1.0, 0.0);
        volume.setDoubleClickReturnValue (true, (double) te::decibelsToVolumeFaderPosition (0.0f));
        volume.onValueChange = [this]
        {
            if (auto vp = host.getEdit().getMasterVolumePlugin())
                vp->setSliderPos ((float) volume.getValue());
        };

        pan.setSliderStyle (juce::Slider::LinearHorizontal);
        pan.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
        pan.setRange (-1.0, 1.0, 0.0);
        pan.setDoubleClickReturnValue (true, 0.0);
        pan.onValueChange = [this]
        {
            if (auto vp = host.getEdit().getMasterVolumePlugin())
                vp->setPan ((float) pan.getValue());
        };

        muteButton.setEnabled (false);
        soloButton.setEnabled (false);

        for (juce::Component* c : { (juce::Component*) &nameLabel, (juce::Component*) &volume, (juce::Component*) &pan,
                                    (juce::Component*) &muteButton, (juce::Component*) &soloButton })
            addAndMakeVisible (c);

        ensureMeterAttached();
        refresh();
    }

    ~MasterStrip() override
    {
        if (attachedMeasurer != nullptr)
            attachedMeasurer->removeClient (meterClient);
    }

    void refresh()
    {
        ensureMeterAttached();

        if (auto vp = host.getEdit().getMasterVolumePlugin())
        {
            if (! volume.isMouseButtonDown())
                volume.setValue (vp->getSliderPos(), juce::dontSendNotification);

            if (! pan.isMouseButtonDown())
                pan.setValue (vp->getPan(), juce::dontSendNotification);
        }
    }

    void updateMeter()
    {
        ensureMeterAttached();
        float db = -100.0f;

        for (int ch = 0; ch < meterClient.getNumChannelsUsed(); ++ch)
            db = juce::jmax (db, meterClient.getAndClearAudioLevel (ch).dB);

        const float target = juce::jlimit (0.0f, 1.0f, juce::jmap (db, -60.0f, 0.0f, 0.0f, 1.0f));
        const float previous = meterLevel;
        meterLevel = target > meterLevel ? target : meterLevel * 0.80f;

        if (std::abs (meterLevel - previous) > 0.02f)
            repaint (meterBounds.expanded (2));
    }

    void paint (juce::Graphics& g) override
    {
        auto r = getLocalBounds().toFloat();
        g.setColour (palette::strip.darker (0.05f));
        g.fillRect (r);

        g.setColour (palette::blue);
        g.fillRect (r.getX(), 0.0f, r.getWidth(), 3.0f);

        g.setColour (palette::line);
        g.drawVerticalLine (getWidth() - 1, 0.0f, r.getBottom());
        g.drawVerticalLine (0, 0.0f, r.getBottom());

        const auto m = meterBounds.toFloat();
        const int segments = 28;
        const float gap = 1.0f;
        const float segH = (m.getHeight() - gap * (float) (segments - 1)) / (float) segments;
        const int lit = juce::roundToInt (meterLevel * (float) segments);

        for (int i = 0; i < segments; ++i)
        {
            const float y = m.getBottom() - (float) (i + 1) * segH - (float) i * gap;
            const auto colour = i < 20 ? palette::blue : (i < 24 ? palette::amber : palette::red);
            g.setColour (i < lit ? colour : palette::line);
            g.fillRect (m.getX(), y, m.getWidth(), segH);
        }
    }

    void resized() override
    {
        const int h = getHeight();
        const int half = (width - 16) / 2;

        nameLabel.setBounds (2, 6, width - 4, 16);
        pan.setBounds (6, 118, width - 12, 16);

        muteButton.setBounds (6, h - 28, half, 22);
        soloButton.setBounds (6 + half + 4, h - 28, half, 22);

        const int faderTop = 142;
        const int faderBottom = h - 36;
        meterBounds = { 12, faderTop + 4, 8, faderBottom - faderTop - 8 };
        volume.setBounds (28, faderTop, 40, faderBottom - faderTop);
    }

private:
    void ensureMeterAttached()
    {
        auto* ctx = host.getEdit().getCurrentPlaybackContext();
        auto* measurer = ctx != nullptr ? &ctx->masterLevels : nullptr;

        if (measurer == attachedMeasurer)
            return;

        if (attachedMeasurer != nullptr)
            attachedMeasurer->removeClient (meterClient);

        attachedMeasurer = measurer;

        if (attachedMeasurer != nullptr)
            attachedMeasurer->addClient (meterClient);
    }

    EngineHost& host;
    juce::Label nameLabel;
    juce::Slider volume, pan;
    juce::TextButton muteButton { "M" }, soloButton { "S" };
    juce::Rectangle<int> meterBounds;
    te::LevelMeasurer::Client meterClient;
    te::LevelMeasurer* attachedMeasurer = nullptr;
    float meterLevel = 0.0f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MasterStrip)
};

//==============================================================================
MixerDock::MixerDock (EngineHost& h, Callbacks cb)
    : host (h), callbacks (std::move (cb))
{
    viewport.setViewedComponent (&content, false);
    viewport.setScrollBarsShown (false, true);
    viewport.setScrollBarThickness (8);
    addAndMakeVisible (viewport);
}

MixerDock::~MixerDock()
{
    clear();
}

void MixerDock::clear()
{
    strips.clear();
    masterStrip.reset();
}

void MixerDock::rebuild()
{
    clear();

    for (auto* track : host.getTracks())
    {
        auto strip = std::make_unique<Strip> (host, *track, callbacks);
        content.addAndMakeVisible (*strip);
        strips.push_back (std::move (strip));
    }

    masterStrip = std::make_unique<MasterStrip> (host);
    addAndMakeVisible (*masterStrip);

    resized();
}

void MixerDock::refresh()
{
    for (auto& strip : strips)
        strip->refresh();

    if (masterStrip != nullptr)
        masterStrip->refresh();
}

void MixerDock::updateMeters()
{
    for (auto& strip : strips)
        strip->updateMeter();

    if (masterStrip != nullptr)
        masterStrip->updateMeter();
}

void MixerDock::paint (juce::Graphics& g)
{
    g.fillAll (palette::strip);
    g.setColour (palette::line);
    g.drawHorizontalLine (0, 0.0f, (float) getWidth());

    if (strips.empty())
    {
        g.setColour (palette::textDim);
        g.setFont (juce::Font (juce::FontOptions (13.0f)));
        g.drawText (tr ("Aucune piste."), getLocalBounds(), juce::Justification::centred);
    }
}

void MixerDock::resized()
{
    auto area = getLocalBounds().withTrimmedTop (1);

    if (masterStrip != nullptr)
        masterStrip->setBounds (area.removeFromRight (MasterStrip::width));

    viewport.setBounds (area);

    const int contentHeight = juce::jmax (100, dockHeight - 1 - viewport.getScrollBarThickness());
    const int contentWidth = juce::jmax (viewport.getMaximumVisibleWidth(), (int) strips.size() * Strip::width);

    content.setSize (contentWidth, contentHeight);

    int x = 0;

    for (auto& strip : strips)
    {
        strip->setBounds (x, 0, Strip::width, contentHeight);
        x += Strip::width;
    }
}
} // namespace fdgpe
