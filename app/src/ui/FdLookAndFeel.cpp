#include "FdLookAndFeel.h"

namespace fdgpe
{
namespace
{
    const juce::Identifier ledId { "fdLed" };
    const juce::Identifier fontSizeId { "fdFontSize" };
}

FdLookAndFeel::FdLookAndFeel()
{
    refreshPalette();
}

void FdLookAndFeel::refreshPalette()
{
    using namespace palette;

    // Fenêtres
    setColour (juce::ResizableWindow::backgroundColourId, panel);
    setColour (juce::DocumentWindow::textColourId, text);
    setColour (juce::DocumentWindow::backgroundColourId, panel);

    // Labels
    setColour (juce::Label::textColourId, text);
    setColour (juce::Label::textWhenEditingColourId, text);
    setColour (juce::Label::backgroundColourId, panelHi);
    setColour (juce::Label::backgroundWhenEditingColourId, panelHi);
    setColour (juce::Label::outlineColourId, line);
    setColour (juce::Label::outlineWhenEditingColourId, blue);

    // Éditeurs de texte
    setColour (juce::TextEditor::backgroundColourId, panelHi);
    setColour (juce::TextEditor::textColourId, text);
    setColour (juce::TextEditor::highlightColourId, blueSoft);
    setColour (juce::TextEditor::highlightedTextColourId, text);
    setColour (juce::TextEditor::outlineColourId, line);
    setColour (juce::TextEditor::focusedOutlineColourId, blue);

    // Listes déroulantes
    setColour (juce::ComboBox::backgroundColourId, panelHi);
    setColour (juce::ComboBox::textColourId, text);
    setColour (juce::ComboBox::buttonColourId, panel);
    setColour (juce::ComboBox::outlineColourId, outline);
    setColour (juce::ComboBox::arrowColourId, blue);
    setColour (juce::ComboBox::focusedOutlineColourId, blue);

    // Menus
    setColour (juce::PopupMenu::backgroundColourId, panelHi);
    setColour (juce::PopupMenu::textColourId, text);
    setColour (juce::PopupMenu::highlightedBackgroundColourId, blue);
    setColour (juce::PopupMenu::highlightedTextColourId, juce::Colours::white);
    setColour (juce::PopupMenu::headerTextColourId, textDim);

    // Ascenseurs
    setColour (juce::ScrollBar::thumbColourId, outline);
    setColour (juce::ScrollBar::trackColourId, juce::Colours::transparentBlack);

    // Boîtes de dialogue
    setColour (juce::AlertWindow::backgroundColourId, panel);
    setColour (juce::AlertWindow::textColourId, text);
    setColour (juce::AlertWindow::outlineColourId, outline);

    // Divers
    setColour (juce::ToggleButton::textColourId, text);
    setColour (juce::ToggleButton::tickColourId, blue);
    setColour (juce::ToggleButton::tickDisabledColourId, textFaint);
    setColour (juce::ListBox::backgroundColourId, panelHi);
    setColour (juce::ListBox::textColourId, text);
    setColour (juce::TabbedComponent::backgroundColourId, panel);
    setColour (juce::TabbedButtonBar::tabTextColourId, textDim);
    setColour (juce::TabbedButtonBar::frontTextColourId, blue);
    setColour (juce::TooltipWindow::backgroundColourId, panelHi);
    setColour (juce::TooltipWindow::textColourId, text);

    setColour (juce::TextButton::buttonColourId, buttonBottom);
    setColour (juce::TextButton::buttonOnColourId, blueSoft);
    setColour (juce::TextButton::textColourOffId, text);
    setColour (juce::TextButton::textColourOnId, blue);
}

//==============================================================================
void FdLookAndFeel::setLed (juce::Button& button, juce::Colour ledColour)
{
    button.getProperties().set (ledId, (juce::int64) ledColour.getARGB());
}

void FdLookAndFeel::setFontSize (juce::Button& button, float size)
{
    button.getProperties().set (fontSizeId, (double) size);
}

bool FdLookAndFeel::hasLed (const juce::Button& button)
{
    return button.getProperties().contains (ledId);
}

juce::Colour FdLookAndFeel::getLedColour (const juce::Button& button)
{
    return juce::Colour ((juce::uint32) (juce::int64) button.getProperties().getWithDefault (ledId, (juce::int64) palette::blue.getARGB()));
}

void FdLookAndFeel::drawLed (juce::Graphics& g, juce::Rectangle<float> area, juce::Colour colour, bool on)
{
    if (on)
    {
        g.setGradientFill (juce::ColourGradient (colour.withAlpha (0.35f), area.getCentre(),
                                                 colour.withAlpha (0.0f), area.getCentre().translated (area.getWidth() * 1.6f, 0.0f), true));
        g.fillEllipse (area.expanded (area.getWidth() * 1.2f));
    }

    g.setColour (palette::outline);
    g.fillEllipse (area.expanded (1.0f));

    g.setGradientFill (juce::ColourGradient (on ? colour.brighter (0.35f) : palette::line,
                                              area.getX(), area.getY(),
                                              on ? colour.darker (0.15f) : palette::panel,
                                              area.getX(), area.getBottom(), false));
    g.fillEllipse (area);

    g.setColour (palette::panelHi.withAlpha (on ? 0.55f : 0.7f));
    g.fillEllipse (area.reduced (area.getWidth() * 0.22f).translated (0.0f, -area.getHeight() * 0.12f));
}

//==============================================================================
void FdLookAndFeel::drawButtonBackground (juce::Graphics& g, juce::Button& button, const juce::Colour&,
                                          bool highlighted, bool down)
{
    auto r = button.getLocalBounds().toFloat().reduced (0.5f);
    const float corner = 6.0f;
    const bool on = button.getToggleState();
    const bool led = hasLed (button);
    const bool big = button.getHeight() >= 36;

    auto top = palette::buttonTop;
    auto bottom = palette::buttonBottom;

    if (highlighted && button.isEnabled())
    {
        top = palette::buttonHover;
        bottom = palette::blueFaint;
    }

    if (down)
    {
        top = palette::buttonDown;
        bottom = palette::buttonBottom;
    }

    if (! button.isEnabled())
    {
        top = palette::panel.brighter (0.1f);
        bottom = palette::panel;
    }

    // ombre douce
    g.setColour (juce::Colours::black.withAlpha (0.06f));
    g.fillRoundedRectangle (r.translated (0.0f, 1.0f), corner);

    // corps
    g.setGradientFill (juce::ColourGradient (top, 0.0f, r.getY(), bottom, 0.0f, r.getBottom(), false));
    g.fillRoundedRectangle (r, corner);

    // reflet en haut : blanc en mode clair, gris clair en mode sombre.
    g.setColour (palette::panelHi.withAlpha (0.9f));
    g.drawLine (r.getX() + corner, r.getY() + 1.0f, r.getRight() - corner, r.getY() + 1.0f, 1.0f);
    g.setColour (palette::panelHi.withAlpha (down ? 0.05f : 0.4f));
    g.fillRoundedRectangle (r.withHeight (r.getHeight() * 0.44f).reduced (1.0f, 1.0f), corner - 1.0f);

    // état actif : teinte bleue
    if (on)
    {
        g.setColour (palette::blue.withAlpha (led && ! big ? 0.20f : 0.12f));
        g.fillRoundedRectangle (r, corner);
    }

    // contour
    g.setColour (on ? palette::blue : palette::outline);
    g.drawRoundedRectangle (r, corner, on ? 1.7f : 1.0f);

    // témoin lumineux
    if (led && big)
        drawLed (g, juce::Rectangle<float> (7.0f, 7.0f).withCentre ({ r.getCentreX(), r.getY() + 11.0f }),
                 getLedColour (button), on);
}

juce::Font FdLookAndFeel::getTextButtonFont (juce::TextButton& button, int buttonHeight)
{
    // Un bouton peut imposer sa taille (voir setFontSize) : utile pour le « + ».
    const auto custom = (float) (double) button.getProperties().getWithDefault (fontSizeId, 0.0);

    if (custom > 0.0f)
        return juce::Font (juce::FontOptions (custom, juce::Font::bold));

    return juce::Font (juce::FontOptions (juce::jlimit (10.0f, 14.5f, (float) buttonHeight * 0.36f), juce::Font::bold));
}

void FdLookAndFeel::drawButtonText (juce::Graphics& g, juce::TextButton& button, bool, bool down)
{
    g.setFont (getTextButtonFont (button, button.getHeight()));

    const bool on = button.getToggleState();
    g.setColour (! button.isEnabled() ? palette::textFaint
                                      : (on ? palette::blueBright : palette::text));

    auto area = button.getLocalBounds().reduced (3, 2);

    if (hasLed (button) && button.getHeight() >= 36)
        area.removeFromTop (12);

    if (down)
        area.translate (0, 1);

    g.drawFittedText (button.getButtonText(), area, juce::Justification::centred, 1);
}

//==============================================================================
void FdLookAndFeel::drawLinearSlider (juce::Graphics& g, int x, int y, int width, int height,
                                      float sliderPos, float minSliderPos, float maxSliderPos,
                                      juce::Slider::SliderStyle style, juce::Slider& slider)
{
    if (style != juce::Slider::LinearHorizontal)
    {
        LookAndFeel_V4::drawLinearSlider (g, x, y, width, height, sliderPos, minSliderPos, maxSliderPos, style, slider);
        return;
    }

    const float cy = (float) y + (float) height * 0.5f;
    juce::Rectangle<float> groove ((float) x + 5.0f, cy - 3.0f, (float) width - 10.0f, 6.0f);

    g.setColour (palette::outline.withAlpha (0.7f));
    g.fillRoundedRectangle (groove.expanded (1.0f), 4.0f);
    g.setColour (palette::line);
    g.fillRoundedRectangle (groove, 3.0f);

    const bool centred = slider.getMinimum() < 0.0 && slider.getMaximum() > 0.0;
    const float originX = centred ? groove.getCentreX() : groove.getX();
    const float fillA = juce::jmin (originX, sliderPos);
    const float fillB = juce::jmax (originX, sliderPos);

    g.setColour (palette::blue);
    g.fillRoundedRectangle (fillA, groove.getY(), fillB - fillA, groove.getHeight(), 3.0f);
    g.setColour (juce::Colours::white.withAlpha (0.35f));
    g.fillRoundedRectangle (fillA, groove.getY(), fillB - fillA, groove.getHeight() * 0.4f, 2.0f);

    if (centred)
    {
        g.setColour (palette::textDim);
        g.drawLine (groove.getCentreX(), groove.getY() - 3.0f, groove.getCentreX(), groove.getBottom() + 3.0f, 1.0f);
    }

    juce::Rectangle<float> cap (13.0f, 19.0f);
    cap.setCentre (sliderPos, cy);

    g.setColour (juce::Colours::black.withAlpha (0.18f));
    g.fillRoundedRectangle (cap.translated (0.0f, 1.0f), 4.0f);
    g.setGradientFill (juce::ColourGradient (palette::buttonTop, 0.0f, cap.getY(),
                                             palette::buttonBottom, 0.0f, cap.getBottom(), false));
    g.fillRoundedRectangle (cap, 4.0f);
    g.setColour (palette::outline);
    g.drawRoundedRectangle (cap, 4.0f, 1.0f);
    g.setColour (palette::blue);
    g.fillRoundedRectangle (cap.getCentreX() - 1.5f, cap.getY() + 3.0f, 3.0f, cap.getHeight() - 6.0f, 1.5f);
}

//==============================================================================
void FdLookAndFeel::drawComboBox (juce::Graphics& g, int width, int height, bool isButtonDown,
                                  int buttonX, int buttonY, int buttonW, int buttonH, juce::ComboBox&)
{
    auto r = juce::Rectangle<int> (0, 0, width, height).toFloat().reduced (0.5f);

    g.setColour (palette::panelHi);
    g.fillRoundedRectangle (r, 5.0f);
    g.setColour (palette::outline);
    g.drawRoundedRectangle (r, 5.0f, 1.0f);
    g.setColour (palette::panelHi.withAlpha (0.8f));
    g.drawLine (r.getX() + 5.0f, r.getY() + 1.0f, r.getRight() - 5.0f, r.getY() + 1.0f, 1.0f);

    auto arrow = juce::Rectangle<int> (buttonX, buttonY, buttonW, buttonH).toFloat();
    g.setColour (isButtonDown ? palette::blueSoft : palette::blueFaint);
    g.fillRoundedRectangle (arrow.reduced (2.0f), 4.0f);
    g.setColour (palette::blue);
    g.setFont (juce::Font (juce::FontOptions (11.0f, juce::Font::bold)));
    g.drawText (isButtonDown ? "-" : "+", arrow, juce::Justification::centred);
}

void FdLookAndFeel::drawTabButton (juce::TabBarButton& button, juce::Graphics& g, bool isMouseOver, bool isMouseDown)
{
    auto r = button.getLocalBounds().toFloat();
    const bool front = button.isFrontTab();

    if (front)
    {
        g.setColour (palette::panelHi);
        g.fillRoundedRectangle (r.withHeight (r.getHeight() + 3.0f), 6.0f);
        g.setColour (palette::blue);
        g.drawRoundedRectangle (r.withHeight (r.getHeight() + 3.0f), 6.0f, 1.3f);
    }
    else if (isMouseOver || isMouseDown)
    {
        g.setColour (palette::blueFaint);
        g.fillRoundedRectangle (r, 6.0f);
    }

    drawTabButtonText (button, g, isMouseOver, isMouseDown);
}

//==============================================================================
void FdLookAndFeel::drawTile (juce::Graphics& g, juce::Rectangle<float> r, bool selected, bool hovered)
{
    g.setColour (juce::Colours::black.withAlpha (0.08f));
    g.fillRoundedRectangle (r.translated (0.0f, 2.0f), 10.0f);

    g.setGradientFill (juce::ColourGradient (hovered ? palette::panelHi.brighter (0.08f) : palette::panelHi,
                                             0.0f, r.getY(),
                                             hovered ? palette::blueFaint : palette::panel,
                                             0.0f, r.getBottom(), false));
    g.fillRoundedRectangle (r, 10.0f);

    g.setColour (selected ? palette::blue : palette::outline);
    g.drawRoundedRectangle (r.reduced (0.5f), 10.0f, selected ? 2.0f : 1.0f);

    g.setColour (palette::panelHi.withAlpha (0.9f));
    g.drawLine (r.getX() + 10.0f, r.getY() + 1.0f, r.getRight() - 10.0f, r.getY() + 1.0f, 1.0f);
}
} // namespace fdgpe
