// Terminal Snake -- entry point and game loop.
//
// The loop runs much faster than the game ticks. Input is drained every
// iteration so steering feels instant, while the simulation only advances
// once per tickInterval(). That decoupling is what keeps a slow snake
// responsive.

#include <chrono>
#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <thread>

#include "snake/Config.hpp"
#include "snake/Game.hpp"
#include "snake/HighScore.hpp"
#include "snake/InputHandler.hpp"
#include "snake/Renderer.hpp"
#include "snake/Terminal.hpp"

#if defined(_WIN32)
#  include <io.h>
#  define SNAKE_ISATTY_STDIN() (_isatty(_fileno(stdin)) != 0)
#else
#  include <unistd.h>
#  define SNAKE_ISATTY_STDIN() (isatty(STDIN_FILENO) != 0)
#endif

using namespace snake;
using Clock = std::chrono::steady_clock;

namespace {

constexpr auto kFrameSleep = std::chrono::milliseconds(4);

std::string defaultPlayerName() {
    if (const char* user = std::getenv("USER"); user && *user) return user;
    if (const char* user = std::getenv("USERNAME"); user && *user) return user;
    return "player";
}

/// Asks for a name (cooked mode) and writes the entry to disk.
/// Returns the 1-based rank, or 0 when the score did not place.
std::size_t recordScore(const Game& game, HighScoreTable& scores, Terminal& term,
                        const Config& cfg) {
    if (!scores.qualifies(game.score())) return 0;

    std::string name = cfg.playerName;
    if (name.empty()) {
        term.setRaw(false);
        term.showCursor(true);
        name = InputHandler::readLine("\n  New high score! Name: ");
        term.showCursor(false);
        term.setRaw(true);
    }
    if (name.empty()) name = defaultPlayerName();

    ScoreEntry entry;
    entry.name  = name;
    entry.score = game.score();
    entry.level = game.level();
    entry.date  = ScoreEntry::today();

    const std::size_t rank = scores.add(entry);
    if (!scores.save()) {
        std::cerr << "\n  warning: could not save scores (" << scores.lastError() << ")\n";
    }
    return rank;
}

void printScoreboard(const HighScoreTable& scores) {
    if (scores.entries().empty()) return;

    std::cout << "\nHigh scores (" << scores.path() << ")\n";
    int rank = 1;
    for (const ScoreEntry& e : scores.entries()) {
        std::cout << "  " << std::setw(2) << rank++ << ". "
                  << std::left << std::setw(18) << e.name << std::right
                  << std::setw(7) << e.score
                  << "   lvl " << std::setw(2) << e.level
                  << "   " << e.date << "\n";
    }
}

int runGame(const Config& cfg, HighScoreTable& scores, int& finalScore) {
    Terminal term;
    InputHandler input;
    Renderer renderer(std::cout);
    Game game(cfg);

    // A board that does not fit gets clipped into an unreadable mess, so
    // bail out early with something actionable instead.
    const TerminalSize size = term.size();
    const int neededCols = cfg.width * 2 + 6;
    const int neededRows = cfg.height + 7;
    if (size.cols < neededCols || size.rows < neededRows) {
        std::cerr << "snake: terminal is " << size.cols << "x" << size.rows
                  << ", needs at least " << neededCols << "x" << neededRows
                  << " (resize, or use --width/--height)\n";
        return 2;
    }

    auto lastTick = Clock::now();
    bool overHandled = false;
    renderer.draw(game, scores);

    while (game.state() != GameState::Quit) {
        if (Terminal::interrupted()) {
            game.quit();
            break;
        }

        bool dirty = false;

        for (const Command cmd : input.poll()) {
            switch (cmd) {
                case Command::Quit:
                    game.quit();
                    break;
                case Command::Restart:
                    game.reset();
                    overHandled = false;
                    lastTick = Clock::now();
                    dirty = true;
                    break;
                case Command::Pause:
                    if (!game.isOver()) { game.togglePause(); dirty = true; }
                    break;
                case Command::Up:
                    if (!game.isOver()) game.requestDirection(Direction::Up);
                    break;
                case Command::Down:
                    if (!game.isOver()) game.requestDirection(Direction::Down);
                    break;
                case Command::Left:
                    if (!game.isOver()) game.requestDirection(Direction::Left);
                    break;
                case Command::Right:
                    if (!game.isOver()) game.requestDirection(Direction::Right);
                    break;
                case Command::None:
                    break;
            }
        }

        if (game.state() == GameState::Quit) break;

        if (game.state() == GameState::Running) {
            const auto now = Clock::now();
            if (now - lastTick >= game.tickInterval()) {
                game.tick();
                lastTick = now;
                dirty = true;
            }
        }

        if (game.isOver() && !overHandled) {
            renderer.draw(game, scores);
            const std::size_t rank = recordScore(game, scores, term, cfg);
            renderer.draw(game, scores);
            renderer.drawGameOver(game, rank);
            overHandled = true;
        } else if (dirty) {
            renderer.draw(game, scores);
        }

        std::this_thread::sleep_for(kFrameSleep);
    }

    finalScore = game.score();
    return 0;  // Terminal's destructor restores the screen here
}

}  // namespace

int main(int argc, char** argv) {
    Config cfg;
    bool helpRequested = false;

    if (!Config::fromArgs(argc, argv, cfg, helpRequested)) return 1;
    if (helpRequested) {
        std::cout << Config::usage(argc > 0 ? argv[0] : "snake");
        return 0;
    }
    cfg.sanitize();

    if (!SNAKE_ISATTY_STDIN()) {
        std::cerr << "snake: stdin is not a terminal; run this directly in a shell.\n";
        return 1;
    }

    HighScoreTable scores(cfg.highScoreFile);
    scores.load();

    int finalScore = 0;
    const int status = runGame(cfg, scores, finalScore);
    if (status != 0) return status;

    std::cout << "Thanks for playing. Final score: " << finalScore << "\n";
    printScoreboard(scores);
    return 0;
}
