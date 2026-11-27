juce_add_gui_app(FDgPE
    PRODUCT_NAME "FDgPE"
    COMPANY_NAME "FD gPE contributors"
    BUNDLE_ID "org.fdgpe.app"
    ICON_BIG "${CMAKE_CURRENT_SOURCE_DIR}/resources/icon-512.png"
    ICON_SMALL "${CMAKE_CURRENT_SOURCE_DIR}/resources/icon-256.png"
    MICROPHONE_PERMISSION_ENABLED TRUE
    MICROPHONE_PERMISSION_TEXT "FD gPE enregistre l'audio de vos pistes.")

juce_add_binary_data(FDgPE_assets
    NAMESPACE fdgpe_assets
    SOURCES
        resources/icon-512.png
        resources/icon-256.png)

target_sources(FDgPE PRIVATE
    src/Main.cpp
    src/engine/EngineHost.h
    src/engine/EngineHost.cpp
    src/ui/Palette.h
    src/ui/Text.h
    src/ui/FdLookAndFeel.h
    src/ui/FdLookAndFeel.cpp
    src/ui/TimelineModel.h
    src/ui/TimelineRuler.h
    src/ui/TimelineRuler.cpp
    src/ui/TrackRow.h
    src/ui/TrackRow.cpp
    src/ui/TransportBar.h
    src/ui/TransportBar.cpp
    src/ui/PluginBrowser.h
    src/ui/PluginBrowser.cpp
    src/ui/PianoRollEditor.h
    src/ui/PianoRollEditor.cpp
    src/ui/TempoDialog.h
    src/ui/TempoDialog.cpp
    src/ui/MixerDock.h
    src/ui/MixerDock.cpp
    src/ui/MainComponent.h
    src/ui/MainComponent.cpp
    src/ui/MainWindow.h)

target_compile_features(FDgPE PRIVATE cxx_std_20)

target_compile_definitions(FDgPE PRIVATE
    FDGPE_VERSION="${PROJECT_VERSION}"
    JUCE_PLUGINHOST_VST3=1          # LV2 viendra au jalon M3
    JUCE_USE_CURL=0                 # réseau : jalon M5
    JUCE_WEB_BROWSER=0
    JUCE_MODAL_LOOPS_PERMITTED=1
    JUCE_STRICT_REFCOUNTEDPOINTER=1
    TRACKTION_ENABLE_TIMESTRETCH_SOUNDTOUCH=1)

target_link_libraries(FDgPE PRIVATE
    FDgPE_assets
    tracktion::tracktion_core
    tracktion::tracktion_engine
    tracktion::tracktion_graph
    juce::juce_audio_devices
    juce::juce_audio_processors
    juce::juce_audio_utils
    juce::juce_gui_basics
    juce::juce_recommended_config_flags
    juce::juce_recommended_warning_flags)

# GCC/Clang sous Linux : std::atomic sur une structure non triviale (Tracktion)
# a besoin de libatomic à l'édition de liens.
if(UNIX AND NOT APPLE)
    target_link_libraries(FDgPE PRIVATE atomic)
endif()
