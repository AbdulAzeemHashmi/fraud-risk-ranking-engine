#include "midnight_queue/algorithms.hpp"
#include "midnight_queue/experiment_data.hpp"
#include "midnight_queue/record.hpp"

#include <algorithm>
#include <cassert>
#include <iostream>
#include <vector>

using namespace midnight_queue;

int main() {
    const HigherPriority higher{};
    std::vector<Record> source{
        {5, 3, 10, 1, 0}, {2, 1, 20, 2, 1}, {4, 2, 20, 2, 0},
        {1, 4, 20, 2, 0}, {3, 2, 20, 2, 1}, {6, 0, 5, 0, 0}
    };
    auto expected = source;
    std::sort(expected.begin(), expected.end(), higher);
    auto check = [&](const std::vector<Record>& input, auto sort_fn, HigherPriority comparator = HigherPriority{}) {
        auto expected = input;
        std::sort(expected.begin(), expected.end(), comparator);
        auto actual = input;
        Metrics metrics;
        sort_fn(actual, comparator, &metrics);
        assert(actual.size() == expected.size());
        for (std::size_t i = 0; i < actual.size(); ++i) assert(actual[i].txn_id == expected[i].txn_id);
        if (actual.size() > 1) assert(metrics.comparisons > 0);
    };
    std::vector<Record> empty;
    std::vector<Record> singleton{{77, 1, 9, 3, 0}};
    const auto full_sorts = {insertion_sort<Record, HigherPriority>, merge_sort<Record, HigherPriority>,
                             quick_sort_naive<Record, HigherPriority>, quick_sort_hardened<Record, HigherPriority>,
                             heap_sort<Record, HigherPriority>};
    for (auto sort_fn : full_sorts) {
        check(source, sort_fn);
        check(empty, sort_fn);
        check(singleton, sort_fn);
    }
    assert(top_k_heap(source, 0, higher).empty());
    assert(top_k_buffer(source, 0, higher).empty());

    for (std::size_t k : {1u, 3u, 6u, 20u}) {
        auto h = top_k_heap(source, k, higher);
        auto b = top_k_buffer(source, k, higher);
        const std::size_t expected_size = std::min(k, expected.size());
        assert(h.size() == expected_size && b.size() == expected_size);
        for (std::size_t i = 0; i < expected_size; ++i) {
            if (h[i].txn_id != expected[i].txn_id) {
                std::cerr << "heap top-K mismatch at k=" << k << " position=" << i
                          << " actual txn=" << h[i].txn_id << " expected txn=" << expected[i].txn_id << '\n';
            }
            assert(h[i].txn_id == expected[i].txn_id);
            assert(b[i].txn_id == expected[i].txn_id);
        }
    }

    std::vector<Record> tied(40, Record{7, 1, 2, 4, 0});
    auto duplicate_expected = tied;
    std::sort(duplicate_expected.begin(), duplicate_expected.end(), higher);
    quick_sort_hardened(tied, higher);
    for (std::size_t i = 0; i < tied.size(); ++i) assert(tied[i].txn_id == duplicate_expected[i].txn_id);

    std::vector<Record> duplicate_heavy;
    for (std::uint64_t i = 0; i < 100; ++i)
        duplicate_heavy.push_back(Record{100 - i, 5, 8.5, 6, static_cast<int>(i % 9 == 0)});
    for (auto sort_fn : full_sorts) check(duplicate_heavy, sort_fn);
    auto band_only = duplicate_heavy;
    for (std::size_t i = 0; i < band_only.size(); ++i) band_only[i].band = static_cast<int>(i % 3);
    for (auto sort_fn : full_sorts) {
        auto actual = band_only;
        sort_fn(actual, comparator_for(Condition::C6BandOnly), nullptr);
        for (std::size_t i = 1; i < actual.size(); ++i)
            assert(actual[i - 1].band >= actual[i].band);
    }

    std::vector<Record> arrivals;
    for (std::uint64_t i = 0; i < 100; ++i)
        arrivals.push_back(Record{i + 1, static_cast<double>(i), static_cast<double>((i * 17) % 13),
                                  static_cast<int>((i * 7) % 5), static_cast<int>(i % 11 == 0)});
    std::mt19937_64 rng1(2013), rng2(2013);
    auto sample1 = sample_in_time_order(arrivals, 40, rng1);
    auto sample2 = sample_in_time_order(arrivals, 40, rng2);
    for (std::size_t i = 0; i < sample1.size(); ++i) {
        assert(sample1[i].txn_id == sample2[i].txn_id);
        if (i) assert(sample1[i - 1].time <= sample1[i].time);
    }

    std::mt19937_64 condition_rng(2013);
    auto c1 = make_condition(arrivals, Condition::C1Arrival, condition_rng);
    auto c2 = make_condition(arrivals, Condition::C2Shuffled, condition_rng);
    auto c3 = make_condition(arrivals, Condition::C3PreRanked, condition_rng);
    auto c4 = make_condition(arrivals, Condition::C4ReverseRanked, condition_rng);
    auto c5 = make_condition(arrivals, Condition::C5NearlyRanked, condition_rng);
    auto c6 = make_condition(arrivals, Condition::C6BandOnly, condition_rng);
    for (std::size_t i = 0; i < arrivals.size(); ++i) {
        assert(c1[i].txn_id == arrivals[i].txn_id);
        assert(c6[i].txn_id == arrivals[i].txn_id);
        assert(c4[i].txn_id == c3[arrivals.size() - 1 - i].txn_id);
        if (i) assert(!higher(c3[i], c3[i - 1]));
    }
    auto ids = [](const std::vector<Record>& records) {
        std::vector<std::uint64_t> result;
        for (const auto& record : records) result.push_back(record.txn_id);
        std::sort(result.begin(), result.end());
        return result;
    };
    assert(ids(c2) == ids(arrivals));
    assert(ids(c5) == ids(arrivals));
    bool c5_changed_order = false;
    for (std::size_t i = 0; i < c5.size(); ++i) c5_changed_order |= c5[i].txn_id != c3[i].txn_id;
    assert(c5_changed_order);
    assert(comparator_for(Condition::C6BandOnly).mode == KeyMode::BandOnly);
}
