#pragma once

#include <chrono>
#include <random>

#include "snake/Board.hpp"
#include "snake/Config.hpp"
#include "snake/Snake.hpp"
#include "snake/Types.hpp"

namespace snake {

/// The rules engine. It has no I/O at all: the caller decides when to tick,
/// and the renderer reads state out of it. That is what makes it unit
/// testable with a fixed seed.
class Game {
public:
    explicit Game(const Config& cfg);

    /// Queues a turn. It is applied at the start of the next tick, so two
    /// keypresses inside one tick cannot fold the snake back onto itself.
    void requestDirection(Direction d);

    /// Advances the simulation one step. No-op unless the state is Running.
    void tick();

    void togglePause();
    void quit();
    void reset();

    GameState state() const { return state_; }
    bool isOver() const { return state_ == GameState::Lost || state_ == GameState::Won; }

    int score() const { return score_; }
    int level() const { return level_; }
    int foodEaten() const { return foodEaten_; }
    long long steps() const { return steps_; }

    const SnakeBody& snake() const { return snake_; }
    const Board& board() const { return board_; }
    const Config& config() const { return cfg_; }

    /// Current delay between ticks; shrinks as the level goes up.
    std::chrono::milliseconds tickInterval() const;

private:
    Config      cfg_;
    Board       board_;
    SnakeBody   snake_;
    std::mt19937 rng_;

    Direction pending_;
    GameState state_     = GameState::Running;
    int       score_     = 0;
    int       level_     = 1;
    int       foodEaten_ = 0;
    long long steps_     = 0;

    static SnakeBody makeSnake(const Config& cfg);
    unsigned resolveSeed() const;
};

}  // namespace snake
