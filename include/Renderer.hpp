#pragma once

#include <iosfwd>
#include <string>

#include "snake/Game.hpp"
#include "snake/HighScore.hpp"

namespace snake {

/// Builds a whole frame into one string and writes it in a single call.
/// Drawing cell-by-cell straight to stdout is what makes terminal games
/// flicker; one buffered write plus a cursor-home escape does not.
class Renderer {
public:
    explicit Renderer(std::ostream& out);

    void draw(const Game& game, const HighScoreTable& scores);

    /// Panel shown under the board once the run has ended.
    void drawGameOver(const Game& game, std::size_t rank);

    /// Renders to a string instead of the stream -- handy for tests.
    std::string frame(const Game& game, const HighScoreTable& scores) const;

private:
    std::ostream& out_;
    std::string   buffer_;
    bool          firstFrame_ = true;

    void appendHeader(std::string& s, const Game& g, const HighScoreTable& hs) const;
    void appendBoard(std::string& s, const Game& g) const;
    void appendFooter(std::string& s, const Game& g) const;

    const char* colour(const char* code, const Config& cfg) const;
};

}  // namespace snake
