#pragma once

#include <atomic>
#include <filesystem>
#include <fstream>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

#include "AudioPlayer.hpp"
#include "RadioBrowser.hpp"
#include "StaticStack.hpp"
#include "json.hpp"

#include "ftxui/component/component.hpp"
#include "ftxui/component/screen_interactive.hpp"
#include "ftxui/dom/elements.hpp"

using json = nlohmann::json;
using namespace ftxui;

enum class Theme { Default, Coffee };
enum class ViewMode { Search, Favorites };

struct ThemeColors {
    Color primary;
    Color secondary;
    Color inactive;
    Color status;
    Color searching;
};

static ThemeColors resolveTheme(Theme t) {
    switch (t) {
        case Theme::Coffee:
            return {
                Color::RGB(210, 140, 80),
                Color::RGB(180, 110, 60),
                Color::RGB(100, 70, 45),
                Color::RGB(120, 200, 100),
                Color::RGB(210, 140, 80),
            };
        case Theme::Default:
        default:
            return {
                Color::Cyan,
                Color::Cyan,
                Color::GrayDark,
                Color::Green,
                Color::Cyan,
            };
    }
}

class UI {
public:
    UI(Theme theme = Theme::Default)
        : screen(ScreenInteractive::Fullscreen())
        , colors(resolveTheme(theme))
    {
        loadHistory();
        loadFavorites();
        buildComponents();
    }

    void run() {
        screen.Loop(ui);
    }

private:
    ftxui::ScreenInteractive screen;
    AudioPlayer              player;
    RadioBrowser             browser;
    ThemeColors              colors;

    std::mutex               mu;
    std::vector<Station>     stations;
    std::vector<Station>     stationsCache;
    std::vector<std::string> entries;
    std::vector<std::string> entriesCache;
    std::atomic<bool>        isSearching{false};

    std::string              searchQuery;
    std::string              statusMsg   = "Type a query and press Enter to search";
    std::string              statusMsgCache;
    std::string              nowPlaying;
    int                      selected    = 0;
    int                      searchMode  = 0;
    ViewMode                 viewMode    = ViewMode::Search;
    StaticStack<Station, 20> recentStations;
    StaticStack<Station, 20> favoriteStations;

    const std::vector<std::string> searchModes = {"By Name", "By Country", "By Language"};
    const std::string historyPath   = std::string(getenv("HOME")) + "/.config/termfm/history.json";
    const std::string favoritesPath = std::string(getenv("HOME")) + "/.config/termfm/favorites.json";

    ftxui::Component searchInput;
    ftxui::Component stationMenu;
    ftxui::Component root;
    ftxui::Component ui;

    void loadHistory() {
        std::ifstream f(historyPath);
        if (!f.is_open()) return;
        try {
            json arr = json::parse(f);
            for (auto& s : arr) {
                stations.push_back({s["name"], s["url"]});
                entries.push_back(s["name"]);
                recentStations.push({s["name"], s["url"]});
            }
            statusMsg = "Recent stations loaded — press Enter to play";
        } catch (...) {}
    }

    void saveHistory() {
        std::filesystem::create_directories(
            std::string(getenv("HOME")) + "/.config/termfm");
        json arr = json::array();
        recentStations.forEach([&](const Station& s) {
            arr.push_back({{"name", s.name}, {"url", s.url}});
        });
        std::ofstream(historyPath) << arr.dump(2);
    }

    void loadFavorites() {
        std::ifstream f(favoritesPath);
        if (!f.is_open()) return;
        try {
            json arr = json::parse(f);
            for (auto& s : arr)
                favoriteStations.push({s["name"], s["url"]});
        } catch (...) {}
    }

    void saveFavorites() {
        std::filesystem::create_directories(
            std::string(getenv("HOME")) + "/.config/termfm");
        json arr = json::array();
        favoriteStations.forEach([&](const Station& s) {
            arr.push_back({{"name", s.name}, {"url", s.url}});
        });
        std::ofstream(favoritesPath) << arr.dump(2);
    }

    void showFavorites() {
        viewMode = ViewMode::Favorites;
        stationsCache = stations;
        entriesCache  = entries;
        stations.clear();
        entries.clear();
        favoriteStations.forEach([&](const Station& s) {
            stations.push_back(s);
            entries.push_back(s.name);
        });
        selected  = 0;
        statusMsgCache = statusMsg;
        statusMsg = entries.empty()
            ? "No favorites yet — press Ctrl+G on a station to add one"
            : std::to_string(entries.size()) + " favorites  |  Enter to play, Ctrl+F again to go back";
    }

    void showSearch() {
        viewMode = ViewMode::Search;
        // stations.clear();
        // entries.clear();
        selected  = 0;
        statusMsg = statusMsgCache;
    }

    void doSearch() {
        if (searchQuery.empty() || isSearching) return;
        isSearching = true;
        statusMsg   = "Searching...";
        screen.PostEvent(Event::Custom);

        std::thread([this] {
            std::string query = searchQuery;
            if (searchMode == 2)
                std::transform(query.begin(), query.end(), query.begin(), ::tolower);

            auto results = searchMode == 0 ? browser.searchByName(query)
                         : searchMode == 1 ? browser.searchByCountry(query)
                                           : browser.searchByLanguage(query);
            {
                std::lock_guard lk(mu);
                stations = std::move(results);
                entries.clear();
		entries.reserve(stations.size());
                for (auto& s : stations) entries.push_back(s.name);
                selected  = 0;
                statusMsg = entries.empty()
                    ? "No stations found."
                    : std::to_string(entries.size()) +
                      " stations found  |  arrow keys to select, Enter to play";
            }
            isSearching = false;
            screen.PostEvent(Event::Custom);
        }).detach();
    }

    void buildComponents() {
        searchInput = Input(&searchQuery, "station / artist name...");
        searchInput |= CatchEvent([this](Event e) {
            if (e == Event::Return) { doSearch(); return true; }
            if (e == Event::Tab) {
                searchMode = (searchMode + 1) % 3;
                return true;
            }
            return false;
        });

        MenuOption menuOpt;
        menuOpt.entries_option.transform = [this](EntryState s) {
            auto t = text(" " + s.label + " ");
            if (s.focused) t = t | inverted;
            if (s.active)  t = t | bold | color(colors.secondary);
            return t;
        };
        stationMenu = Menu(&entries, &selected, menuOpt);
        stationMenu |= CatchEvent([this](Event e) {
            if (e == Event::Return) {
                std::lock_guard lk(mu);
                if (selected >= 0 && selected < (int)stations.size()) {
                    player.play(stations[selected].url);
                    nowPlaying = stations[selected].name;
                    recentStations.push(stations[selected]);
                    statusMsg  = "Now playing: " + nowPlaying;
                }
                return true;
            }
            if (e == Event::Character('s') || e == Event::Character('S')) {
                player.stop();
                nowPlaying.clear();
                statusMsg = "Stopped.";
                return true;
            }
            if (e == Event::Special("\x06")) {
                std::lock_guard lk(mu);
                if (viewMode == ViewMode::Favorites) {
                    entries = std::move(entriesCache);
                    stations = std::move(stationsCache);
                    showSearch();
                } else {
                    // save search
                    showFavorites();
                }
                return true;
            }
            if (e == Event::Special("\x07")) {
                std::lock_guard lk(mu);
                if (selected >= 0 && selected < (int)stations.size()) {
                    favoriteStations.push(stations[selected]);
                    saveFavorites();
                    statusMsg = "★ Added to favorites: " + stations[selected].name;
                }
                return true;
            }
            return false;
        });

        root = Container::Vertical({searchInput, stationMenu});
        root |= CatchEvent([this](Event e) {
            if (e == Event::Character('q') || e == Event::Character('Q')) {
                saveHistory();
                saveFavorites();
                player.stop();
                screen.ExitLoopClosure()();
                return true;
            }
            return false;
        });

        ui = Renderer(root, [this] {
            std::lock_guard lk(mu);

            auto modeLabel = [this](int idx) {
                bool active = (searchMode == idx);
                return text(searchModes[idx])
                    | (active ? bold : dim)
                    | color(active ? colors.primary : colors.inactive);
            };

            auto modeBar = hbox({
                text(" Mode: "),
                modeLabel(0),
                text("  /  "),
                modeLabel(1),
                text("  /  "),
                modeLabel(2),
                text("  [Tab to switch]") | dim,
            });

            Element list;
            if (isSearching) {
                list = text("  Searching...") | blink | center | flex
                                              | color(colors.searching);
            } else if (entries.empty()) {
                list = text("  No results yet") | dim | center | flex;
            } else {
                list = stationMenu->Render() | vscroll_indicator | frame | flex;
            }

            Color statusColor = nowPlaying.empty() ? colors.inactive : colors.status;
            Element statusBar = hbox({text(" "), text(statusMsg) | color(statusColor)});

            auto titleText = hbox({
                text(" TermFM") | bold | color(colors.primary),
                text(viewMode == ViewMode::Favorites ? "  [★ Favorites]" : "")
                    | bold | color(colors.secondary),
            });

            return vbox({
                titleText | center,
                separator(),
                hbox({modeBar, text("  |  Search: "), searchInput->Render()}) | border,
                list | border,
                statusBar | border,
                text("  [Enter] play   [S] stop   [Tab] mode   [Ctrl+F] favorites   [Ctrl+G] add favorite   [Q] quit  ")
                    | dim | center,
            });
        });
    }
};
