#include <juce_gui_basics/juce_gui_basics.h>

#include "ui/FdLookAndFeel.h"
#include "ui/MainWindow.h"

class FDgPEApplication final : public juce::JUCEApplication
{
public:
    const juce::String getApplicationName() override    { return "FD gPE"; }
    const juce::String getApplicationVersion() override { return FDGPE_VERSION; }
    bool moreThanOneInstanceAllowed() override          { return true; }

    void initialise (const juce::String&) override
    {
        juce::LookAndFeel::setDefaultLookAndFeel (&lookAndFeel);
        mainWindow = std::make_unique<fdgpe::MainWindow> (getApplicationName());
    }

    void shutdown() override
    {
        mainWindow.reset();
        juce::LookAndFeel::setDefaultLookAndFeel (nullptr);
    }

    void systemRequestedQuit() override
    {
        quit();
    }

private:
    fdgpe::FdLookAndFeel lookAndFeel;
    std::unique_ptr<fdgpe::MainWindow> mainWindow;
};

START_JUCE_APPLICATION (FDgPEApplication)
