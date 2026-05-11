#include "UI.hpp"
#include <iostream>

int main(int argc, char* argv[]) {
    Theme theme = Theme::Default;

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--version") {
            std::cout << "termfm v" << APP_VERSION << "\n";
            return 0;
        }
        if (arg == "--theme=coffee" || arg == "--theme" && i+1 < argc && std::string(argv[++i]) == "coffee") {
            theme = Theme::Coffee;
        } else if (arg == "--theme=default") {
            theme = Theme::Default;
        } else if (arg == "--help") {
            std::cout << "Usage: termfm [--theme <default|coffee>] [--version]\n";
            return 0;
        }
    }

    UI ui(theme);
    ui.run();
}