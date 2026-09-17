#include "snake/Board.hpp"

#include <algorithm>
#include <vector>

namespace snake {

Board::Board(int width, int height)
    : width_(std::max(1, width)), height_(std::max(1, height)) {}

bool Board::outOfBounds(Point p) const {
    return p.x < 0 || p.y < 0 || p.x >= width_ || p.y >= height_;
}

Point Board::wrapped(Point p) const {
    return Point{(p.x % width_ + width_) % width_, (p.y % height_ + height_) % height_};
}

void Board::setFood(Point p) {
    food_ = p;
    hasFood_ = !outOfBounds(p);
}

bool Board::spawnFood(const SnakeBody& body, std::mt19937& rng) {
    // Building the free list is O(cells) but exact. Rejection sampling gets
    // pathologically slow once the snake fills most of the board, which is
    // precisely when the endgame matters most.
    std::vector<Point> free;
    free.reserve(static_cast<std::size_t>(cellCount()) - body.size());

    std::vector<char> taken(static_cast<std::size_t>(cellCount()), 0);
    for (const Point& c : body.cells()) {
        if (!outOfBounds(c)) taken[static_cast<std::size_t>(c.y) * width_ + c.x] = 1;
    }

    for (int y = 0; y < height_; ++y) {
        for (int x = 0; x < width_; ++x) {
            if (!taken[static_cast<std::size_t>(y) * width_ + x]) free.push_back(Point{x, y});
        }
    }

    if (free.empty()) {
        hasFood_ = false;
        return false;
    }

    std::uniform_int_distribution<std::size_t> pick(0, free.size() - 1);
    setFood(free[pick(rng)]);
    return true;
}

}  // namespace snake
