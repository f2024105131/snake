#pragma once

#include <string>
#include <vector>

#include "snake/Types.hpp"

namespace snake {

/// Drains whatever the user has typed without ever blocking the game loop.
/// Arrow keys arrive as multi-byte escape sequences that can be split across
/// reads, so the parser keeps a tiny state machine between calls.
class InputHandler {
public:
    /// Returns every command typed since the previous call, in order.
    std::vector<Command> poll();

    /// Blocking, line-oriented read used only on the game-over screen.
    /// The caller must leave raw mode first (see Terminal::setRaw).
    static std::string readLine(const std::string& prompt, std::size_t maxLength = 16);

    /// Maps a plain character to a command. Exposed for unit testing.
    static Command fromChar(char c);

private:
    enum class EscapeState { None, Escape, Bracket };
    EscapeState escape_ = EscapeState::None;
};

}  // namespace snake
