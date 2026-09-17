#include "snake/Game.hpp"

#include <algorithm>
#include <chrono>

namespace snake {

SnakeBody Game::makeSnake(const Config& cfg) {
    // Start centred, heading right, with room for the tail behind the head.
    const Point start{std::max(cfg.initialLength - 1, cfg.width / 2), cfg.height / 2};
    return SnakeBody(start, cfg.initialLength, Direction::Right);
}

unsigned Game::resolveSeed() const {
    if (cfg_.seed != 0) return cfg_.seed;
    using clock = std::chrono::high_resolution_clock;
    return static_cast<unsigned>(clock::now().time_since_epoch().count());
}

Game::Game(const Config& cfg)
    : cfg_(cfg),
      board_(cfg.width, cfg.height),
      snake_(makeSnake(cfg)),
      rng_(0),
      pending_(Direction::Right) {
    cfg_.sanitize();
    rng_.seed(resolveSeed());
    board_.spawnFood(snake_, rng_);
}

void Game::reset() {
    board_ = Board(cfg_.width, cfg_.height);
    snake_ = makeSnake(cfg_);
    pending_ = Direction::Right;
    state_ = GameState::Running;
    score_ = 0;
    level_ = 1;
    foodEaten_ = 0;
    steps_ = 0;
    board_.spawnFood(snake_, rng_);
}

void Game::requestDirection(Direction d) {
    if (state_ == GameState::Paused) state_ = GameState::Running;
    if (state_ != GameState::Running) return;

    // Compare against the committed heading, not the pending one: pressing
    // Up then Left within a single tick must not become a reversal.
    if (snake_.size() > 1 && opposite(d, snake_.direction())) return;
    pending_ = d;
}

void Game::togglePause() {
    if (state_ == GameState::Running)      state_ = GameState::Paused;
    else if (state_ == GameState::Paused)  state_ = GameState::Running;
}

void Game::quit() { state_ = GameState::Quit; }

void Game::tick() {
    if (state_ != GameState::Running) return;

    snake_.setDirection(pending_);

    const Point next = snake_.nextHead(board_.width(), board_.height(), cfg_.wrap);

    if (!cfg_.wrap && board_.outOfBounds(next)) {
        state_ = GameState::Lost;
        return;
    }

    const bool eats = board_.hasFood() && next == board_.food();

    // On a normal step the tail tip vacates its cell this same tick, so
    // moving into it is legal. On a growing step it does not.
    if (snake_.occupies(next, /*ignoreTail=*/!eats)) {
        state_ = GameState::Lost;
        return;
    }

    snake_.move(next, eats);
    ++steps_;

    if (eats) {
        ++foodEaten_;
        score_ += cfg_.pointsPerFood * level_;
        if (foodEaten_ % cfg_.foodPerLevel == 0) ++level_;

        if (!board_.spawnFood(snake_, rng_)) {
            state_ = GameState::Won;  // board is full
        }
    }
}

std::chrono::milliseconds Game::tickInterval() const {
    const int ms = std::max(cfg_.minTickMs, cfg_.baseTickMs - (level_ - 1) * cfg_.speedStepMs);
    return std::chrono::milliseconds(ms);
}

}  // namespace snake
