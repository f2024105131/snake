#include "snake/Config.hpp"

#include <algorithm>
#include <cstdlib>
#include <iostream>
#include <string>

namespace snake {
namespace {

bool parseInt(const std::string& text, int& out) {
    try {
        std::size_t consumed = 0;
        int value = std::stoi(text, &consumed);
        if (consumed != text.size()) return false;
        out = value;
        return true;
    } catch (...) {
        return false;
    }
}

/// Reads the value for "--opt value" or "--opt=value".
bool takeValue(int argc, char** argv, int& i, const std::string& arg,
               const std::string& name, std::string& value) {
    if (arg == name) {
        if (i + 1 >= argc) return false;
        value = argv[++i];
        return true;
    }
    if (arg.rfind(name + "=", 0) == 0) {
        value = arg.substr(name.size() + 1);
        return true;
    }
    return false;
}

}  // namespace

bool Config::fromArgs(int argc, char** argv, Config& out, bool& helpRequested) {
    helpRequested = false;

    for (int i = 1; i < argc; ++i) {
        const std::string arg = argv[i];
        std::string value;

        if (arg == "-h" || arg == "--help") {
            helpRequested = true;
            return true;
        }
        if (arg == "--wrap")      { out.wrap = true;   continue; }
        if (arg == "--no-wrap")   { out.wrap = false;  continue; }
        if (arg == "--ascii")     { out.ascii = true;  continue; }
        if (arg == "--no-color" || arg == "--no-colour") { out.color = false; continue; }

        if (takeValue(argc, argv, i, arg, "--width", value)) {
            if (!parseInt(value, out.width)) { std::cerr << "snake: bad --width: " << value << "\n"; return false; }
            continue;
        }
        if (takeValue(argc, argv, i, arg, "--height", value)) {
            if (!parseInt(value, out.height)) { std::cerr << "snake: bad --height: " << value << "\n"; return false; }
            continue;
        }
        if (takeValue(argc, argv, i, arg, "--speed", value)) {
            if (!parseInt(value, out.baseTickMs)) { std::cerr << "snake: bad --speed: " << value << "\n"; return false; }
            continue;
        }
        if (takeValue(argc, argv, i, arg, "--length", value)) {
            if (!parseInt(value, out.initialLength)) { std::cerr << "snake: bad --length: " << value << "\n"; return false; }
            continue;
        }
        if (takeValue(argc, argv, i, arg, "--seed", value)) {
            int seed = 0;
            if (!parseInt(value, seed) || seed < 0) { std::cerr << "snake: bad --seed: " << value << "\n"; return false; }
            out.seed = static_cast<unsigned>(seed);
            continue;
        }
        if (takeValue(argc, argv, i, arg, "--scores", value)) { out.highScoreFile = value; continue; }
        if (takeValue(argc, argv, i, arg, "--name", value))   { out.playerName = value;    continue; }

        std::cerr << "snake: unknown option '" << arg << "' (try --help)\n";
        return false;
    }

    if (out.highScoreFile.empty()) out.highScoreFile = defaultHighScorePath();
    return true;
}

std::string Config::usage(const char* programName) {
    std::string name = programName ? programName : "snake";
    return
        "Terminal Snake\n"
        "\n"
        "Usage: " + name + " [options]\n"
        "\n"
        "Options:\n"
        "  --width N        board columns (default 30, min 8)\n"
        "  --height N       board rows (default 20, min 6)\n"
        "  --length N       starting snake length (default 4)\n"
        "  --speed MS       tick interval at level 1 in ms (default 140)\n"
        "  --wrap           edges teleport instead of ending the run\n"
        "  --ascii          ASCII glyphs instead of block characters\n"
        "  --no-color       disable ANSI colour\n"
        "  --seed N         fixed RNG seed (reproducible food placement)\n"
        "  --name NAME      skip the high-score name prompt\n"
        "  --scores PATH    high-score file (default " + defaultHighScorePath() + ")\n"
        "  -h, --help       show this help\n"
        "\n"
        "Controls:\n"
        "  arrows / WASD    steer        p or space   pause\n"
        "  r                restart      q            quit\n";
}

std::string Config::defaultHighScorePath() {
    if (const char* xdg = std::getenv("XDG_DATA_HOME"); xdg && *xdg) {
        return std::string(xdg) + "/snake/highscores.txt";
    }
    if (const char* home = std::getenv("HOME"); home && *home) {
        return std::string(home) + "/.local/share/snake/highscores.txt";
    }
    return "highscores.txt";
}

bool Config::sanitize() {
    const Config before = *this;

    width         = std::clamp(width, 8, 200);
    height        = std::clamp(height, 6, 100);
    initialLength = std::clamp(initialLength, 1, std::max(1, width - 2));
    baseTickMs    = std::clamp(baseTickMs, 20, 1000);
    minTickMs     = std::clamp(minTickMs, 10, baseTickMs);
    speedStepMs   = std::clamp(speedStepMs, 0, 100);
    foodPerLevel  = std::max(1, foodPerLevel);
    pointsPerFood = std::max(1, pointsPerFood);

    return width == before.width && height == before.height &&
           initialLength == before.initialLength && baseTickMs == before.baseTickMs;
}

}  // namespace snake
