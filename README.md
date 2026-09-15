# Terminal Snake (C++17)

A terminal Snake game with a hard separation between game logic and
rendering: the rules engine has no I/O at all, so the whole thing is unit
tested headlessly (287 checks, no test framework dependency).

```
  S N A K E
  Score 120    Best 300    Level 3    Len 16

  ┌────────────────────────────────────────┐
  │                                        │
  │                    ▒▒                  │
  │                    ▒▒        ◆◆        │
  │                    ▒▒                  │
  │                    ██                  │
  └────────────────────────────────────────┘
  arrows/WASD move  -  p pause  -  r restart  -  q quit
```

## Project structure

```
snake/
├── CMakeLists.txt              # CMake build (library + game + tests)
├── Makefile                    # plain make build: all / run / test / debug
├── README.md
├── .gitignore
├── include/
│   └── snake/
│       ├── Types.hpp           # Point, Direction, GameState, Command
│       ├── Config.hpp          # tunables + CLI parsing
│       ├── Snake.hpp           # snake body (deque of cells)
│       ├── Board.hpp           # bounds + food placement
│       ├── Game.hpp            # rules engine (no I/O)
│       ├── HighScore.hpp       # scoreboard + file persistence
│       ├── Terminal.hpp        # RAII raw mode / alt screen
│       ├── InputHandler.hpp    # non-blocking key parsing
│       └── Renderer.hpp        # frame buffer -> ostream
├── src/
│   ├── main.cpp                # arg handling + game loop
│   ├── Config.cpp
│   ├── Snake.cpp
│   ├── Board.cpp
│   ├── Game.cpp
│   ├── HighScore.cpp
│   ├── Terminal.cpp
│   ├── InputHandler.cpp
│   └── Renderer.cpp
└── tests/
    └── test_game_logic.cpp     # headless test runner
```

## Build and run

With make:

```bash
make            # builds bin/snake
make run        # build and play
make test       # build and run the unit tests
make debug      # -O0 -g with ASan/UBSan, runs the tests
```

With CMake:

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
./build/snake
ctest --test-dir build --output-on-failure
```

Requires a C++17 compiler. POSIX (Linux/macOS) uses `termios`; there is a
Windows path using the console API and `conio.h`.

## Controls

| Key | Action |
| --- | --- |
| arrows, WASD, HJKL | steer |
| `p` or space | pause / resume |
| `r` | restart |
| `q` or Ctrl-C | quit |

## Options

```
--width N      board columns (default 30)
--height N     board rows (default 20)
--length N     starting snake length (default 4)
--speed MS     tick interval at level 1 (default 140)
--wrap         edges teleport instead of ending the run
--ascii        ASCII glyphs instead of block characters
--no-color     disable ANSI colour
--seed N       fixed RNG seed (reproducible food placement)
--name NAME    skip the high-score name prompt
--scores PATH  high-score file location
```

## How it fits together

```
main.cpp
   │  poll                     read state
   ├──────────► InputHandler        ▲
   │                                │
   ├──────────► Game ───────────────┤
   │            (Board, SnakeBody)  │
   │                                │
   └──────────► Renderer ───────────┘ ──► stdout
                Terminal (RAII mode switch)
```

**Logic / rendering split.** `Game` owns `Board` and `SnakeBody` and exposes
`tick()`, `requestDirection()` and read-only accessors. It never touches
stdout, never sleeps and never reads a key, so `tests/` drives it directly
with a fixed seed.

**Loop timing.** The loop spins every ~4 ms and drains input every pass, but
only calls `game.tick()` once `tickInterval()` has elapsed. Steering feels
instant even at level 1 speeds, and the game speed is frame-rate independent.

**Flicker-free drawing.** `Renderer` builds the entire frame into one
`std::string` and writes it after a cursor-home escape, with `\x1b[K` on each
line to erase leftovers. Writing cell by cell is what makes terminal games
tear.

**Input.** Raw mode with `VMIN=0, VTIME=0` makes `read()` return immediately
when nothing is pending. Arrow keys arrive as `ESC [ A..D` and the sequence
can be split across two reads, so the parser keeps a three-state machine
between calls.

**Terminal safety.** `Terminal` is RAII: the constructor enters the alternate
screen and raw mode, the destructor always restores them. `SIGINT` sets a
flag the loop checks, so Ctrl-C exits through the destructor instead of
leaving your shell without echo.

**Turn validation.** A queued direction is checked against the *committed*
heading, not the pending one, so pressing Up then Left inside a single tick
cannot fold the snake into itself.

**Tail chasing.** On a non-growing step the tail tip vacates its cell during
the same tick, so moving into it is legal. On a growing step it is not. The
`ignoreTail` flag on `SnakeBody::occupies` encodes exactly that.

**Food placement.** `Board::spawnFood` collects free cells and picks one
uniformly. Rejection sampling gets pathologically slow once the snake covers
most of the board, which is exactly when the endgame matters. An empty free
list means the board is full, which is the win condition.

**Scoring.** Each food is worth `pointsPerFood × level`. Every 5 items adds a
level and shaves 8 ms off the tick interval down to a 55 ms floor.

**Persistence.** `HighScoreTable` keeps the top 10 in
`$XDG_DATA_HOME/snake/highscores.txt` (falling back to
`~/.local/share/snake/`) as `name|score|level|date` records. Writes go to a
temp file and are renamed into place, so an interrupted save cannot truncate
the scoreboard. Corrupt lines are skipped on load rather than aborting, and
names are sanitised so a `|` in a name can never break the format.

## Suggested commit sequence

1. **Terminal game loop and grid rendering** — `Types.hpp`, `Config`,
   `Board`, `SnakeBody`, `Renderer`, `Terminal`, plus a `main` that renders a
   static board on a fixed timer.
2. **Non-blocking input handling** — `InputHandler` with the escape-sequence
   state machine, `VMIN=0/VTIME=0` raw mode, and the steer/pause/quit
   commands wired into the loop.
3. **Collision detection and score tracking** — `Game::tick` with wall,
   self and tail-chase rules, food consumption, scoring, levels, speed-up,
   win/loss states, and `tests/test_game_logic.cpp`.
4. **High-score persistence** — `HighScore` load/save, the name prompt on
   game over, atomic writes, and the `--scores` / `--name` options.

## Testing

`make test` runs the suite. It covers body movement and growth, rejected
reversals, wall/self/tail-chase collisions, wrap mode, food never landing on
the snake, the full-board win condition, scoring and level progression,
pause, reset, deterministic seeding, scoreboard sorting/capping/round-trip,
name sanitising, key mapping, frame shape, and CLI parsing.
