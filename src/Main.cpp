#include <atomic>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

#include "AudioPlayer.hpp"
#include "RadioBrowser.hpp"
#include "ftxui/component/component.hpp"
#include "ftxui/component/screen_interactive.hpp"
#include "ftxui/dom/elements.hpp"

using namespace ftxui;

int main() {
    auto screen = ScreenInteractive::Fullscreen();

    RadioBrowser browser;
    AudioPlayer player;

    std::mutex mu;
    std::vector<Station> stations;
    std::vector<std::string> entries;
    std::atomic<bool> isSearching{false};

    std::string searchQuery;
    std::string statusMsg  = "Type a query and press Enter to search";
    std::string nowPlaying;
    int selected = 0;

    std::vector<std::string> searchModes = {"By Name", "By Country", "By Language"};
    int searchMode = 0;

    auto doSearch = [&] {
        if (searchQuery.empty() || isSearching) return;
        isSearching = true;
        statusMsg = "Searching...";
        screen.PostEvent(Event::Custom);

        std::thread([&] {
            auto results = searchMode == 0 ? browser.searchByName(searchQuery) : (searchMode == 1 ? browser.searchByCountry(searchQuery) : browser.searchByLanguage(searchQuery));
            {
                std::lock_guard lk(mu);
                stations = std::move(results);
                entries.clear();
                for (auto& s : stations) entries.push_back(s.name);
                selected = 0;
                statusMsg = entries.empty()
                    ? "No stations found."
                    : std::to_string(entries.size()) +
                      " stations found  |  arrow keys to select, Enter to play";
            }
            isSearching = false;
            screen.PostEvent(Event::Custom);
        }).detach();
    };

    auto searchInput = Input(&searchQuery, "station / artist name...");
    searchInput |= CatchEvent([&](Event e) {
        if (e == Event::Return) { doSearch(); return true; }
        if (e == Event::Tab) {
            searchMode = (searchMode + 1) % 3;
            return true;
        }
        return false;
    });

    MenuOption menuOpt;
    menuOpt.entries_option.transform = [](EntryState s) {
        auto t = text(" " + s.label + " ");
        if (s.focused) t = t | inverted;
        if (s.active)  t = t | bold;
        return t;
    };
    auto stationMenu = Menu(&entries, &selected, menuOpt);
    stationMenu |= CatchEvent([&](Event e) {
        if (e == Event::Return) {
            std::lock_guard lk(mu);
            if (selected >= 0 && selected < (int)stations.size()) {
                player.play(stations[selected].url);
                nowPlaying = stations[selected].name;
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
        return false;
    });

    auto root = Container::Vertical({searchInput, stationMenu});
    root |= CatchEvent([&](Event e) {
        if (e == Event::Character('q') || e == Event::Character('Q')) {
            player.stop();
            screen.ExitLoopClosure()();
            return true;
        }
        return false;
    });

    auto ui = Renderer(root, [&] {
        std::lock_guard lk(mu);

        auto modeBar = hbox({
            text(" Mode: "),
            text(searchModes[0]) | (searchMode == 0 ? bold : dim)
                                 | (searchMode == 0 ? color(Color::Cyan) : color(Color::GrayDark)),
            text("  /  "),
            text(searchModes[1]) | (searchMode == 1 ? bold : dim)
                                 | (searchMode == 1 ? color(Color::Cyan) : color(Color::GrayDark)),
            text("  /  "),
            text(searchModes[2]) | (searchMode == 2 ? bold : dim)
                                 | (searchMode == 2 ? color(Color::Cyan) : color(Color::GrayDark)),
            text("  [Tab to switch]") | dim,
        });

        Element list;
        if (isSearching) {
            list = text("  Searching...") | blink | center | flex;
        } else if (entries.empty()) {
            list = text("  No results yet") | dim | center | flex;
        } else {
            list = stationMenu->Render() | vscroll_indicator | frame | flex;
        }

        Color c = nowPlaying.empty() ? Color::GrayDark : Color::Green;
        Element statusBar = hbox({text(" "), text(statusMsg) | color(c)});

        return vbox({
            text(" TermFM")
                | bold | color(Color::Cyan) | center,
            separator(),
            hbox({
                modeBar,
                text("  |  Search: "),
                searchInput->Render(),
            }) | border,
            list | border,
            statusBar | border,
            text("  [Enter] search / play   [S] stop   [Tab] mode   [Q] quit  ")
                | dim | center,
        });
    });

    screen.Loop(ui);
    return 0;
}