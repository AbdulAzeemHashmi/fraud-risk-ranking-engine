#pragma once

#include <cstdint>

namespace midnight_queue {

struct Record {
    std::uint64_t txn_id{};
    double time{};
    double amount{};
    int band{};
    int fraud{}; // Evaluation only; never inspected by the triage comparator.
};

enum class KeyMode { Triage, BandOnly };

// Returns true when lhs must appear before rhs in the output queue.
struct HigherPriority {
    KeyMode mode{KeyMode::Triage};

    bool operator()(const Record& lhs, const Record& rhs) const {
        if (lhs.band != rhs.band) return lhs.band > rhs.band;
        if (mode == KeyMode::BandOnly) return false;
        if (lhs.amount != rhs.amount) return lhs.amount > rhs.amount;
        if (lhs.time != rhs.time) return lhs.time < rhs.time;
        return lhs.txn_id < rhs.txn_id;
    }
};

} // namespace midnight_queue
