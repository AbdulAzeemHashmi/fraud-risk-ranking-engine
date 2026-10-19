#include "midnight_queue/algorithms.hpp"
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
    auto check = [&](auto sort_fn) {
        auto actual = source;
        Metrics metrics;
        sort_fn(actual, higher, &metrics);
        assert(actual.size() == expected.size());
        for (std::size_t i = 0; i < actual.size(); ++i) assert(actual[i].txn_id == expected[i].txn_id);
        assert(metrics.comparisons > 0);
    };
    check(insertion_sort<Record, HigherPriority>);
    check(merge_sort<Record, HigherPriority>);
    check(quick_sort_naive<Record, HigherPriority>);
    check(quick_sort_hardened<Record, HigherPriority>);
    check(heap_sort<Record, HigherPriority>);

    std::vector<Record> empty;
    insertion_sort(empty, higher); merge_sort(empty, higher); quick_sort_naive(empty, higher);
    quick_sort_hardened(empty, higher); heap_sort(empty, higher);
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
}
