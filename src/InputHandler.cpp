#include "snake/InputHandler.hpp"

#include <cctype>
#include <iostream>

#if defined(_WIN32)
#  include <conio.h>
#else
#  include <unistd.h>
#endif

namespace snake {

Command InputHandler::fromChar(char c) {
    switch (std::tolower(static_cast<unsigned char>(c))) {
        case 'w': return Command::Up;
        case 's': return Command::Down;
        case 'a': return Command::Left;
        case 'd': return Command::Right;
        case 'k': return Command::Up;      // vi keys, because muscle memory
        case 'j': return Command::Down;
        case 'h': return Command::Left;
        case 'l': return Command::Right;
        case 'p': return Command::Pause;
        case ' ': return Command::Pause;
        case 'r': return Command::Restart;
        case 'q': return Command::Quit;
        default:  return Command::None;
    }
}

std::vector<Command> InputHandler::poll() {
    std::vector<Command> commands;

#if defined(_WIN32)
    while (_kbhit()) {
        const int ch = _getch();
        if (ch == 0 || ch == 224) {           // extended key prefix
            switch (_getch()) {
                case 72: commands.push_back(Command::Up);    break;
                case 80: commands.push_back(Command::Down);  break;
                case 75: commands.push_back(Command::Left);  break;
                case 77: commands.push_back(Command::Right); break;
                default: break;
            }
            continue;
        }
        const Command c = fromChar(static_cast<char>(ch));
        if (c != Command::None) commands.push_back(c);
    }
#else
    char buffer[64];
    ssize_t count = 0;

    // VMIN=0/VTIME=0 raw mode makes this return 0 straight away when there
    // is nothing to read, so the game loop never stalls on input.
    while ((count = ::read(STDIN_FILENO, buffer, sizeof(buffer))) > 0) {
        for (ssize_t i = 0; i < count; ++i) {
            const char c = buffer[i];

            switch (escape_) {
                case EscapeState::None:
                    if (c == '\x1b') {
                        escape_ = EscapeState::Escape;
                    } else {
                        const Command cmd = fromChar(c);
                        if (cmd != Command::None) commands.push_back(cmd);
                    }
                    break;

                case EscapeState::Escape:
                    // An arrow key is ESC [ A..D. The sequence can straddle
                    // two read() calls, hence the persistent state.
                    escape_ = (c == '[' || c == 'O') ? EscapeState::Bracket : EscapeState::None;
                    break;

                case EscapeState::Bracket:
                    switch (c) {
                        case 'A': commands.push_back(Command::Up);    break;
                        case 'B': commands.push_back(Command::Down);  break;
                        case 'C': commands.push_back(Command::Right); break;
                        case 'D': commands.push_back(Command::Left);  break;
                        default: break;
                    }
                    escape_ = EscapeState::None;
                    break;
            }
        }
        if (count < static_cast<ssize_t>(sizeof(buffer))) break;
    }
#endif

    return commands;
}

std::string InputHandler::readLine(const std::string& prompt, std::size_t maxLength) {
    std::cout << prompt << std::flush;

    std::string line;
    if (!std::getline(std::cin, line)) {
        std::cin.clear();
        return {};
    }
    if (line.size() > maxLength) line.resize(maxLength);
    return line;
}

}  // namespace snake
