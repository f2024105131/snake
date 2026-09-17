#pragma once

#include <string>

namespace snake {

/// Everything tunable in one place, so the game logic never reads globals.
struct Config {
    int  width          = 30;    ///< board columns
    int  height         = 20;    ///< board rows
    int  initialLength  = 4;     ///< starting snake length
    bool wrap           = false; ///< walls teleport instead of killing
    bool ascii          = false; ///< ASCII glyphs instead of block characters
    bool color          = true;  ///< ANSI colour output

    int  baseTickMs     = 140;   ///< tick interval at level 1
    int  minTickMs      = 55;    ///< fastest tick interval
    int  speedStepMs    = 8;     ///< ms removed per level
    int  foodPerLevel   = 5;     ///< food items needed to level up
    int  pointsPerFood  = 10;    ///< multiplied by the current level

    unsigned    seed = 0;        ///< 0 means "seed from the clock"
    std::string highScoreFile;   ///< empty means defaultHighScorePath()
    std::string playerName;      ///< empty means "ask / fall back to $USER"

    /// Parses argv. Returns false on a bad argument (message written to
    /// stderr). `helpRequested` is set when --help was passed.
    static bool fromArgs(int argc, char** argv, Config& out, bool& helpRequested);

    /// Human readable option list for --help.
    static std::string usage(const char* programName);

    /// $XDG_DATA_HOME/snake/highscores.txt, ~/.local/share/snake/... or
    /// ./highscores.txt as a last resort.
    static std::string defaultHighScorePath();

    /// Clamps values into sane ranges; returns false if something was fixed.
    bool sanitize();
};

}  // namespace snake
