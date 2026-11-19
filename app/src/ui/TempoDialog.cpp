#include "TempoDialog.h"

#include "Palette.h"
#include "Text.h"

namespace fdgpe
{
//==============================================================================
TempoRow::TempoRow (EngineHost& h, const TempoPoint& p, std::function<void()> change)
    : host (h), point (p), changed (std::move (change))
{
    beatLabel.setText (juce::String (point.beat, 2), juce::dontSendNotification);
    beatLabel.setFont (juce::Font (juce::FontOptions (juce::Font::getDefaultSansSerifFontName(), 13.0f, juce::Font::bold)));
    beatLabel.setColour (juce::Label::textColourId, palette::blue);
    beatLabel.setJustificationType (juce::Justification::centred);

    bpmCaption.setText (tr ("BPM"), juce::dontSendNotification);
    sigCaption.setText (tr ("Signature"), juce::dontSendNotification);

    for (auto* label : { (juce::Label*) &bpmCaption, (juce::Label*) &sigCaption })
    {
        label->setFont (juce::Font (juce::FontOptions (10.5f, juce::Font::bold)));
        label->setColour (juce::Label::textColourId, palette::textDim);
        label->setJustificationType (juce::Justification::centredLeft);
        label->setInterceptsMouseClicks (false, false);
    }

    bpmEditor.setText (juce::String (point.bpm, 1), juce::dontSendNotification);
    bpmEditor.setJustification (juce::Justification::centred);
    bpmEditor.onTextChange = [this]
    {
        const double value = bpmEditor.getText().getDoubleValue();

        if (value >= 20.0 && value <= 300.0)
        {
            host.setTempoAtBeat (point.beat, value);

            if (changed)
                changed();
        }
    };

    sigEditor.setText (juce::String (point.numerator) + "/" + juce::String (point.denominator), juce::dontSendNotification);
    sigEditor.setJustification (juce::Justification::centred);
    sigEditor.onTextChange = [this]
    {
        const auto parts = juce::StringArray::fromTokens (sigEditor.getText(), "/", "");

        if (parts.size() == 2)
        {
            const int num = parts[0].getIntValue();
            const int den = parts[1].getIntValue();

            if (num >= 1 && num <= 32 && den >= 1 && den <= 32)
            {
                host.setTimeSigAtBeat (point.beat, num, den);

                if (changed)
                    changed();
            }
        }
    };

    removeButton.setButtonText ("X");
    removeButton.onClick = [this]
    {
        int index = 0;

        for (const auto& mapPoint : host.getTempoMap())
        {
            if (std::abs (mapPoint.beat - point.beat) < 0.001)
            {
                host.removeTempoAt (index);
                break;
            }

            ++index;
        }

        if (changed)
            changed();
    };

    for (auto* c : { (juce::Component*) &beatLabel, (juce::Component*) &bpmCaption, (juce::Component*) &sigCaption,
                     (juce::Component*) &bpmEditor, (juce::Component*) &sigEditor, (juce::Component*) &removeButton })
        addAndMakeVisible (c);
}

void TempoRow::paint (juce::Graphics& g)
{
    g.setColour (juce::Colours::black.withAlpha (0.03f));
    g.fillRoundedRectangle (getLocalBounds().toFloat().reduced (1.0f), 6.0f);
    g.setColour (palette::line);
    g.drawRoundedRectangle (getLocalBounds().toFloat().reduced (1.0f), 6.0f, 1.0f);
}

void TempoRow::resized()
{
    auto r = getLocalBounds().reduced (10, 4);
    beatLabel.setBounds (r.removeFromLeft (80));
    bpmCaption.setBounds (r.removeFromLeft (60));
    bpmEditor.setBounds (r.removeFromLeft (80).reduced (0, 5));
    r.removeFromLeft (12);
    sigCaption.setBounds (r.removeFromLeft (90));
    sigEditor.setBounds (r.removeFromLeft (90).reduced (0, 5));
    removeButton.setBounds (r.removeFromRight (44).reduced (0, 5));
}

//==============================================================================
TempoDialog::TempoDialog (EngineHost& h) : host (h)
{
    titleLabel.setText (tr ("Gestion du tempo"), juce::dontSendNotification);
    titleLabel.setFont (juce::Font (juce::FontOptions (20.0f, juce::Font::bold)));
    titleLabel.setColour (juce::Label::textColourId, palette::text);

    hintLabel.setText (tr ("Modifiez un BPM ou une signature, ou ajoutez un marqueur à la position de lecture."),
                       juce::dontSendNotification);
    hintLabel.setFont (juce::Font (juce::FontOptions (11.5f, juce::Font::plain)));
    hintLabel.setColour (juce::Label::textColourId, palette::textDim);

    currentLabel.setFont (juce::Font (juce::FontOptions (12.0f, juce::Font::bold)));
    currentLabel.setColour (juce::Label::textColourId, palette::blue);

    addTempoButton.setButtonText (tr ("+ MARQUEUR DE TEMPO"));
    addSigButton.setButtonText (tr ("+ SIGNATURE"));
    tapButton.setButtonText (tr ("TAP TEMPO"));
    closeButton.setButtonText (tr ("FERMER"));

    bpmEditor.setText (juce::String (host.getTempoBpm(), 1), juce::dontSendNotification);
    bpmEditor.setJustification (juce::Justification::centred);
    const auto sig = host.getTimeSignature();
    sigEditor.setText (juce::String (sig.first) + "/" + juce::String (sig.second), juce::dontSendNotification);
    sigEditor.setJustification (juce::Justification::centred);

    addTempoButton.onClick = [this]
    {
        host.setTempoAtBeat (host.getPositionBeats(), bpmEditor.getText().getDoubleValue());
        rebuild();
        statusLabel.setText (tr ("Marqueur de tempo ajouté."), juce::dontSendNotification);
    };

    addSigButton.onClick = [this]
    {
        const auto parts = juce::StringArray::fromTokens (sigEditor.getText(), "/", "");

        if (parts.size() == 2)
        {
            host.setTimeSigAtBeat (host.getPositionBeats(), parts[0].getIntValue(), parts[1].getIntValue());
            rebuild();
            statusLabel.setText (tr ("Signature ajoutée."), juce::dontSendNotification);
        }
    };

    tapButton.onClick = [this]
    {
        host.tapTempo();
        bpmEditor.setText (juce::String (host.getTempoBpm(), 1), juce::dontSendNotification);
        rebuild();
    };

    closeButton.onClick = [this]
    {
        host.saveProject();

        if (auto* window = findParentComponentOfClass<juce::DialogWindow>())
            window->closeButtonPressed();
    };

    statusLabel.setFont (juce::Font (juce::FontOptions (11.0f, juce::Font::plain)));
    statusLabel.setColour (juce::Label::textColourId, palette::textDim);

    viewport.setViewedComponent (&list, false);
    viewport.setScrollBarsShown (true, false);

    for (juce::Component* c : { (juce::Component*) &titleLabel, (juce::Component*) &hintLabel,
                                (juce::Component*) &currentLabel, (juce::Component*) &addTempoButton,
                                (juce::Component*) &addSigButton, (juce::Component*) &tapButton,
                                (juce::Component*) &closeButton, (juce::Component*) &bpmEditor,
                                (juce::Component*) &sigEditor, (juce::Component*) &statusLabel,
                                (juce::Component*) &viewport })
        addAndMakeVisible (c);

    rebuild();
    setSize (720, 540);
}

void TempoDialog::paint (juce::Graphics& g)
{
    g.fillAll (palette::panel);
    g.setColour (palette::blue);
    g.drawHorizontalLine (0, 0.0f, (float) getWidth());
}

void TempoDialog::resized()
{
    auto area = getLocalBounds().reduced (18, 14);
    titleLabel.setBounds (area.removeFromTop (32));
    hintLabel.setBounds (area.removeFromTop (22));

    auto current = area.removeFromTop (34);
    currentLabel.setBounds (current.removeFromLeft (280));
    tapButton.setBounds (current.removeFromRight (100).reduced (0, 2));

    auto addRow = area.removeFromTop (40);
    addTempoButton.setBounds (addRow.removeFromLeft (140).reduced (0, 3));
    bpmEditor.setBounds (addRow.removeFromLeft (70).reduced (0, 5));
    addRow.removeFromLeft (8);
    addSigButton.setBounds (addRow.removeFromLeft (120).reduced (0, 3));
    sigEditor.setBounds (addRow.removeFromLeft (80).reduced (0, 5));
    closeButton.setBounds (addRow.removeFromRight (90).reduced (0, 3));

    statusLabel.setBounds (area.removeFromBottom (24));
    viewport.setBounds (area);
}

void TempoDialog::rebuild()
{
    rows.clear();
    list.removeAllChildren();

    for (const auto& point : host.getTempoMap())
    {
        auto row = std::make_unique<TempoRow> (host, point, [this] { scheduleRebuild(); });
        list.addAndMakeVisible (*row);
        rows.push_back (std::move (row));
    }

    const int width = juce::jmax (500, viewport.getMaximumVisibleWidth() - 8);
    list.setSize (width, juce::jmax (viewport.getHeight(), (int) rows.size() * 36 + 8));

    int y = 4;

    for (auto& row : rows)
    {
        row->setBounds (4, y, width - 8, 32);
        y += 36;
    }

    const auto sig = host.getTimeSignature();
    currentLabel.setText (tr ("Position : ") + juce::String (host.getPositionBeats(), 2) + tr (" battements · ")
                          + juce::String (sig.first) + "/" + juce::String (sig.second),
                          juce::dontSendNotification);
}

void TempoDialog::scheduleRebuild()
{
    // La ligne peut être en train de traiter un TextEditor::onTextChange : on
    // reporte sa destruction à la prochaine tournée de la boucle de messages.
    juce::MessageManager::callAsync ([safe = juce::Component::SafePointer<TempoDialog> (this)]
    {
        if (safe != nullptr)
            safe->rebuild();
    });
}

void TempoDialog::show (EngineHost& host)
{
    juce::DialogWindow::LaunchOptions options;
    options.dialogTitle = tr ("Tempo et signature");
    options.dialogBackgroundColour = palette::panel;
    options.escapeKeyTriggersCloseButton = true;
    options.useNativeTitleBar = true;
    options.resizable = true;

    options.content.setOwned (new TempoDialog (host));

    auto* window = options.launchAsync();
    window->setSize (740, 560);
    window->centreWithSize (740, 560);
}
} // namespace fdgpe
