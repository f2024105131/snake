#include "snake/Snake.hpp"

#include <algorithm>

namespace snake {

SnakeBody::SnakeBody(Point start, int initialLength, Direction dir) : dir_(dir) {
    const Point step = delta(dir);
    const int length = std::max(1, initialLength);

    // Body trails behind the head, opposite to the heading.
    for (int i = 0; i < length; ++i) {
        body_.push_back(Point{start.x - step.x * i, start.y - step.y * i});
    }
}

bool SnakeBody::setDirection(Direction d) {
    if (d == dir_) return false;
    if (body_.size() > 1 && opposite(d, dir_)) return false;
    dir_ = d;
    return true;
}

Point SnakeBody::nextHead(int boardWidth, int boardHeight, bool wrap) const {
    const Point step = delta(dir_);
    Point next{head().x + step.x, head().y + step.y};

    if (wrap && boardWidth > 0 && boardHeight > 0) {
        next.x = (next.x % boardWidth + boardWidth) % boardWidth;
        next.y = (next.y % boardHeight + boardHeight) % boardHeight;
    }
    return next;
}

void SnakeBody::move(Point newHead, bool grow) {
    body_.push_front(newHead);
    if (!grow && body_.size() > 1) {
        body_.pop_back();
    }
}

bool SnakeBody::occupies(Point p, bool ignoreTail) const {
    if (body_.empty()) return false;

    const auto end = (ignoreTail && body_.size() > 1) ? std::prev(body_.end()) : body_.end();
    return std::find(body_.begin(), end, p) != end;
}

}  // namespace snake
