#pragma once

#include "midnight_queue/algorithms.hpp"
#include "midnight_queue/record.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <iterator>
#include <optional>
#include <random>
#include <stdexcept>
#include <vector>

namespace midnight_queue {

enum class Condition { C1Arrival, C2Shuffled, C3PreRanked, C4ReverseRanked, C5NearlyRanked, C6BandOnly };

inline const char* condition_name(Condition condition) {
    switch (condition) {
        case Condition::C1Arrival: return "C1";
        case Condition::C2Shuffled: return "C2";
        case Condition::C3PreRanked: return "C3";
        case Condition::C4ReverseRanked: return "C4";
        case Condition::C5NearlyRanked: return "C5";
        case Condition::C6BandOnly: return "C6";
    }
    throw std::invalid_argument("Unknown input condition");
}

inline std::vector<Record> sample_in_time_order(const std::vector<Record>& full,
                                                 std::size_t n,
                                                 std::mt19937_64& rng) {
    if (n > full.size()) throw std::invalid_argument("Sample size exceeds dataset size");
    std::vector<std::size_t> chosen;
    chosen.reserve(n);
    std::vector<std::size_t> indices(full.size());
    for (std::size_t i = 0; i < indices.size(); ++i) indices[i] = i;
    std::sample(indices.begin(), indices.end(), std::back_inserter(chosen), n, rng);
    std::vector<bool> selected(full.size(), false);
    for (const auto i : chosen) selected[i] = true;
    std::vector<Record> sample;
    sample.reserve(n);
    // Traversing the original C1 sequence restores sampled rows to Time order without sorting.
    for (std::size_t i = 0; i < full.size(); ++i) if (selected[i]) sample.push_back(full[i]);
    return sample;
}

inline std::vector<Record> make_condition(const std::vector<Record>& c1,
                                          Condition condition,
                                          std::mt19937_64& rng) {
    std::vector<Record> result = c1;
    const HigherPriority triage{};
    switch (condition) {
        case Condition::C1Arrival:
        case Condition::C6BandOnly:
            break; // C6 changes the contender's key, not the C1 input order.
        case Condition::C2Shuffled:
            std::shuffle(result.begin(), result.end(), rng);
            break;
        case Condition::C3PreRanked:
            merge_sort(result, triage);
            break;
        case Condition::C4ReverseRanked:
            merge_sort(result, triage);
            std::reverse(result.begin(), result.end());
            break;
        case Condition::C5NearlyRanked: {
            merge_sort(result, triage);
            const std::size_t moved = result.size() / 100;
            if (moved == 0) break;

            std::vector<std::size_t> positions(result.size());
            for (std::size_t i = 0; i < positions.size(); ++i) positions[i] = i;
            std::vector<std::size_t> source_positions, target_positions;
            source_positions.reserve(moved);
            target_positions.reserve(moved);
            std::sample(positions.begin(), positions.end(), std::back_inserter(source_positions), moved, rng);
            bool has_fixed_position = false;
            do {
                target_positions.clear();
                std::sample(positions.begin(), positions.end(), std::back_inserter(target_positions), moved, rng);
                std::shuffle(target_positions.begin(), target_positions.end(), rng);
                has_fixed_position = false;
                for (std::size_t i = 0; i < moved; ++i)
                    if (source_positions[i] == target_positions[i]) { has_fixed_position = true; break; }
            } while (has_fixed_position);

            std::vector<bool> selected(result.size(), false);
            std::vector<Record> selected_records;
            selected_records.reserve(moved);
            for (const auto i : source_positions) {
                selected[i] = true;
                selected_records.push_back(result[i]);
            }

            std::vector<std::optional<Record>> slots(result.size());
            for (std::size_t i = 0; i < moved; ++i) slots[target_positions[i]] = selected_records[i];
            std::size_t source = 0;
            for (std::size_t i = 0; i < result.size(); ++i) {
                if (slots[i].has_value()) continue;
                while (selected[source]) ++source;
                slots[i] = result[source++];
            }
            for (std::size_t i = 0; i < slots.size(); ++i) result[i] = std::move(*slots[i]);
            break;
        }
    }
    return result;
}

inline HigherPriority comparator_for(Condition condition) {
    return condition == Condition::C6BandOnly ? HigherPriority{KeyMode::BandOnly} : HigherPriority{};
}

} // namespace midnight_queue
