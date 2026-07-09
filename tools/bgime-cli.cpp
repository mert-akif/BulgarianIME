#include "bgime/Transliterator.hpp"
#include "bgime/Mapping.hpp"

#include <exception>
#include <iostream>
#include <string>

#ifdef _WIN32
    #ifndef NOMINMAX
        #define NOMINMAX
    #endif
    #include <windows.h>
#endif

namespace {

void setupConsoleUtf8() {
#ifdef _WIN32
    // Makes Windows Terminal / modern cmd.exe display UTF-8 Cyrillic output correctly.
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
#endif
}

void printHelp() {
    std::cout
        << "BulgarianIME CLI\n"
        << "\n"
        << "Usage:\n"
        << "  bgime-cli [--standard] <text>\n"
        << "  bgime-cli --interactive\n"
        << "\n"
        << "Examples:\n"
        << "  bgime-cli zdravey az sım\n"
        << "  bgime-cli --standard zdravey az sym\n"
        << "\n"
        << "Interactive commands:\n"
        << "  :help      show this help\n"
        << "  :turkish   use Turkish-Q friendly mapping\n"
        << "  :standard  use standard Latin mapping\n"
        << "  :quit      exit\n"
        << "\n"
        << "Turkish-Q extras:\n"
        << "  ş -> ш, ç -> ч, j -> ж, ı -> ъ, ü -> ю\n";
}

bgime::Mapping makeMapping(bool standard) {
    return standard ? bgime::Mapping::standardLatin() : bgime::Mapping::turkishQ();
}

int runOnce(bool standard, const std::string& text) {
    bgime::Transliterator transliterator(makeMapping(standard));
    std::cout << transliterator.convertUtf8(text) << '\n';
    return 0;
}

int runInteractive(bool standard) {
    std::cout << "BulgarianIME interactive mode\n";
    std::cout << "Type Latin/Turkish-Q text and press Enter. Type :quit to exit.\n";
    std::cout << "Current mode: " << (standard ? "standard Latin" : "Turkish-Q friendly") << "\n\n";

    bgime::Transliterator transliterator(makeMapping(standard));
    std::string line;

    while (true) {
        std::cout << "> ";
        if (!std::getline(std::cin, line)) {
            break;
        }

        if (line == ":quit" || line == ":q" || line == "exit") {
            break;
        }
        if (line == ":help") {
            printHelp();
            continue;
        }
        if (line == ":standard") {
            standard = true;
            transliterator = bgime::Transliterator(makeMapping(standard));
            std::cout << "Mode changed: standard Latin\n";
            continue;
        }
        if (line == ":turkish") {
            standard = false;
            transliterator = bgime::Transliterator(makeMapping(standard));
            std::cout << "Mode changed: Turkish-Q friendly\n";
            continue;
        }
        if (line.empty()) {
            continue;
        }

        std::cout << transliterator.convertUtf8(line) << "\n\n";
    }

    return 0;
}

} // namespace

int main(int argc, char** argv) {
    setupConsoleUtf8();

    bool standard = false;
    bool interactive = false;
    bool showHelp = false;
    std::string text;

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];

        if (arg == "--standard") {
            standard = true;
            continue;
        }
        if (arg == "--turkish") {
            standard = false;
            continue;
        }
        if (arg == "--interactive" || arg == "-i") {
            interactive = true;
            continue;
        }
        if (arg == "--help" || arg == "-h") {
            showHelp = true;
            continue;
        }

        if (!text.empty()) {
            text += " ";
        }
        text += arg;
    }

    if (showHelp) {
        printHelp();
        return 0;
    }

    try {
        // If the user double-clicks the exe or runs it without arguments, keep it open.
        if (interactive || text.empty()) {
            return runInteractive(standard);
        }

        return runOnce(standard, text);
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << '\n';
        return 2;
    }
}
