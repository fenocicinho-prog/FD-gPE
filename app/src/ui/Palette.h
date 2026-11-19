#pragma once

#include <juce_graphics/juce_graphics.h>

namespace fdgpe::palette
{
// ==============================================================================
// Thème "Firefox" : gris clair légèrement brillant + accent bleu.
// Les surfaces sont très claires, avec un reflet en haut et une ombre douce,
// comme la barre d'outils d'un navigateur moderne. Le bleu est réservé aux
// actions, à la sélection et à la tête de lecture.
// ==============================================================================

// Fonds et panneaux
inline const juce::Colour bgTop        { 0xfffafafd };
inline const juce::Colour bgBottom     { 0xffe9ebef };
inline const juce::Colour panel        { 0xfff6f7f9 };
inline const juce::Colour panelHi      { 0xffffffff };
inline const juce::Colour lane         { 0xfffdfdff };
inline const juce::Colour laneAlt      { 0xfff4f6f9 };
inline const juce::Colour line         { 0xffd5d8df };
inline const juce::Colour outline      { 0xffb9bec9 };

// Textes
inline const juce::Colour text         { 0xff1b1b23 };
inline const juce::Colour textDim      { 0xff606574 };
inline const juce::Colour textFaint    { 0xff9aa0ad };

// Boutons : corps blanc/gris avec reflet en haut (aspect "chrome")
inline const juce::Colour buttonTop    { 0xffffffff };
inline const juce::Colour buttonBottom { 0xffe1e4ea };
inline const juce::Colour buttonHover  { 0xfff2f7ff };
inline const juce::Colour buttonDown   { 0xffd2d9e6 };

// Accent bleu (Firefox blue)
inline const juce::Colour blue         { 0xff0060df };
inline const juce::Colour blueBright   { 0xff3b92ff };
inline const juce::Colour blueSoft     { 0xffcfe3ff };
inline const juce::Colour blueFaint    { 0xffeaf3ff };

// Anciens noms conservés comme alias pour compatibilité avec le code existant.
inline const juce::Colour violet       = blue;
inline const juce::Colour violetSoft   = blueSoft;

// Témoins lumineux
inline const juce::Colour red          { 0xffe5343a };
inline const juce::Colour amber        { 0xffffa528 };
inline const juce::Colour green        { 0xff12b886 };

// Couleurs des clips
inline const juce::Colour audioClip    { 0xffd8e6ff };
inline const juce::Colour audioClipHi  { 0xff9dc2ff };
inline const juce::Colour midiClip     { 0xffd9f5ea };
inline const juce::Colour midiClipHi   { 0xff63d6a4 };
}
