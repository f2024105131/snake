#pragma once

#include <string>
#include <vector>

namespace snake {

struct ScoreEntry {
    std::string name  = "player";
    int         score = 0;
    int         level = 1;
    std::string date;  ///< YYYY-MM-DD

    static std::string today();
};

/// Top-N scoreboard persisted as one pipe-separated record per line:
///     name|score|level|YYYY-MM-DD
/// Malformed lines are skipped rather than aborting the load, so a corrupt
/// file never stops you from playing.
class HighScoreTable {
public:
    static constexpr std::size_t kMaxEntries = 10;

    explicit HighScoreTable(std::string path);

    const std::string& path() const { return path_; }
    const std::vector<ScoreEntry>& entries() const { return entries_; }

    /// Missing file is not an error -- it just means an empty table.
    bool load();
    /// Creates parent directories when needed. False on a write failure.
    bool save() const;

    int  best() const;
    bool qualifies(int score) const;
    /// Inserts, re-sorts and truncates to kMaxEntries. Returns the 1-based
    /// rank, or 0 when the score did not make the table.
    std::size_t add(const ScoreEntry& entry);

    const std::string& lastError() const { return lastError_; }

private:
    std::string             path_;
    std::vector<ScoreEntry> entries_;
    mutable std::string     lastError_;
};

/// Escapes '|' and newlines so names can never corrupt the record format.
std::string sanitizeName(const std::string& raw);

}  // namespace snake
