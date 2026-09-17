#pragma once

#include <cstddef>
#include <deque>

#include "snake/Types.hpp"

namespace snake {

/// The snake body. front() is the head, back() is the tail tip.
/// Pure data + movement; it knows nothing about food, score or walls.
class SnakeBody {
public:
    SnakeBody(Point start, int initialLength, Direction dir);

    const std::deque<Point>& cells() const { return body_; }
    Point head() const { return body_.front(); }
    Point tail() const { return body_.back(); }
    Direction direction() const { return dir_; }
    std::size_t size() const { return body_.size(); }

    /// Ignores 180-degree turns, which would be instant self-collision.
    /// Returns true when the direction actually changed.
    bool setDirection(Direction d);

    /// Where the head would land next tick. Wraps around the board edges
    /// when `wrap` is true; otherwise may return an off-board point.
    Point nextHead(int boardWidth, int boardHeight, bool wrap) const;

    /// Moves the head to `newHead`. When `grow` is false the tail tip is
    /// dropped, keeping the length constant.
    void move(Point newHead, bool grow);

    /// `ignoreTail` skips the tail tip, which is about to move away on a
    /// non-growing step and therefore is not a real obstacle.
    bool occupies(Point p, bool ignoreTail = false) const;

private:
    std::deque<Point> body_;
    Direction dir_;
};

}  // namespace snake
