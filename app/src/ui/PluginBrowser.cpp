#include "PluginBrowser.h"

#include "FdLookAndFeel.h"
#include "Palette.h"
#include "Text.h"

namespace fdgpe
{
namespace
{
    constexpr int tileWidth = 178;
    constexpr int tileHeight = 92;
    constexpr int tileGap = 10;
    constexpr int gridPadding = 10;

    juce::Colour colourForCategory (const PluginInfo& info)
    {
        if (info.instrument)
            return palette::blue;

        if (info.category.containsIgnoreCase ("reverb") || info.category.containsIgnoreCase ("réverb"))
            return juce::Colour (0xff2f9e8f);

        if (info.category.containsIgnoreCase ("delay") || info.category.containsIgnoreCase ("délai"))
            return juce::Colour (0xff8e6bff);

        if (info.category.containsIgnoreCase ("compress"))
            return juce::Colour (0xffe07a2f);

        return palette::blueBright;
    }

    juce::String initialsFor (const juce::String& name)
    {
        juce::String result;
        juce::StringArray words;
        words.addTokens (name, " -_/", "");
        words.removeEmptyStrings();

        for (const auto& word : words)
            if (result.length() < 2)
                result += word.substring (0, 1).toUpperCase();

        return result.isEmpty() ? "PL" : result;
    }
}

//==============================================================================
class PluginBrowser::TileCanvas final : public juce::Component
{
public:
    explicit TileCanvas (PluginBrowser& o) : owner (o) {}

    void paint (juce::Graphics& g) override
    {
        owner.paintTiles (g, getLocalBounds());
    }

    void mouseMove (const juce::MouseEvent& e) override
    {
        int found = -1;

        for (int i = 0; i < owner.getTileCount(); ++i)
            if (owner.getTileBounds (i).contains (e.getPosition()))
                found = i;

        owner.tileHovered (found);
    }

    void mouseExit (const juce::MouseEvent&) override
    {
        owner.tileHovered (-1);
    }

    void mouseDown (const juce::MouseEvent& e) override
    {
        for (int i = 0; i < owner.getTileCount(); ++i)
            if (owner.getTileBounds (i).contains (e.getPosition()))
            {
                owner.tileClicked (i, e.mods);
                return;
            }
    }

    void mouseDoubleClick (const juce::MouseEvent& e) override
    {
        for (int i = 0; i < owner.getTileCount(); ++i)
            if (owner.getTileBounds (i).contains (e.getPosition()))
            {
                owner.tileDoubleClicked (i);
                return;
            }
    }

private:
    PluginBrowser& owner;
};

//==============================================================================
PluginBrowser::PluginBrowser (EngineHost& h, juce::String target,
                              std::function<void (const PluginInfo&)> add,
                              std::function<void()> close)
    : host (h), trackName (std::move (target)), onAdd (std::move (add)), onClose (std::move (close))
{
    titleLabel.setText (tr ("Ajouter un plugin"), juce::dontSendNotification);
    titleLabel.setFont (juce::Font (juce::FontOptions (20.0f, juce::Font::bold)));
    titleLabel.setColour (juce::Label::textColourId, palette::text);

    targetLabel.setText (tr ("Piste cible : ") + (trackName.isEmpty() ? tr ("sélectionne une piste") : trackName),
                         juce::dontSendNotification);
    targetLabel.setFont (juce::Font (juce::FontOptions (11.5f, juce::Font::plain)));
    targetLabel.setColour (juce::Label::textColourId, palette::textDim);

    selectedLabel.setText (tr ("Choisis une tuile, puis Ajouter."), juce::dontSendNotification);
    selectedLabel.setFont (juce::Font (juce::FontOptions (12.0f, juce::Font::plain)));
    selectedLabel.setColour (juce::Label::textColourId, palette::textDim);

    statusLabel.setFont (juce::Font (juce::FontOptions (11.0f, juce::Font::plain)));
    statusLabel.setColour (juce::Label::textColourId, palette::blue);
    statusLabel.setJustificationType (juce::Justification::centredRight);

    search.setTextToShowWhenEmpty (tr ("Rechercher un plugin…"), palette::textFaint);
    search.setFont (juce::Font (juce::FontOptions (13.0f)));
    search.onTextChange = [this] { applyFilters(); };
    search.setColour (juce::TextEditor::outlineColourId, palette::outline);
    search.setColour (juce::TextEditor::focusedOutlineColourId, palette::blue);

    filter.addItem (tr ("Tous"), 1);
    filter.addItem (tr ("Instruments"), 2);
    filter.addItem (tr ("Effets"), 3);
    filter.addItem (tr ("Intégrés"), 4);
    filter.addItem (tr ("VST3 / externes"), 5);
    filter.setSelectedId (1, juce::dontSendNotification);
    filter.onChange = [this] { applyFilters(); };

    scanButton.onClick = [this] { scanPlugins(); };
    folderButton.onClick = [this] { chooseFolder(); };
    addButton.onClick = [this] { addSelected(); };
    unblockButton.onClick = [this] { unblockSelected(); };
    unblockButton.setTooltip (tr ("Retirer ce plugin de la liste noire, puis relancer le scan."));
    closeButton.onClick = [this]
    {
        if (onClose)
            onClose();

        if (auto* window = findParentComponentOfClass<juce::DialogWindow>())
            window->closeButtonPressed();
    };

    canvas = std::make_unique<TileCanvas> (*this);
    viewport.setViewedComponent (canvas.get(), false);
    viewport.setScrollBarsShown (true, false);
    viewport.setScrollBarThickness (10);

    for (juce::Component* c : { (juce::Component*) &titleLabel, (juce::Component*) &targetLabel,
                                (juce::Component*) &search, (juce::Component*) &filter,
                                (juce::Component*) &scanButton, (juce::Component*) &folderButton,
                                (juce::Component*) &addButton, (juce::Component*) &unblockButton,
                                 (juce::Component*) &closeButton,
                                (juce::Component*) &selectedLabel, (juce::Component*) &statusLabel,
                                (juce::Component*) &viewport })
        addAndMakeVisible (c);

    refreshPluginList();
    updateButtons();
    setSize (860, 620);
}

PluginBrowser::~PluginBrowser() = default;

//==============================================================================
void PluginBrowser::refreshPluginList()
{
    plugins = host.getAvailablePlugins();
    applyFilters();
}

void PluginBrowser::applyFilters()
{
    visible.clear();

    const int filterId = filter.getSelectedId();
    const auto query = search.getText().trim();

    for (int i = 0; i < (int) plugins.size(); ++i)
    {
        const auto& info = plugins[(size_t) i];

        if (filterId == 2 && ! info.instrument)  continue;
        if (filterId == 3 && info.instrument)    continue;
        if (filterId == 4 && ! info.builtIn)     continue;
        if (filterId == 5 && info.builtIn)       continue;

        if (query.isNotEmpty())
        {
            const auto haystack = info.name + " " + info.category + " " + info.manufacturer;
            if (! haystack.containsIgnoreCase (query))
                continue;
        }

        visible.push_back (i);
    }

    selected = -1;
    hovered = -1;

    const int availableWidth = juce::jmax (tileWidth, viewport.getMaximumVisibleWidth());
    columns = juce::jmax (1, (availableWidth - gridPadding * 2 + tileGap) / (tileWidth + tileGap));
    const int rows = (int) (visible.size() + (size_t) columns - 1) / juce::jmax (1, columns);

    canvas->setSize (availableWidth,
                     juce::jmax (tileHeight + gridPadding * 2,
                                 rows * (tileHeight + tileGap) + gridPadding * 2));
    canvas->repaint();

    statusLabel.setText (juce::String (visible.size()) + tr (" plugin(s) affiché(s)"), juce::dontSendNotification);
    updateButtons();
}

int PluginBrowser::getTileCount() const
{
    return (int) visible.size();
}

juce::Rectangle<int> PluginBrowser::getTileBounds (int index) const
{
    if (index < 0 || index >= (int) visible.size())
        return {};

    const int col = index % columns;
    const int row = index / columns;

    return { gridPadding + col * (tileWidth + tileGap),
             gridPadding + row * (tileHeight + tileGap),
             tileWidth, tileHeight };
}

void PluginBrowser::tileHovered (int index)
{
    if (index != hovered)
    {
        hovered = index;
        canvas->repaint();
    }
}

void PluginBrowser::tileClicked (int index, const juce::ModifierKeys&)
{
    selectIndex (index);
}

void PluginBrowser::tileDoubleClicked (int index)
{
    selectIndex (index);
    addSelected();
}

void PluginBrowser::selectIndex (int index)
{
    if (index >= 0 && index < (int) visible.size())
    {
        selected = visible[(size_t) index];
        const auto& info = plugins[(size_t) selected];
        selectedLabel.setText (info.name + "  ·  " + categoryFor (info)
                                   + (info.manufacturer.isNotEmpty() ? "  ·  " + info.manufacturer : ""),
                               juce::dontSendNotification);
    }
    else
    {
        selected = -1;
    }

    canvas->repaint();
    updateButtons();
}

void PluginBrowser::updateButtons()
{
    const bool hasSelection = selected >= 0 && selected < (int) plugins.size();
    const bool blocked = hasSelection && plugins[(size_t) selected].blacklisted;

    addButton.setEnabled (hasSelection && ! trackName.isEmpty() && ! blocked);
    unblockButton.setEnabled (blocked);
    closeButton.setEnabled (true);
}

void PluginBrowser::unblockSelected()
{
    if (selected < 0 || selected >= (int) plugins.size())
        return;

    const auto& info = plugins[(size_t) selected];

    if (! info.blacklisted)
        return;

    host.setPluginBlacklisted (info.identifier, false);

    // La tuile change d'aspect (ou disparaît) : ne pas laisser l'index pointer
    // une autre tuile.
    selected = -1;
    hovered = -1;
    selectedLabel.setText (tr ("Choisis une tuile, puis Ajouter."), juce::dontSendNotification);

    // refreshPluginList() redessine la grille et met déjà à jour les boutons.
    refreshPluginList();
    statusLabel.setText (tr ("Plugin réhabilité : relancez le scan."), juce::dontSendNotification);
}

void PluginBrowser::addSelected()
{
    if (selected < 0 || selected >= (int) plugins.size())
        return;

    if (plugins[(size_t) selected].blacklisted)
        return;

    if (onAdd)
        onAdd (plugins[(size_t) selected]);

    statusLabel.setText (tr ("Ajouté à la piste : ") + plugins[(size_t) selected].name,
                         juce::dontSendNotification);
}

juce::String PluginBrowser::categoryFor (const PluginInfo& info) const
{
    if (info.category.isNotEmpty())
        return info.category;

    return info.instrument ? tr ("Instrument") : tr ("Effet");
}

//==============================================================================
void PluginBrowser::scanPlugins()
{
    struct ScanTask final : juce::ThreadWithProgressWindow
    {
        ScanTask (EngineHost& h, std::function<void()> done)
            : juce::ThreadWithProgressWindow (tr ("Scan des plugins"), true, true),
              host (h), finished (std::move (done))
        {
            setStatusMessage (tr ("Recherche des plugins VST3 et LV2…"));
        }

        void run() override
        {
            int count = 0;
            host.scanForPlugins ([this, &count] (const juce::String& name)
            {
                ++count;
                setStatusMessage (tr ("Analyse : ") + name);
                setProgress (juce::jmin (0.95, 1.0 - 1.0 / (1.0 + (double) count * 0.08)));
            });
        }

        void threadComplete (bool) override
        {
            if (finished)
                juce::MessageManager::callAsync (finished);
        }

        EngineHost& host;
        std::function<void()> finished;
    };

    statusLabel.setText (tr ("Scan en cours…"), juce::dontSendNotification);
    ScanTask task (host, nullptr);

    // Persistance sur le thread UI, même si l'utilisateur annule : le scan a pu
    // blacklister des plugins avant l'annulation.
    host.savePluginBlacklist();

    if (task.runThread())
    {
        refreshPluginList();
        statusLabel.setText (tr ("Scan terminé."), juce::dontSendNotification);
    }
    else
    {
        refreshPluginList();
        statusLabel.setText (tr ("Scan annulé."), juce::dontSendNotification);
    }
}

void PluginBrowser::chooseFolder()
{
    folderChooser = std::make_unique<juce::FileChooser> (tr ("Dossier de plugins VST3 / LV2"),
                                                         juce::File::getSpecialLocation (juce::File::userHomeDirectory),
                                                         "*");

    folderChooser->launchAsync (juce::FileBrowserComponent::openMode
                                    | juce::FileBrowserComponent::canSelectDirectories,
                                [this] (const juce::FileChooser& chooser)
    {
        const auto folder = chooser.getResult();

        if (folder.isDirectory())
        {
            host.addPluginFolder (folder);
            refreshPluginList();
            statusLabel.setText (tr ("Dossier ajouté : ") + folder.getFileName(), juce::dontSendNotification);
        }
    });
}

//==============================================================================
void PluginBrowser::show (EngineHost& host, te::AudioTrack* track)
{
    const juce::String target = track != nullptr ? track->getName() : juce::String();

    auto onAdd = [&host, track] (const PluginInfo& info)
    {
        if (track == nullptr)
            return;

        if (host.addPluginToTrack (*track, info) != nullptr)
            host.saveProject();
    };

    juce::DialogWindow::LaunchOptions options;
    options.dialogTitle = tr ("Bibliothèque de plugins");
    options.dialogBackgroundColour = palette::panel;
    options.escapeKeyTriggersCloseButton = true;
    options.useNativeTitleBar = true;
    options.resizable = true;

    options.content.setOwned (new PluginBrowser (host, target, std::move (onAdd), {}));

    auto* window = options.launchAsync();
    window->setSize (900, 640);
    window->centreWithSize (900, 640);
}

//==============================================================================
void PluginBrowser::paint (juce::Graphics& g)
{
    auto r = getLocalBounds().toFloat();
    g.setGradientFill (juce::ColourGradient (palette::bgTop, 0.0f, 0.0f,
                                             palette::bgBottom, 0.0f, r.getBottom(), false));
    g.fillRect (r);

    g.setColour (juce::Colours::white.withAlpha (0.9f));
    g.drawHorizontalLine (0, 0.0f, r.getRight());
    g.setColour (palette::blueSoft);
    g.drawRect (getLocalBounds(), 1);

    // Bandeau supérieur
    g.setGradientFill (juce::ColourGradient (palette::panelHi, 0.0f, 0.0f,
                                             palette::panel, 0.0f, 66.0f, false));
    g.fillRect (0.0f, 0.0f, r.getRight(), 66.0f);
    g.setColour (palette::blue);
    g.drawHorizontalLine (66, 0.0f, r.getRight());
}

void PluginBrowser::resized()
{
    auto area = getLocalBounds().reduced (14, 10);

    auto top = area.removeFromTop (46);
    titleLabel.setBounds (top.removeFromLeft (250).withTrimmedTop (2));
    targetLabel.setBounds (top.removeFromLeft (300).withTrimmedTop (8));

    statusLabel.setBounds (top.removeFromRight (220).withTrimmedTop (8));

    auto searchRow = area.removeFromTop (34);
    search.setBounds (searchRow.removeFromLeft (330).reduced (0, 2));
    searchRow.removeFromLeft (8);
    filter.setBounds (searchRow.removeFromLeft (170).reduced (0, 2));
    searchRow.removeFromLeft (8);
    scanButton.setBounds (searchRow.removeFromLeft (120).reduced (0, 2));
    searchRow.removeFromLeft (6);
    folderButton.setBounds (searchRow.removeFromLeft (100).reduced (0, 2));

    area.removeFromTop (8);

    auto bottom = area.removeFromBottom (50);
    selectedLabel.setBounds (bottom.removeFromLeft (juce::jmax (120, bottom.getWidth() - 478)).reduced (0, 8));
    // 50 px de barre donnent des boutons de 44 px de haut, comme les autres
    // grosses commandes de l'application.
    addButton.setBounds (bottom.removeFromRight (200).reduced (0, 3));
    bottom.removeFromRight (8);
    unblockButton.setBounds (bottom.removeFromRight (130).reduced (0, 3));
    bottom.removeFromRight (8);
    closeButton.setBounds (bottom.removeFromRight (110).reduced (0, 3));

    area.removeFromBottom (8);
    viewport.setBounds (area);

    const int width = juce::jmax (tileWidth, viewport.getMaximumVisibleWidth());
    columns = juce::jmax (1, (width - gridPadding * 2 + tileGap) / (tileWidth + tileGap));
    const int rows = (int) (visible.size() + (size_t) columns - 1) / juce::jmax (1, columns);
    canvas->setSize (width, juce::jmax (tileHeight + gridPadding * 2,
                                        rows * (tileHeight + tileGap) + gridPadding * 2));
}

//==============================================================================
void PluginBrowser::paintTiles (juce::Graphics& g, juce::Rectangle<int> area)
{
    g.fillAll (palette::bgBottom);
    g.setColour (palette::line.withAlpha (0.25f));
    g.drawRect (area);

    // En mode sombre, les encres colorées sont trop peu contrastées sur les
    // badges teintés ; le texte neutre de la palette reste lisible partout.
    const bool darkTheme = palette::bgTop.getBrightness() < 0.5f;

    for (int i = 0; i < (int) visible.size(); ++i)
    {
        const auto r = getTileBounds (i).toFloat();
        const auto& info = plugins[(size_t) visible[(size_t) i]];
        const bool isSelected = (visible[(size_t) i] == selected);
        const bool isHovered = (i == hovered);

        FdLookAndFeel::drawTile (g, r, isSelected, isHovered);

        auto icon = juce::Rectangle<float> (r.getX() + 10.0f, r.getY() + 10.0f,
                                            r.getHeight() - 20.0f, r.getHeight() - 20.0f);
        const auto colour = colourForCategory (info);

        g.setGradientFill (juce::ColourGradient (colour.brighter (0.28f), icon.getX(), icon.getY(),
                                                 colour.darker (0.25f), icon.getX(), icon.getBottom(), false));
        g.fillRoundedRectangle (icon, 9.0f);
        g.setColour (juce::Colours::white.withAlpha (0.25f));
        g.drawRoundedRectangle (icon.reduced (0.5f), 9.0f, 1.0f);

        g.setColour (juce::Colours::white);
        g.setFont (juce::Font (juce::FontOptions (juce::jmax (14.0f, icon.getWidth() * 0.42f), juce::Font::bold)));
        g.drawText (initialsFor (info.name), icon, juce::Justification::centred);

        auto text = r.withTrimmedLeft (r.getHeight() - 2.0f).reduced (6.0f, 7.0f);

        g.setColour (palette::text);
        g.setFont (juce::Font (juce::FontOptions (13.0f, juce::Font::bold)));
        g.drawFittedText (info.name, text.removeFromTop (18.0f).toNearestInt(),
                          juce::Justification::topLeft, 1);

        g.setColour (palette::textDim);
        g.setFont (juce::Font (juce::FontOptions (10.0f, juce::Font::plain)));
        g.drawFittedText (info.manufacturer, text.removeFromTop (14.0f).toNearestInt(),
                          juce::Justification::topLeft, 1);

        // Catégorie sur sa propre ligne : à droite du badge il ne reste que
        // quelques pixels sur une tuile de 178 px.
        g.setFont (juce::Font (juce::FontOptions (9.5f, juce::Font::plain)));
        g.drawFittedText (categoryFor (info), text.removeFromTop (14.0f).toNearestInt(),
                          juce::Justification::topLeft, 1);

        // Même fond teinté pour INTÉGRÉ / VST3 / BLOQUÉ ; l'encre suit le thème
        // pour garder les trois libellés lisibles en clair comme en sombre.
        auto badge = juce::Rectangle<float> (text.getX(), r.getBottom() - 22.0f,
                                             info.blacklisted ? 68.0f : 62.0f, 15.0f);
        g.setColour (info.blacklisted ? palette::amber.withAlpha (0.16f)
                                      : (info.builtIn ? palette::blueFaint : palette::green.withAlpha (0.16f)));
        g.fillRoundedRectangle (badge, 4.0f);
        g.setColour (darkTheme ? palette::text
                               : (info.blacklisted ? palette::amber.darker (0.55f)
                                                   : (info.builtIn ? palette::blue : palette::green.darker (0.25f))));
        g.setFont (juce::Font (juce::FontOptions (9.0f, juce::Font::bold)));
        g.drawText (info.blacklisted ? tr ("BLOQUÉ") : (info.builtIn ? tr ("INTÉGRÉ") : tr ("VST3")),
                    badge, juce::Justification::centred);
    }
}
} // namespace fdgpe
