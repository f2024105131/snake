// Dependency-free test runner. The logic layer has no terminal calls, so
// every rule below can be checked headlessly in CI.

#include <cstdio>
#include <cstdlib>
#include <cmath>
#include <filesystem>
#include <iostream>
#include <string>

#include "snake/Board.hpp"
#include "snake/Config.hpp"
#include "snake/Game.hpp"
#include "snake/HighScore.hpp"
#include "snake/InputHandler.hpp"
#include "snake/Renderer.hpp"
#include "snake/Snake.hpp"

using namespace snake;

namespace {

int g_failures = 0;
int g_checks   = 0;

void check(bool condition, const std::string& label) {
    ++g_checks;
    if (!condition) {
        ++g_failures;
        std::cout << "  FAIL  " << label << "\n";
    }
}

Config testConfig() {
    Config cfg;
    cfg.width         = 12;
    cfg.height        = 8;
    cfg.initialLength = 3;
    cfg.seed          = 1234;  // deterministic food placement
    cfg.foodPerLevel  = 2;
    return cfg;
}

// --- snake body -----------------------------------------------------------

void testSnakeConstruction() {
    SnakeBody s(Point{5, 5}, 3, Direction::Right);
    check(s.size() == 3, "snake has the requested length");
    check(s.head() == (Point{5, 5}), "head sits at the start point");
    check(s.cells()[1] == (Point{4, 5}), "body trails behind the head");
    check(s.tail() == (Point{3, 5}), "tail is furthest back");
}

void testNoReversal() {
    SnakeBody s(Point{5, 5}, 3, Direction::Right);
    check(!s.setDirection(Direction::Left), "180-degree turn is rejected");
    check(s.direction() == Direction::Right, "direction survives a rejected turn");
    check(s.setDirection(Direction::Up), "perpendicular turn is accepted");
}

void testMoveAndGrow() {
    SnakeBody s(Point{5, 5}, 3, Direction::Right);
    s.move(Point{6, 5}, false);
    check(s.size() == 3, "plain move keeps the length");
    check(s.head() == (Point{6, 5}), "head advanced");
    check(s.tail() == (Point{4, 5}), "tail tip was dropped");

    s.move(Point{7, 5}, true);
    check(s.size() == 4, "growing move extends the body");
    check(s.tail() == (Point{4, 5}), "tail stays put while growing");
}

void testWrapping() {
    SnakeBody s(Point{0, 3}, 1, Direction::Left);
    check(s.nextHead(10, 10, true) == (Point{9, 3}), "wrap moves across the edge");
    check(s.nextHead(10, 10, false) == (Point{-1, 3}), "no-wrap leaves the board");
}

void testOccupies() {
    SnakeBody s(Point{5, 5}, 3, Direction::Right);
    check(s.occupies(Point{4, 5}), "body cell is occupied");
    check(!s.occupies(Point{3, 5}, /*ignoreTail=*/true), "tail tip is ignored on request");
    check(!s.occupies(Point{9, 9}), "free cell is not occupied");
}

// --- board ----------------------------------------------------------------

void testBoardBounds() {
    Board b(10, 6);
    check(b.outOfBounds(Point{-1, 0}), "negative x is out of bounds");
    check(b.outOfBounds(Point{10, 0}), "x == width is out of bounds");
    check(b.outOfBounds(Point{0, 6}), "y == height is out of bounds");
    check(!b.outOfBounds(Point{9, 5}), "bottom-right corner is in bounds");
    check(b.wrapped(Point{-1, -1}) == (Point{9, 5}), "wrapping maps -1 to the far edge");
}

void testFoodNeverOnSnake() {
    Board b(6, 4);
    std::mt19937 rng(7);
    SnakeBody s(Point{3, 2}, 4, Direction::Right);

    for (int i = 0; i < 200; ++i) {
        check(b.spawnFood(s, rng), "food found a free cell");
        if (s.occupies(b.food())) {
            check(false, "food never lands on the snake");
            return;
        }
    }
}

void testFullBoardHasNoFood() {
    Board b(2, 2);
    std::mt19937 rng(1);
    SnakeBody s(Point{0, 0}, 1, Direction::Right);
    s.move(Point{1, 0}, true);
    s.move(Point{1, 1}, true);
    s.move(Point{0, 1}, true);
    check(s.size() == 4, "snake covers the 2x2 board");
    check(!b.spawnFood(s, rng), "no food can spawn on a full board");
}

// --- game rules -----------------------------------------------------------

void testWallCollisionEndsGame() {
    Config cfg = testConfig();
    cfg.wrap = false;
    Game g(cfg);

    for (int i = 0; i < cfg.width + 5 && g.state() == GameState::Running; ++i) g.tick();
    check(g.state() == GameState::Lost, "running into a wall ends the game");
}

void testWrapKeepsGameAlive() {
    Config cfg = testConfig();
    cfg.wrap = true;
    Game g(cfg);

    for (int i = 0; i < cfg.width + 5; ++i) g.tick();
    check(g.state() != GameState::Lost, "wrap mode survives crossing the edge");
}

void testSelfCollision() {
    Config cfg = testConfig();
    cfg.initialLength = 6;
    Game g(cfg);

    // A tight square turns the head back into the body.
    g.requestDirection(Direction::Up);    g.tick();
    g.requestDirection(Direction::Left);  g.tick();
    g.requestDirection(Direction::Down);  g.tick();
    check(g.state() == GameState::Lost, "biting your own body ends the game");
}

void testTailChaseIsLegal() {
    Config cfg = testConfig();
    cfg.initialLength = 4;
    Game g(cfg);

    // Moving into the cell the tail tip is vacating must be allowed.
    g.requestDirection(Direction::Up);    g.tick();
    g.requestDirection(Direction::Left);  g.tick();
    check(g.state() == GameState::Running, "turning into the vacated tail cell is fine");
}

/// Greedy autopilot used by the integration tests: picks the legal move
/// (no reversal, no wall, no body) that gets closest to the food.
Direction chooseMove(const Game& g) {
    const Point head = g.snake().head();
    const Point food = g.board().food();
    const Direction current = g.snake().direction();

    Direction best = current;
    int bestDistance = -1;

    for (const Direction d : {Direction::Up, Direction::Down, Direction::Left, Direction::Right}) {
        if (g.snake().size() > 1 && opposite(d, current)) continue;

        const Point step = delta(d);
        const Point next{head.x + step.x, head.y + step.y};
        if (g.board().outOfBounds(next)) continue;
        if (g.snake().occupies(next, /*ignoreTail=*/true)) continue;

        const int distance = std::abs(next.x - food.x) + std::abs(next.y - food.y);
        if (bestDistance < 0 || distance < bestDistance) {
            bestDistance = distance;
            best = d;
        }
    }
    return best;
}

void testEatingScoresAndGrows() {
    Config cfg = testConfig();
    Game g(cfg);

    const std::size_t lengthBefore = g.snake().size();
    const int scoreBefore = g.score();

    bool ate = false;
    for (int i = 0; i < 400 && g.state() == GameState::Running; ++i) {
        g.requestDirection(chooseMove(g));
        g.tick();
        if (g.score() > scoreBefore) { ate = true; break; }
    }

    check(ate, "the snake can reach and eat food");
    if (ate) {
        check(g.score() == scoreBefore + cfg.pointsPerFood, "eating awards points");
        check(g.snake().size() == lengthBefore + 1, "eating grows the snake");
        check(g.foodEaten() == 1, "food counter increments");
    }
}

void testLevelUpAndSpeedUp() {
    Config cfg = testConfig();  // foodPerLevel == 2
    Game g(cfg);
    const auto startInterval = g.tickInterval();

    for (int i = 0; i < 3000 && g.state() == GameState::Running && g.foodEaten() < 4; ++i) {
        g.requestDirection(chooseMove(g));
        g.tick();
    }

    check(g.foodEaten() >= 4, "autopilot eats several food items");
    check(g.level() >= 3, "level rises once per foodPerLevel items");
    check(g.tickInterval() < startInterval, "higher levels tick faster");
    check(g.score() > cfg.pointsPerFood * 4, "later food is worth more (level multiplier)");
}

void testPauseBlocksTicks() {
    Game g(testConfig());
    g.togglePause();
    const Point head = g.snake().head();
    g.tick();
    check(g.state() == GameState::Paused, "pause holds the state");
    check(g.snake().head() == head, "a paused game does not move");

    g.togglePause();
    g.tick();
    check(g.snake().head() != head, "unpausing resumes movement");
}

void testDoubleTurnCannotReverse() {
    Game g(testConfig());
    // Heading right: Up then Left inside one tick must not become a reversal.
    g.requestDirection(Direction::Up);
    g.requestDirection(Direction::Left);
    g.tick();
    check(g.state() == GameState::Running, "two turns in one tick cannot fold the snake");
}

void testResetRestoresStartingState() {
    Config cfg = testConfig();
    Game g(cfg);
    for (int i = 0; i < 3; ++i) g.tick();
    g.reset();

    check(g.score() == 0, "reset clears the score");
    check(g.level() == 1, "reset returns to level 1");
    check(g.state() == GameState::Running, "reset resumes running");
    check(g.snake().size() == static_cast<std::size_t>(cfg.initialLength),
          "reset restores the starting length");
}

void testSpeedIncreasesWithLevel() {
    Config cfg = testConfig();
    Game g(cfg);
    const auto first = g.tickInterval();
    check(first.count() == cfg.baseTickMs, "level 1 uses the base interval");
    check(cfg.baseTickMs - cfg.speedStepMs < cfg.baseTickMs, "levels shorten the interval");
}

void testDeterministicSeed() {
    Config cfg = testConfig();
    Game a(cfg);
    Game b(cfg);
    check(a.board().food() == b.board().food(), "the same seed places food identically");
}

// --- persistence ----------------------------------------------------------

void testHighScoreRoundTrip() {
    const std::filesystem::path dir =
        std::filesystem::temp_directory_path() / "snake_test_scores";
    std::filesystem::remove_all(dir);
    const std::string path = (dir / "highscores.txt").string();

    {
        HighScoreTable table(path);
        check(table.load(), "loading a missing file is not an error");
        check(table.entries().empty(), "missing file yields an empty table");
        check(table.best() == 0, "empty table has a best of zero");

        check(table.add(ScoreEntry{"alice", 120, 3, "2026-01-01"}) == 1, "first score ranks 1st");
        check(table.add(ScoreEntry{"bob", 300, 5, "2026-01-02"}) == 1, "a better score takes 1st");
        check(table.add(ScoreEntry{"carol", 200, 4, "2026-01-03"}) == 2, "middling score ranks 2nd");
        check(table.best() == 300, "best reports the top score");
        check(table.save(), "saving creates the file and its directory");
    }

    HighScoreTable reloaded(path);
    check(reloaded.load(), "reload succeeds");
    check(reloaded.entries().size() == 3, "all entries round-trip");
    check(reloaded.entries()[0].name == "bob", "order survives the round trip");
    check(reloaded.entries()[0].score == 300, "scores survive the round trip");
    check(reloaded.entries()[0].level == 5, "levels survive the round trip");

    std::filesystem::remove_all(dir);
}

void testHighScoreCapAndQualification() {
    HighScoreTable table("/dev/null/never-written");
    for (int i = 0; i < 15; ++i) {
        table.add(ScoreEntry{"p" + std::to_string(i), (i + 1) * 10, 1, "2026-01-01"});
    }
    check(table.entries().size() == HighScoreTable::kMaxEntries, "table is capped");
    check(table.entries().front().score == 150, "highest score stays on top");
    check(!table.qualifies(5), "a low score does not qualify on a full table");
    check(table.qualifies(1000), "a big score always qualifies");
    check(!table.qualifies(0), "a zero score never qualifies");
}

void testNameSanitising() {
    check(sanitizeName("a|b") == "ab", "pipe is stripped from names");
    check(sanitizeName("  ") == "player", "blank names fall back");
    check(sanitizeName("line\nbreak") == "linebreak", "newlines are stripped");
    check(sanitizeName("abcdefghijklmnopqrstuvwxyz").size() <= 16, "names are truncated");
}

// --- input & rendering ----------------------------------------------------

void testInputMapping() {
    check(InputHandler::fromChar('w') == Command::Up, "w steers up");
    check(InputHandler::fromChar('S') == Command::Down, "uppercase works too");
    check(InputHandler::fromChar('q') == Command::Quit, "q quits");
    check(InputHandler::fromChar(' ') == Command::Pause, "space pauses");
    check(InputHandler::fromChar('z') == Command::None, "unmapped keys are ignored");
}

void testRendererFrameShape() {
    Config cfg = testConfig();
    cfg.color = false;
    cfg.ascii = true;
    Game g(cfg);
    HighScoreTable scores("/tmp/unused-snake-scores");
    Renderer renderer(std::cout);

    const std::string frame = renderer.frame(g, scores);
    std::size_t lines = 0;
    for (char c : frame) if (c == '\n') ++lines;

    // header(2) + top border + rows + bottom border + footer
    check(lines == static_cast<std::size_t>(cfg.height) + 5, "frame has the expected line count");
    check(frame.find("@@") != std::string::npos, "the head is drawn");
    check(frame.find("**") != std::string::npos, "the food is drawn");
    check(frame.find("\x1b[3") == std::string::npos, "no colour codes when colour is off");
}

// --- config ---------------------------------------------------------------

void testConfigSanitize() {
    Config cfg;
    cfg.width = 2;
    cfg.height = 1;
    cfg.baseTickMs = 5;
    check(!cfg.sanitize(), "sanitize reports that values were clamped");
    check(cfg.width >= 8 && cfg.height >= 6, "board is clamped to a playable size");
    check(cfg.minTickMs <= cfg.baseTickMs, "min tick never exceeds the base tick");
}

void testConfigArgs() {
    const char* argv[] = {"snake", "--width", "40", "--height=15", "--wrap", "--ascii"};
    Config cfg;
    bool help = false;
    check(Config::fromArgs(6, const_cast<char**>(argv), cfg, help), "valid args parse");
    check(!help, "no help requested");
    check(cfg.width == 40, "--width value parsed");
    check(cfg.height == 15, "--opt=value form parsed");
    check(cfg.wrap && cfg.ascii, "flags parsed");

    std::cout << "  (one 'bad --width' line below is expected)\n";
    const char* bad[] = {"snake", "--width", "abc"};
    Config cfg2;
    check(!Config::fromArgs(3, const_cast<char**>(bad), cfg2, help), "bad args are rejected");
}

}  // namespace

int main() {
    std::cout << "running snake tests\n";

    testSnakeConstruction();
    testNoReversal();
    testMoveAndGrow();
    testWrapping();
    testOccupies();

    testBoardBounds();
    testFoodNeverOnSnake();
    testFullBoardHasNoFood();

    testWallCollisionEndsGame();
    testWrapKeepsGameAlive();
    testSelfCollision();
    testTailChaseIsLegal();
    testEatingScoresAndGrows();
    testLevelUpAndSpeedUp();
    testPauseBlocksTicks();
    testDoubleTurnCannotReverse();
    testResetRestoresStartingState();
    testSpeedIncreasesWithLevel();
    testDeterministicSeed();

    testHighScoreRoundTrip();
    testHighScoreCapAndQualification();
    testNameSanitising();

    testInputMapping();
    testRendererFrameShape();

    testConfigSanitize();
    testConfigArgs();

    std::cout << (g_failures == 0 ? "OK   " : "FAIL ") << (g_checks - g_failures) << "/"
              << g_checks << " checks passed\n";
    return g_failures == 0 ? 0 : 1;
}
