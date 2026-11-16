#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

#include "MainComponent.h"

namespace fdgpe
{
class MainWindow final : public juce::DocumentWindow
{
public:
    explicit MainWindow (const juce::String& name)
        : juce::DocumentWindow (name,
                                juce::Colour (0xff15171c),
                                juce::DocumentWindow::allButtons)
    {
        setUsingNativeTitleBar (true);
        setContentOwned (new MainComponent(), true);
        setResizable (true, true);
        setResizeLimits (760, 480, 4096, 4096);
        centreWithSize (getWidth(), getHeight());
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
