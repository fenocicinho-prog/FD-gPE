#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

#include "Palette.h"

namespace fdgpe
{
/** Apparence de FD gPE : gris clair brillant façon navigateur, accents bleus,
    boutons "chrome" avec reflet et petits témoins lumineux. */
class FdLookAndFeel final : public juce::LookAndFeel_V4
{
public:
    FdLookAndFeel();

    /** Réapplique les couleurs courantes de la palette après un changement de thème. */
    void refreshPalette();

    void drawButtonBackground (juce::Graphics&, juce::Button&, const juce::Colour&,
                               bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown) override;

    void drawButtonText (juce::Graphics&, juce::TextButton&,
                         bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown) override;

    juce::Font getTextButtonFont (juce::TextButton&, int buttonHeight) override;

    void drawLinearSlider (juce::Graphics&, int x, int y, int width, int height,
                           float sliderPos, float minSliderPos, float maxSliderPos,
                           juce::Slider::SliderStyle, juce::Slider&) override;

    int getSliderThumbRadius (juce::Slider&) override { return 8; }

    void drawComboBox (juce::Graphics&, int width, int height, bool isButtonDown,
                       int buttonX, int buttonY, int buttonW, int buttonH, juce::ComboBox&) override;

    void drawTabButton (juce::TabBarButton&, juce::Graphics&, bool isMouseOver, bool isMouseDown) override;

    /** Pose cette propriété sur un bouton pour lui donner un témoin lumineux. */
    static void setLed (juce::Button&, juce::Colour ledColour);

    /** Force la taille de police d'un bouton (par exemple le bouton « + »). */
    static void setFontSize (juce::Button&, float size);

    /** Dessine une tuile (fond clair, bordure, ombre) utilisée par le navigateur de plugins. */
    static void drawTile (juce::Graphics&, juce::Rectangle<float>, bool selected, bool hovered);

private:
    static void drawLed (juce::Graphics&, juce::Rectangle<float>, juce::Colour, bool on);
    static bool hasLed (const juce::Button&);
    static juce::Colour getLedColour (const juce::Button&);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (FdLookAndFeel)
};
} // namespace fdgpe
