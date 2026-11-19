#pragma once

#include <juce_core/juce_core.h>

namespace fdgpe
{
/** Les textes de l'interface sont écrits en UTF-8 (accents). */
inline juce::String tr (const char* utf8)
{
    return juce::String::fromUTF8 (utf8);
}
}
