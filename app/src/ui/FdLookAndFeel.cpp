#include "FdLookAndFeel.h"

#include <cmath>

namespace fdgpe
{
namespace
{
    const juce::Identifier ledId { "fdLed" };
    const juce::Identifier fontSizeId { "fdFontSize" };
    const juce::Identifier symbolId { "fdSymbol" };
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
    setColour (juce::Label::backgroundColourId, juce::Colours::transparentBlack);
    setColour (juce::Label::backgroundWhenEditingColourId, panelHi);
    setColour (juce::Label::outlineColourId, juce::Colours::transparentBlack);
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
    setColour (juce::PopupMenu::highlightedTextColourId, text.getBrightness() > 0.5f ? bgDeep : juce::Colours::white);
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

void FdLookAndFeel::setSymbol (juce::Button& button, ButtonSymbol symbol)
{
    button.getProperties().set (symbolId, (int) symbol);
}

bool FdLookAndFeel::hasLed (const juce::Button& button)
{
    return button.getProperties().contains (ledId);
}

juce::Colour FdLookAndFeel::getLedColour (const juce::Button& button)
{
    return juce::Colour ((juce::uint32) (juce::int64) button.getProperties().getWithDefault (ledId, (juce::int64) palette::blue.getARGB()));
}

FdLookAndFeel::ButtonSymbol FdLookAndFeel::getSymbol (const juce::Button& button)
{
    return (ButtonSymbol) (int) button.getProperties().getWithDefault (symbolId, (int) ButtonSymbol::none);
}

void FdLookAndFeel::drawLed (juce::Graphics& g, juce::Rectangle<float> area, juce::Colour colour, bool on)
{
    // Témoin plat : un point plein allumé, un anneau éteint.
    if (on)
    {
        g.setColour (colour.withAlpha (0.25f));
        g.fillEllipse (area.expanded (3.0f));
        g.setColour (colour);
        g.fillEllipse (area);
    }
    else
    {
        g.setColour (palette::outline);
        g.drawEllipse (area.reduced (0.5f), 1.0f);
    }
}

//==============================================================================
void FdLookAndFeel::drawButtonBackground (juce::Graphics& g, juce::Button& button, const juce::Colour&,
                                          bool highlighted, bool down)
{
    auto r = button.getLocalBounds().toFloat().reduced (0.5f);
    const float corner = 5.0f;
    const bool on = button.getToggleState();
    const bool led = hasLed (button);
    const bool big = button.getHeight() >= 36;
    const bool iconOnly = getSymbol (button) != ButtonSymbol::none;
    const auto onColour = led ? getLedColour (button) : palette::blue;

    // Boutons-icônes (lecteur vidéo) : aucun fond au repos, seulement au survol ou quand ils sont actifs.
    if (iconOnly)
    {
        if (down)
        {
            g.setColour (palette::buttonDown);
            g.fillRoundedRectangle (r, corner);
        }
        else if (highlighted && button.isEnabled())
        {
            g.setColour (palette::buttonHover);
            g.fillRoundedRectangle (r, corner);
        }

        if (on)
        {
            g.setColour (onColour.withAlpha (0.28f));
            g.fillRoundedRectangle (r, corner);
        }

        return;
    }

    auto fill = palette::buttonBottom;

    if (highlighted && button.isEnabled())
        fill = palette::buttonHover;

    if (down)
        fill = palette::buttonDown;

    if (! button.isEnabled())
        fill = palette::panel;

    // Aplat sans contour, comme sur la maquette.
    g.setColour (fill);
    g.fillRoundedRectangle (r, corner);

    if (on)
    {
        g.setColour (onColour.withAlpha (0.30f));
        g.fillRoundedRectangle (r, corner);
    }

    if (led && big)
        drawLed (g, juce::Rectangle<float> (6.0f, 6.0f).withCentre ({ r.getCentreX(), r.getY() + 10.0f }),
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
    auto area = button.getLocalBounds().reduced (3, 2);

    if (hasLed (button) && button.getHeight() >= 36)
        area.removeFromTop (12);

    if (down)
        area.translate (0, 1);

    const auto symbol = getSymbol (button);

    if (symbol != ButtonSymbol::none)
    {
        // Icônes de lecteur vidéo : formes pleines, nettes, sans contour.
        const float side = juce::jmin ((float) area.getWidth(), (float) area.getHeight()) * 0.66f;
        auto box = juce::Rectangle<float> (side, side).withCentre (area.toFloat().getCentre());
        const float w = box.getWidth();
        const float h = box.getHeight();

        juce::Colour colour = palette::textFaint;

        if (button.isEnabled())
            colour = (symbol == ButtonSymbol::rec) ? palette::red : palette::text;

        g.setColour (colour);

        switch (symbol)
        {
            case ButtonSymbol::play:
            {
                const float dx = w * 0.06f;   // le triangle paraît plus centré ainsi
                juce::Path p;
                p.addTriangle (box.getX() + dx, box.getY(), box.getRight() + dx, box.getCentreY(),
                               box.getX() + dx, box.getBottom());
                g.fillPath (p);
                break;
            }

            case ButtonSymbol::rec:
            {
                g.fillEllipse (box);
                break;
            }

            case ButtonSymbol::skipBack:
            {
                const float bar = juce::jmax (2.0f, w * 0.16f);
                g.fillRect (box.getX(), box.getY(), bar, h);

                juce::Path p;
                p.addTriangle (box.getRight(), box.getY(), box.getX() + bar + 1.5f, box.getCentreY(),
                               box.getRight(), box.getBottom());
                g.fillPath (p);
                break;
            }

            case ButtonSymbol::loop:
            {
                constexpr float pi = 3.14159265358979323846f;
                const float r = w * 0.5f - 1.0f;
                const auto c = box.getCentre();

                // Deux arcs fléchés qui se suivent : l'icône « répéter » des lecteurs.
                auto arrowArc = [&] (float from, float to)
                {
                    juce::Path arc;
                    arc.addCentredArc (c.x, c.y, r, r, 0.0f, from, to, true);
                    g.strokePath (arc, juce::PathStrokeType (2.0f));

                    const float tx = std::cos (to), ty = std::sin (to);   // tangente (sens horaire)
                    const float px = c.x + r * std::sin (to), py = c.y - r * std::cos (to);
                    const float len = w * 0.30f, half = w * 0.20f;
                    const float bx = px - tx * len * 0.35f, by = py - ty * len * 0.35f;

                    juce::Path head;
                    head.addTriangle (px + tx * len * 0.65f, py + ty * len * 0.65f,
                                      bx - ty * half, by + tx * half,
                                      bx + ty * half, by - tx * half);
                    g.fillPath (head);
                };

                arrowArc (0.15f * pi, 0.80f * pi);
                arrowArc (1.15f * pi, 1.80f * pi);
                break;
            }

            case ButtonSymbol::none:
                break;
        }

        return;
    }

    g.setFont (getTextButtonFont (button, button.getHeight()));
    g.setColour (! button.isEnabled() ? palette::textFaint
                                      : (button.getToggleState() ? palette::blueBright : palette::text));

    g.drawFittedText (button.getButtonText(), area, juce::Justification::centred, 1);
}

//==============================================================================
void FdLookAndFeel::drawLinearSlider (juce::Graphics& g, int x, int y, int width, int height,
                                      float sliderPos, float minSliderPos, float maxSliderPos,
                                      juce::Slider::SliderStyle style, juce::Slider& slider)
{
    if (style == juce::Slider::LinearVertical)
    {
        const float cx = (float) x + (float) width * 0.5f;
        juce::Rectangle<float> groove (cx - 1.5f, (float) y + 5.0f, 3.0f, (float) height - 10.0f);

        g.setColour (palette::line);
        g.fillRoundedRectangle (groove, 1.5f);
        g.setColour (palette::blue);
        g.fillRoundedRectangle (groove.getX(), sliderPos, groove.getWidth(), juce::jmax (0.0f, groove.getBottom() - sliderPos), 1.5f);

        g.setColour (palette::text);
        g.fillRoundedRectangle (juce::Rectangle<float> (30.0f, 14.0f).withCentre ({ cx, sliderPos }), 2.0f);
        return;
    }

    if (style != juce::Slider::LinearHorizontal)
    {
        LookAndFeel_V4::drawLinearSlider (g, x, y, width, height, sliderPos, minSliderPos, maxSliderPos, style, slider);
        return;
    }

    const float cy = (float) y + (float) height * 0.5f;
    juce::Rectangle<float> groove ((float) x + 5.0f, cy - 1.5f, (float) width - 10.0f, 3.0f);

    g.setColour (palette::line);
    g.fillRoundedRectangle (groove, 1.5f);

    const bool centred = slider.getMinimum() < 0.0 && slider.getMaximum() > 0.0;
    const float originX = centred ? groove.getCentreX() : groove.getX();
    const float fillA = juce::jmin (originX, sliderPos);
    const float fillB = juce::jmax (originX, sliderPos);

    g.setColour (palette::blue);
    g.fillRoundedRectangle (fillA, groove.getY(), fillB - fillA, groove.getHeight(), 1.5f);

    if (centred)
    {
        g.setColour (palette::textFaint);
        g.drawLine (groove.getCentreX(), groove.getY() - 3.0f, groove.getCentreX(), groove.getBottom() + 3.0f, 1.0f);
    }

    // Poignée : petit disque plat.
    g.setColour (palette::text);
    g.fillEllipse (juce::Rectangle<float> (10.0f, 10.0f).withCentre ({ sliderPos, cy }));
}

//==============================================================================
void FdLookAndFeel::drawComboBox (juce::Graphics& g, int width, int height, bool isButtonDown,
                                  int buttonX, int buttonY, int buttonW, int buttonH, juce::ComboBox&)
{
    auto r = juce::Rectangle<int> (0, 0, width, height).toFloat().reduced (0.5f);

    g.setColour (palette::panelHi);
    g.fillRoundedRectangle (r, 4.0f);
    g.setColour (palette::line);
    g.drawRoundedRectangle (r, 4.0f, 1.0f);

    auto arrow = juce::Rectangle<int> (buttonX, buttonY, buttonW, buttonH).toFloat();
    g.setColour (isButtonDown ? palette::blueBright : palette::blue);

    juce::Path p;
    const auto c = arrow.getCentre();
    p.addTriangle (c.x - 4.0f, c.y - 2.0f, c.x + 4.0f, c.y - 2.0f, c.x, c.y + 3.0f);
    g.fillPath (p);
}

void FdLookAndFeel::drawTabButton (juce::TabBarButton& button, juce::Graphics& g, bool isMouseOver, bool isMouseDown)
{
    auto r = button.getLocalBounds().toFloat();
    const bool front = button.isFrontTab();

    if (front)
    {
        g.setColour (palette::panelHi);
        g.fillRect (r);
        g.setColour (palette::blue);
        g.fillRect (r.removeFromBottom (2.0f));
    }
    else if (isMouseOver || isMouseDown)
    {
        g.setColour (palette::buttonHover);
        g.fillRect (r);
    }

    drawTabButtonText (button, g, isMouseOver, isMouseDown);
}

//==============================================================================
void FdLookAndFeel::drawTile (juce::Graphics& g, juce::Rectangle<float> r, bool selected, bool hovered)
{
    g.setColour (hovered ? palette::buttonHover : palette::panelHi);
    g.fillRoundedRectangle (r, 6.0f);

    g.setColour (selected ? palette::blue : palette::line);
    g.drawRoundedRectangle (r.reduced (0.5f), 6.0f, selected ? 1.6f : 1.0f);
}
} // namespace fdgpe
