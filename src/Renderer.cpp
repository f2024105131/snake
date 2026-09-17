#include "snake/Renderer.hpp"

#include <ostream>
#include <string>
#include <vector>

namespace snake {
namespace {

// ANSI colour codes, all funnelled through Renderer::colour() so a single
// --no-color check disables them everywhere.
constexpr const char* kReset   = "\x1b[0m";
constexpr const char* kDim     = "\x1b[2m";
constexpr const char* kBold    = "\x1b[1m";
constexpr const char* kGreen   = "\x1b[32m";
constexpr const char* kBGreen  = "\x1b[92m";
constexpr const char* kRed     = "\x1b[91m";
constexpr const char* kCyan    = "\x1b[96m";
constexpr const char* kYellow  = "\x1b[93m";

constexpr const char* kHome      = "\x1b[H";   // cursor to 1,1
constexpr const char* kClearLine = "\x1b[K";   // erase to end of line
constexpr const char* kClearDown = "\x1b[J";   // erase to end of screen

struct Glyphs {
    const char* empty;
    const char* body;
    const char* head;
    const char* food;
    const char* hBar;
    const char* vBar;
    const char* tl;
    const char* tr;
    const char* bl;
    const char* br;
};

Glyphs glyphsFor(bool ascii) {
    if (ascii) return Glyphs{"  ", "oo", "@@", "**", "-", "|", "+", "+", "+", "+"};
    return Glyphs{"  ", "\u2592\u2592", "\u2588\u2588", "\u25c6\u25c6",
                  "\u2500", "\u2502", "\u250c", "\u2510", "\u2514", "\u2518"};
}

std::string padded(const std::string& s, std::size_t width) {
    return s.size() >= width ? s : s + std::string(width - s.size(), ' ');
}

}  // namespace

Renderer::Renderer(std::ostream& out) : out_(out) {}

const char* Renderer::colour(const char* code, const Config& cfg) const {
    return cfg.color ? code : "";
}

void Renderer::appendHeader(std::string& s, const Game& g, const HighScoreTable& hs) const {
    const Config& cfg = g.config();
    const int best = hs.best() > g.score() ? hs.best() : g.score();

    s += colour(kBold, cfg);
    s += colour(kCyan, cfg);
    s += "  S N A K E";
    s += colour(kReset, cfg);
    s += colour(kDim, cfg);
    s += cfg.wrap ? "   [wrap]" : "";
    s += colour(kReset, cfg);
    s += kClearLine;
    s += "\n";

    s += "  Score ";
    s += colour(kYellow, cfg);
    s += padded(std::to_string(g.score()), 6);
    s += colour(kReset, cfg);
    s += " Best ";
    s += padded(std::to_string(best), 6);
    s += " Level ";
    s += padded(std::to_string(g.level()), 4);
    s += " Len ";
    s += padded(std::to_string(g.snake().size()), 4);
    s += kClearLine;
    s += "\n";
}

void Renderer::appendBoard(std::string& s, const Game& g) const {
    const Config& cfg = g.config();
    const Board& board = g.board();
    const Glyphs gl = glyphsFor(cfg.ascii);

    // Rasterise the snake into a lookup grid first: scanning the whole body
    // for every cell would be O(cells * length) per frame.
    const int w = board.width();
    const int h = board.height();
    std::vector<char> grid(static_cast<std::size_t>(w) * h, 0);  // 0 empty, 1 body, 2 head

    for (const Point& p : g.snake().cells()) {
        if (!board.outOfBounds(p)) grid[static_cast<std::size_t>(p.y) * w + p.x] = 1;
    }
    const Point head = g.snake().head();
    if (!board.outOfBounds(head)) grid[static_cast<std::size_t>(head.y) * w + head.x] = 2;

    std::string horizontal;
    for (int i = 0; i < w * 2; ++i) horizontal += gl.hBar;

    s += "  ";
    s += colour(kDim, cfg);
    s += gl.tl + horizontal + gl.tr;
    s += colour(kReset, cfg);
    s += kClearLine;
    s += "\n";

    for (int y = 0; y < h; ++y) {
        s += "  ";
        s += colour(kDim, cfg);
        s += gl.vBar;
        s += colour(kReset, cfg);

        for (int x = 0; x < w; ++x) {
            const Point here{x, y};
            if (board.hasFood() && board.food() == here) {
                s += colour(kRed, cfg);
                s += gl.food;
                s += colour(kReset, cfg);
                continue;
            }
            switch (grid[static_cast<std::size_t>(y) * w + x]) {
                case 2:
                    s += colour(kBGreen, cfg);
                    s += gl.head;
                    s += colour(kReset, cfg);
                    break;
                case 1:
                    s += colour(kGreen, cfg);
                    s += gl.body;
                    s += colour(kReset, cfg);
                    break;
                default:
                    s += gl.empty;
                    break;
            }
        }

        s += colour(kDim, cfg);
        s += gl.vBar;
        s += colour(kReset, cfg);
        s += kClearLine;
        s += "\n";
    }

    s += "  ";
    s += colour(kDim, cfg);
    s += gl.bl + horizontal + gl.br;
    s += colour(kReset, cfg);
    s += kClearLine;
    s += "\n";
}

void Renderer::appendFooter(std::string& s, const Game& g) const {
    const Config& cfg = g.config();

    s += "  ";
    switch (g.state()) {
        case GameState::Paused:
            s += colour(kYellow, cfg);
            s += "PAUSED - press p to resume";
            s += colour(kReset, cfg);
            break;
        case GameState::Lost:
            s += colour(kRed, cfg);
            s += "GAME OVER";
            s += colour(kReset, cfg);
            break;
        case GameState::Won:
            s += colour(kBGreen, cfg);
            s += "YOU WIN - the board is full!";
            s += colour(kReset, cfg);
            break;
        default:
            s += colour(kDim, cfg);
            s += "arrows/WASD move  -  p pause  -  r restart  -  q quit";
            s += colour(kReset, cfg);
            break;
    }
    s += kClearLine;
    s += "\n";
}

std::string Renderer::frame(const Game& game, const HighScoreTable& scores) const {
    std::string s;
    s.reserve(static_cast<std::size_t>(game.board().cellCount()) * 12 + 256);
    appendHeader(s, game, scores);
    appendBoard(s, game);
    appendFooter(s, game);
    return s;
}

void Renderer::draw(const Game& game, const HighScoreTable& scores) {
    buffer_.clear();
    buffer_ += kHome;  // redraw over the previous frame instead of clearing
    buffer_ += frame(game, scores);
    buffer_ += kClearDown;

    out_ << buffer_ << std::flush;
    firstFrame_ = false;
}

void Renderer::drawGameOver(const Game& game, std::size_t rank) {
    const Config& cfg = game.config();
    std::string s;

    s += "\n  ";
    s += colour(kBold, cfg);
    s += (game.state() == GameState::Won ? "You filled the board!" : "You crashed.");
    s += colour(kReset, cfg);
    s += "  Final score ";
    s += colour(kYellow, cfg);
    s += std::to_string(game.score());
    s += colour(kReset, cfg);
    s += "  (level ";
    s += std::to_string(game.level());
    s += ", length ";
    s += std::to_string(game.snake().size());
    s += ")\n";

    if (rank > 0) {
        s += "  ";
        s += colour(kBGreen, cfg);
        s += "New high score - rank #" + std::to_string(rank) + "!";
        s += colour(kReset, cfg);
        s += "\n";
    }

    s += "  ";
    s += colour(kDim, cfg);
    s += "press r to play again, q to quit";
    s += colour(kReset, cfg);
    s += "\n";

    out_ << s << std::flush;
}

}  // namespace snake
