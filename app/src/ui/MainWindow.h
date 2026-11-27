#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <BinaryData.h>

#include "MainComponent.h"
#include "Palette.h"

namespace fdgpe
{
class MainWindow final : public juce::DocumentWindow
{
public:
    explicit MainWindow (const juce::String& name)
        : juce::DocumentWindow (name,
                                palette::bgBottom,
                                juce::DocumentWindow::allButtons)
    {
        setUsingNativeTitleBar (true);
        setContentOwned (new MainComponent(), true);
        setResizable (true, true);
        setResizeLimits (1040, 600, 4096, 4096);
        centreWithSize (getWidth(), getHeight());

        const auto icon = juce::ImageCache::getFromMemory (fdgpe_assets::icon512_png,
                                                           (int) fdgpe_assets::icon512_pngSize);
        if (icon.isValid())
            setIcon (icon);

        setVisible (true);
    }

    void closeButtonPressed() override
    {
        juce::JUCEApplication::getInstance()->systemRequestedQuit();
    }

private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainWindow)
};
} // namespace fdgpe
