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
    juce::Colour blue         { 0xffc99a2f };
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
            // Gris neutres (inspirés du thème sombre de Firefox), jamais noirs :
            // les textes et les boutons restent lisibles sur toute la surface.
            bgTop        = c (0xff23222b);
            bgBottom     = c (0xff1c1b22);
            panel        = c (0xff2b2a33);
            panelHi      = c (0xff38373f);
            lane         = c (0xff24232c);
            laneAlt      = c (0xff2e2d36);
            line         = c (0xff494857);
            outline      = c (0xff5a5968);
            text         = c (0xfff9f9ff);
            textDim      = c (0xffc6c5cf);
            textFaint    = c (0xff9d9caa);
            buttonTop    = c (0xff4b4a56);
            buttonBottom = c (0xff38373f);
            buttonHover  = c (0xff585764);
            buttonDown   = c (0xff2c2b34);
            midiClip     = c (0xff25483f);
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
        // Variante lisible pour les textes : plus claire en sombre, plus foncée en clair.
        blueBright  = dark ? accent.withBrightness (juce::jmin (1.0f, accent.getBrightness() + 0.2f))
                           : accent.darker (0.25f);
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
