#pragma once

#include <memory>

namespace snake {

struct TerminalSize {
    int rows = 24;
    int cols = 80;
};

/// RAII wrapper around the terminal mode. Construction switches to the
/// alternate screen buffer and raw mode; destruction always puts the
/// terminal back, including on an exception or Ctrl-C.
class Terminal {
public:
    Terminal();
    ~Terminal();

    Terminal(const Terminal&)            = delete;
    Terminal& operator=(const Terminal&) = delete;

    void setRaw(bool enabled);
    bool raw() const { return raw_; }

    void setAlternateScreen(bool enabled);
    void showCursor(bool visible);
    void clear();

    TerminalSize size() const;
    bool isTty() const;

    /// Set by the SIGINT handler so the main loop can exit cleanly instead
    /// of dying with the terminal left in raw mode.
    static bool interrupted();
    static void clearInterrupt();

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
    bool raw_ = false;
    bool alt_ = false;
};

}  // namespace snake
