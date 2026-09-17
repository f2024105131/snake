#pragma once

#include <random>

#include "snake/Snake.hpp"
#include "snake/Types.hpp"

namespace snake {

/// The playfield: bounds checking and food placement.
class Board {
public:
    Board(int width, int height);

    int width() const { return width_; }
    int height() const { return height_; }
    int cellCount() const { return width_ * height_; }

    bool outOfBounds(Point p) const;
    Point wrapped(Point p) const;

    Point food() const { return food_; }
    bool hasFood() const { return hasFood_; }
    void setFood(Point p);

    /// Places food on a uniformly chosen free cell. Returns false when the
    /// snake covers the whole board -- that is the win condition.
    bool spawnFood(const SnakeBody& body, std::mt19937& rng);

private:
    int width_;
    int height_;
    Point food_{0, 0};
    bool hasFood_ = false;
};

}  // namespace snake
