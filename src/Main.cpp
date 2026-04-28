#include <atomic>
#include <chrono>
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
    std::atomic<bool> isConnecting{false};
    std::atomic<int>  spinnerFrame{0};
    std::atomic<int>  connectGen{0};

    std::string searchQuery;
    std::string statusMsg  = "Type a query and press Enter to search";
    std::string nowPlaying;
    int selected = 0;

    std::vector<std::string> searchModes = {"By Name", "By Country"};
    int searchMode = 0;
    MenuOption toggleOpt = MenuOption::Toggle();
    toggleOpt.entries_option.transform = [](EntryState s) {
        auto t = text("  " + s.label + "  ");
        if (s.active)  t = t | bold;
        if (s.focused) t = t | inverted;
        return t;
    };
    auto modeToggle = Menu(&searchModes, &searchMode, toggleOpt);

    auto doSearch = [&] {
        if (searchQuery.empty() || isSearching) return;
        isSearching = true;
        statusMsg = "Searching...";
        screen.PostEvent(Event::Custom);

        std::thread([&] {
            auto results = searchMode == 0
                ? browser.searchByName(searchQuery)
                : browser.searchByCountry(searchQuery);
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
        return false;
    });

    auto stationMenu = Menu(&entries, &selected);
    stationMenu |= CatchEvent([&](Event e) {
        if (e == Event::Return) {
            std::lock_guard lk(mu);
            if (selected >= 0 && selected < (int)stations.size()) {
                player.play(stations[selected].url);
                nowPlaying    = stations[selected].name;
                statusMsg     = "Connecting...";
                isConnecting  = true;
                int gen       = ++connectGen;

                // animate spinner and auto-resolve after ~3 s
                std::thread([&, gen] {
                    using namespace std::chrono_literals;
                    auto deadline = std::chrono::steady_clock::now() + 3s;
                    while (isConnecting && connectGen == gen) {
                        ++spinnerFrame;
                        screen.PostEvent(Event::Custom);
                        std::this_thread::sleep_for(80ms);
                        if (std::chrono::steady_clock::now() >= deadline) {
                            if (connectGen == gen) {
                                isConnecting = false;
                                std::lock_guard lk2(mu);
                                statusMsg = "Now playing: " + nowPlaying;
                            }
                            screen.PostEvent(Event::Custom);
                            return;
                        }
                    }
                }).detach();
            }
            return true;
        }
        if (e == Event::Character('s') || e == Event::Character('S')) {
            isConnecting = false;
            ++connectGen;
            player.stop();
            nowPlaying.clear();
            statusMsg = "Stopped.";
            return true;
        }
        return false;
    });

    auto searchRow = Container::Horizontal({modeToggle, searchInput});
    auto root = Container::Vertical({searchRow, stationMenu});
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

        Element list;
        if (isSearching) {
            list = text("  Searching...") | blink | center | flex;
        } else if (entries.empty()) {
            list = text("  No results yet") | dim | center | flex;
        } else {
            list = stationMenu->Render() | vscroll_indicator | frame | flex;
        }

        Element statusBar;
        if (isConnecting) {
            statusBar = hbox({
                text(" "),
                spinner(6, spinnerFrame) | color(Color::Yellow),
                text("  " + statusMsg) | color(Color::Yellow),
            });
        } else {
            Color c = nowPlaying.empty() ? Color::GrayDark : Color::Green;
            statusBar = hbox({text(" "), text(statusMsg) | color(c)});
        }

        return vbox({
            text(" TermFM  -  Terminal Radio Player ")
                | bold | color(Color::Cyan) | center,
            separator(),
            hbox({
                text(" "), modeToggle->Render(),
                text("  |  Search: "), searchInput->Render(),
            }) | border,
            list | border,
            statusBar | border,
            text("  [Enter] search / play   [S] stop   [Q] quit  ")
                | dim | center,
        });
    });

    screen.Loop(ui);
    return 0;
}
