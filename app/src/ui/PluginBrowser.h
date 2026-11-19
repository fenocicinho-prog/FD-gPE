#pragma once

#include <functional>
#include <memory>
#include <vector>

#include <juce_gui_basics/juce_gui_basics.h>

#include "../engine/EngineHost.h"

namespace fdgpe
{
/** Navigateur de plugins en tuiles, dans l'esprit de la fiche technique :
    recherche, filtres, scan VST3 et ajout en un clic sur la piste choisie.
    Les tuiles sont dessinées par le projet (aucune image externe requise) ;
    la tuile reste lisible même si aucun logo n'est fourni. */
class PluginBrowser final : public juce::Component
{
public:
    PluginBrowser (EngineHost& host,
                   juce::String targetTrackName,
                   std::function<void (const PluginInfo&)> onAdd,
                   std::function<void()> onClose);

    ~PluginBrowser() override;

    /** Ouvre le navigateur dans une fenêtre modale légère. Si \p track est
        nul, la liste reste consultable mais l'ajout est désactivé. */
    static void show (EngineHost& host, te::AudioTrack* track);

    void paint (juce::Graphics&) override;
    void resized() override;

    // Appelées par la grille de tuiles.
    void paintTiles (juce::Graphics&, juce::Rectangle<int> area);
    int getTileCount() const;
    juce::Rectangle<int> getTileBounds (int index) const;
    void tileHovered (int index);
    void tileClicked (int index, const juce::ModifierKeys&);
    void tileDoubleClicked (int index);

private:
    class TileCanvas;

    void refreshPluginList();
    void applyFilters();
    void scanPlugins();
    void chooseFolder();
    void addSelected();
    void selectIndex (int index);
    void updateButtons();
    juce::String categoryFor (const PluginInfo&) const;

    EngineHost& host;
    juce::String trackName;
    std::function<void (const PluginInfo&)> onAdd;
    std::function<void()> onClose;

    juce::TextEditor search;
    juce::ComboBox filter;
    juce::TextButton scanButton { "SCANNER VST3" }, folderButton { "DOSSIER…" },
                     addButton { "AJOUTER À LA PISTE" }, closeButton { "FERMER" };
    juce::Label titleLabel, targetLabel, selectedLabel, statusLabel;

    juce::Viewport viewport;
    std::unique_ptr<TileCanvas> canvas;

    std::vector<PluginInfo> plugins;
    std::vector<int> visible;
    int selected = -1;
    int hovered = -1;
    int columns = 1;
    std::unique_ptr<juce::FileChooser> folderChooser;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PluginBrowser)
};
} // namespace fdgpe
