#pragma once

// Types.hpp -- small value types shared by the logic and rendering layers.
// Deliberately dependency-free so the game logic stays testable without a
// terminal attached.

namespace snake {

struct Point {
    int x = 0;
    int y = 0;
};

inline bool operator==(const Point& a, const Point& b) {
    return a.x == b.x && a.y == b.y;
}

inline bool operator!=(const Point& a, const Point& b) {
    return !(a == b);
}

enum class Direction { Up, Down, Left, Right };

/// Unit vector for a direction. y grows downwards (row 0 is the top row).
inline Point delta(Direction d) {
    switch (d) {
        case Direction::Up:    return Point{0, -1};
        case Direction::Down:  return Point{0, 1};
        case Direction::Left:  return Point{-1, 0};
        case Direction::Right: return Point{1, 0};
    }
    return Point{0, 0};
}

inline bool opposite(Direction a, Direction b) {
    return (a == Direction::Up && b == Direction::Down) ||
           (a == Direction::Down && b == Direction::Up) ||
           (a == Direction::Left && b == Direction::Right) ||
           (a == Direction::Right && b == Direction::Left);
}

enum class GameState { Running, Paused, Lost, Won, Quit };

/// Commands produced by the input layer. Keeping this separate from
/// Direction means the input layer never needs to know the game rules.
enum class Command { None, Up, Down, Left, Right, Pause, Restart, Quit };

}  // namespace snake
