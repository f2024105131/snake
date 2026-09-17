#include "snake/HighScore.hpp"

#include <algorithm>
#include <cctype>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <sstream>

namespace snake {
namespace {

int toInt(const std::string& text, int fallback = -1) {
    try {
        std::size_t consumed = 0;
        const int value = std::stoi(text, &consumed);
        return consumed == text.size() ? value : fallback;
    } catch (...) {
        return fallback;
    }
}

}  // namespace

std::string ScoreEntry::today() {
    const std::time_t now = std::time(nullptr);
    std::tm tm{};
#if defined(_WIN32)
    localtime_s(&tm, &now);
#else
    localtime_r(&now, &tm);
#endif
    char buf[16] = {0};
    std::strftime(buf, sizeof(buf), "%Y-%m-%d", &tm);
    return std::string(buf);
}

std::string sanitizeName(const std::string& raw) {
    std::string out;
    out.reserve(raw.size());

    for (char c : raw) {
        const unsigned char u = static_cast<unsigned char>(c);
        if (c == '|' || c == '\n' || c == '\r') continue;      // record separators
        if (u < 0x20) continue;                                 // control chars
        out.push_back(c);
        if (out.size() >= 16) break;
    }

    // Trim surrounding blanks.
    const auto first = out.find_first_not_of(' ');
    const auto last  = out.find_last_not_of(' ');
    if (first == std::string::npos) return "player";
    out = out.substr(first, last - first + 1);

    return out.empty() ? "player" : out;
}

HighScoreTable::HighScoreTable(std::string path) : path_(std::move(path)) {}

bool HighScoreTable::load() {
    entries_.clear();
    lastError_.clear();

    std::ifstream in(path_);
    if (!in) {
        // No file yet is the normal first-run case, not a failure.
        return true;
    }

    std::string line;
    while (std::getline(in, line)) {
        if (line.empty() || line[0] == '#') continue;

        std::stringstream ss(line);
        std::string name, score, level, date;
        if (!std::getline(ss, name, '|'))  continue;
        if (!std::getline(ss, score, '|')) continue;
        std::getline(ss, level, '|');
        std::getline(ss, date, '|');

        const int scoreValue = toInt(score);
        if (scoreValue < 0) continue;  // skip corrupt record

        ScoreEntry entry;
        entry.name  = sanitizeName(name);
        entry.score = scoreValue;
        entry.level = std::max(1, toInt(level, 1));
        entry.date  = date.empty() ? "-" : date;
        entries_.push_back(entry);
    }

    std::stable_sort(entries_.begin(), entries_.end(),
                     [](const ScoreEntry& a, const ScoreEntry& b) { return a.score > b.score; });
    if (entries_.size() > kMaxEntries) entries_.resize(kMaxEntries);
    return true;
}

bool HighScoreTable::save() const {
    lastError_.clear();

    std::error_code ec;
    const std::filesystem::path p(path_);
    if (p.has_parent_path() && !p.parent_path().empty()) {
        std::filesystem::create_directories(p.parent_path(), ec);
        if (ec) {
            lastError_ = "cannot create " + p.parent_path().string() + ": " + ec.message();
            return false;
        }
    }

    // Write to a temp file then rename, so a crash mid-write cannot leave a
    // truncated scoreboard behind.
    const std::filesystem::path tmp = p.string() + ".tmp";
    {
        std::ofstream out(tmp, std::ios::trunc);
        if (!out) {
            lastError_ = "cannot write " + tmp.string();
            return false;
        }
        out << "# snake high scores: name|score|level|date\n";
        for (const ScoreEntry& e : entries_) {
            out << sanitizeName(e.name) << '|' << e.score << '|' << e.level << '|'
                << (e.date.empty() ? "-" : e.date) << '\n';
        }
        if (!out) {
            lastError_ = "write failed for " + tmp.string();
            return false;
        }
    }

    std::filesystem::rename(tmp, p, ec);
    if (ec) {
        lastError_ = "cannot replace " + p.string() + ": " + ec.message();
        std::filesystem::remove(tmp, ec);
        return false;
    }
    return true;
}

int HighScoreTable::best() const {
    return entries_.empty() ? 0 : entries_.front().score;
}

bool HighScoreTable::qualifies(int score) const {
    if (score <= 0) return false;
    if (entries_.size() < kMaxEntries) return true;
    return score > entries_.back().score;
}

std::size_t HighScoreTable::add(const ScoreEntry& entry) {
    if (!qualifies(entry.score)) return 0;

    ScoreEntry copy = entry;
    copy.name = sanitizeName(copy.name);
    if (copy.date.empty()) copy.date = ScoreEntry::today();

    entries_.push_back(copy);
    std::stable_sort(entries_.begin(), entries_.end(),
                     [](const ScoreEntry& a, const ScoreEntry& b) { return a.score > b.score; });
    if (entries_.size() > kMaxEntries) entries_.resize(kMaxEntries);

    for (std::size_t i = 0; i < entries_.size(); ++i) {
        if (entries_[i].score == copy.score && entries_[i].name == copy.name) {
            return i + 1;
        }
    }
    return 0;
}

}  // namespace snake
