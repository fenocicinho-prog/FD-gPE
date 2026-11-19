#pragma once

#include <juce_graphics/juce_graphics.h>

namespace fdgpe::palette
{
// ==============================================================================
// Thème de couleurs modifiable à chaud (M2.7).
// Les noms historiques restent identiques : ce sont des références vers l'état
// global `theme`, ainsi tous les `palette::*` existants suivent le thème sans
// qu'il faille modifier les 186 appelants.
// Le violet est réservé aux bordures et aux petits témoins ; l'accent reste
// configurable par l'utilisateur (préférences, sans recompilation).
// ==============================================================================

struct Theme
{
    // Fonds et panneaux
    juce::Colour bgTop        { 0xfffafafd };
    juce::Colour bgBottom     { 0xffe9ebef };
    juce::Colour panel        { 0xfff6f7f9 };
    juce::Colour panelHi      { 0xffffffff };
    juce::Colour lane         { 0xfffdfdff };
    juce::Colour laneAlt      { 0xfff4f6f9 };
    juce::Colour line         { 0xffd5d8df };
    juce::Colour outline      { 0xffb9bec9 };

    // Textes
    juce::Colour text         { 0xff1b1b23 };
    juce::Colour textDim      { 0xff606574 };
    juce::Colour textFaint    { 0xff9aa0ad };

    // Boutons
    juce::Colour buttonTop    { 0xffffffff };
    juce::Colour buttonBottom { 0xffe1e4ea };
    juce::Colour buttonHover  { 0xfff2f7ff };
    juce::Colour buttonDown   { 0xffd2d9e6 };

    // Accent
    juce::Colour blue         { 0xff0060df };
    juce::Colour blueBright   { 0xff3b92ff };
    juce::Colour blueSoft     { 0xffcfe3ff };
    juce::Colour blueFaint    { 0xffeaf3ff };

    // Violet des bordures et témoins (dérivé de l'accent)
    juce::Colour violet       { 0xff8b5cf6 };
    juce::Colour violetSoft   { 0xffe4d9ff };

    // Témoins
    juce::Colour red          { 0xffe5343a };
    juce::Colour amber        { 0xffffa528 };
    juce::Colour green        { 0xff12b886 };

    // Clips
    juce::Colour audioClip    { 0xffd8e6ff };
    juce::Colour audioClipHi  { 0xff9dc2ff };
    juce::Colour midiClip     { 0xffd9f5ea };
    juce::Colour midiClipHi   { 0xff63d6a4 };

    static juce::Colour c (juce::uint32 argb) noexcept  { return juce::Colour (argb); }

    void apply (bool dark, const juce::Colour& accent)
    {
        if (dark)
        {
            bgTop        = c (0xff10121a);
            bgBottom     = c (0xff181b26);
            panel        = c (0xff1d2130);
            panelHi      = c (0xff282d3f);
            lane         = c (0xff12151f);
            laneAlt      = c (0xff1a1e2b);
            line         = c (0xff33394d);
            outline      = c (0xff4a5168);
            text         = c (0xfff0f2f8);
            textDim      = c (0xffb0b6c8);
            textFaint    = c (0xff7b8298);
            buttonTop    = c (0xff2a3044);
            buttonBottom = c (0xff1d2231);
            buttonHover  = c (0xff353c53);
            buttonDown   = c (0xff141824);
            midiClip     = c (0xff163a30);
        }
        else
        {
            bgTop        = c (0xfffafafd);
            bgBottom     = c (0xffe9ebef);
            panel        = c (0xfff6f7f9);
            panelHi      = c (0xffffffff);
            lane         = c (0xfffdfdff);
            laneAlt      = c (0xfff4f6f9);
            line         = c (0xffd5d8df);
            outline      = c (0xffb9bec9);
            text         = c (0xff1b1b23);
            textDim      = c (0xff606574);
            textFaint    = c (0xff9aa0ad);
            buttonTop    = c (0xffffffff);
            buttonBottom = c (0xffe1e4ea);
            buttonHover  = c (0xfff2f7ff);
            buttonDown   = c (0xffd2d9e6);
            midiClip     = c (0xffd9f5ea);
        }

        blue        = accent;
        blueBright  = accent.withBrightness (juce::jmin (1.0f, accent.getBrightness() + 0.2f));
        blueSoft    = accent.withSaturationHSL (0.25f).withBrightness (dark ? 0.55f : 0.92f);
        blueFaint   = accent.withSaturationHSL (0.12f).withBrightness (dark ? 0.30f : 0.97f);
        violet      = accent.withHue (0.75f).withSaturationHSL (0.70f);
        violetSoft  = violet.withSaturationHSL (0.25f).withBrightness (dark ? 0.55f : 0.92f);
        audioClip   = blueSoft;
        audioClipHi = blue.withAlpha (0.65f);
        midiClipHi  = green.withAlpha (dark ? 0.45f : 0.65f);
    }
};

inline Theme theme;

inline juce::Colour& bgTop        = theme.bgTop;
inline juce::Colour& bgBottom     = theme.bgBottom;
inline juce::Colour& panel        = theme.panel;
inline juce::Colour& panelHi      = theme.panelHi;
inline juce::Colour& lane         = theme.lane;
inline juce::Colour& laneAlt      = theme.laneAlt;
inline juce::Colour& line         = theme.line;
inline juce::Colour& outline      = theme.outline;
inline juce::Colour& text         = theme.text;
inline juce::Colour& textDim      = theme.textDim;
inline juce::Colour& textFaint    = theme.textFaint;
inline juce::Colour& buttonTop    = theme.buttonTop;
inline juce::Colour& buttonBottom = theme.buttonBottom;
inline juce::Colour& buttonHover  = theme.buttonHover;
inline juce::Colour& buttonDown   = theme.buttonDown;
inline juce::Colour& blue         = theme.blue;
inline juce::Colour& blueBright   = theme.blueBright;
inline juce::Colour& blueSoft     = theme.blueSoft;
inline juce::Colour& blueFaint    = theme.blueFaint;
inline juce::Colour& violet       = theme.violet;
inline juce::Colour& violetSoft   = theme.violetSoft;
inline juce::Colour& red          = theme.red;
inline juce::Colour& amber        = theme.amber;
inline juce::Colour& green        = theme.green;
inline juce::Colour& audioClip    = theme.audioClip;
inline juce::Colour& audioClipHi  = theme.audioClipHi;
inline juce::Colour& midiClip     = theme.midiClip;
inline juce::Colour& midiClipHi   = theme.midiClipHi;
} // namespace fdgpe::palette
