#include "snake/Terminal.hpp"

#include <csignal>
#include <cstdio>
#include <iostream>

#if defined(_WIN32)
#  include <windows.h>
#  include <io.h>
#else
#  include <sys/ioctl.h>
#  include <termios.h>
#  include <unistd.h>
#endif

namespace snake {
namespace {

volatile std::sig_atomic_t g_interrupted = 0;

extern "C" void handleSignal(int) { g_interrupted = 1; }

}  // namespace

struct Terminal::Impl {
#if defined(_WIN32)
    HANDLE inHandle  = INVALID_HANDLE_VALUE;
    HANDLE outHandle = INVALID_HANDLE_VALUE;
    DWORD  savedIn   = 0;
    DWORD  savedOut  = 0;
    bool   saved     = false;
#else
    termios saved{};
    bool    hasSaved = false;
#endif
};

Terminal::Terminal() : impl_(std::make_unique<Impl>()) {
    std::signal(SIGINT, handleSignal);
    std::signal(SIGTERM, handleSignal);

#if defined(_WIN32)
    impl_->inHandle  = GetStdHandle(STD_INPUT_HANDLE);
    impl_->outHandle = GetStdHandle(STD_OUTPUT_HANDLE);
    if (GetConsoleMode(impl_->inHandle, &impl_->savedIn) &&
        GetConsoleMode(impl_->outHandle, &impl_->savedOut)) {
        impl_->saved = true;
        // Needed for ANSI escape sequences on Windows 10+.
        SetConsoleMode(impl_->outHandle,
                       impl_->savedOut | ENABLE_VIRTUAL_TERMINAL_PROCESSING);
    }
#else
    if (tcgetattr(STDIN_FILENO, &impl_->saved) == 0) impl_->hasSaved = true;
#endif

    setAlternateScreen(true);
    showCursor(false);
    setRaw(true);
    clear();
}

Terminal::~Terminal() {
    // Order matters: leave raw mode before restoring the normal screen so a
    // crashed run never leaves the shell without echo.
    setRaw(false);
    showCursor(true);
    setAlternateScreen(false);
    std::cout.flush();
}

void Terminal::setRaw(bool enabled) {
    if (enabled == raw_) return;

#if defined(_WIN32)
    if (impl_->saved) {
        DWORD mode = impl_->savedIn;
        if (enabled) {
            mode &= ~(ENABLE_ECHO_INPUT | ENABLE_LINE_INPUT | ENABLE_PROCESSED_INPUT);
        }
        SetConsoleMode(impl_->inHandle, mode);
    }
#else
    if (!impl_->hasSaved) return;

    if (enabled) {
        termios raw = impl_->saved;
        raw.c_lflag &= ~(ECHO | ICANON | IEXTEN);  // no echo, no line buffering
        raw.c_iflag &= ~(IXON | ICRNL);            // no flow control, raw CR
        raw.c_cc[VMIN]  = 0;                       // read() returns immediately
        raw.c_cc[VTIME] = 0;
        tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
    } else {
        tcsetattr(STDIN_FILENO, TCSAFLUSH, &impl_->saved);
    }
#endif
    raw_ = enabled;
}

void Terminal::setAlternateScreen(bool enabled) {
    if (enabled == alt_) return;
    std::cout << (enabled ? "\x1b[?1049h" : "\x1b[?1049l") << std::flush;
    alt_ = enabled;
}

void Terminal::showCursor(bool visible) {
    std::cout << (visible ? "\x1b[?25h" : "\x1b[?25l") << std::flush;
}

void Terminal::clear() {
    std::cout << "\x1b[2J\x1b[H" << std::flush;
}

TerminalSize Terminal::size() const {
    TerminalSize s;
#if defined(_WIN32)
    CONSOLE_SCREEN_BUFFER_INFO info;
    if (GetConsoleScreenBufferInfo(impl_->outHandle, &info)) {
        s.cols = info.srWindow.Right - info.srWindow.Left + 1;
        s.rows = info.srWindow.Bottom - info.srWindow.Top + 1;
    }
#else
    winsize ws{};
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == 0 && ws.ws_col > 0) {
        s.cols = ws.ws_col;
        s.rows = ws.ws_row;
    }
#endif
    return s;
}

bool Terminal::isTty() const {
#if defined(_WIN32)
    return _isatty(_fileno(stdin)) != 0;
#else
    return isatty(STDIN_FILENO) != 0;
#endif
}

bool Terminal::interrupted() { return g_interrupted != 0; }
void Terminal::clearInterrupt() { g_interrupted = 0; }

}  // namespace snake
