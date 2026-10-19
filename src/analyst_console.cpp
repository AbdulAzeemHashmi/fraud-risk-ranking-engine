#include "midnight_queue/algorithms.hpp"
#include "midnight_queue/dataset.hpp"

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <iomanip>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

using namespace midnight_queue;
using Clock = std::chrono::steady_clock;

namespace {

bool is_sorted_by_priority(const std::vector<Record>& records) {
    const HigherPriority higher{};
    for (std::size_t i = 1; i < records.size(); ++i)
        if (higher(records[i], records[i - 1])) return false;
    return true;
}

} // namespace

int main(int argc, char** argv) {
    if (argc != 4) {
        std::cerr << "Usage: analyst_console <creditcard.csv> <merge_sort|size_k_min_heap|bounded_insertion_buffer> <K>\n";
        return 2;
    }

    try {
        std::size_t consumed = 0;
        const std::string method = argv[2];
        const auto requested_k = std::stoull(argv[3], &consumed);
        if (consumed != std::string(argv[3]).size() || requested_k == 0)
            throw std::invalid_argument("K must be a positive whole number");

        const auto dataset = load_dataset(argv[1]);
        const auto k = std::min<std::size_t>(static_cast<std::size_t>(requested_k), dataset.records.size());
        Metrics metrics;
        std::vector<Record> queue;
        Clock::time_point start;
        if (method == "merge_sort") {
            queue = dataset.records; // Keep the common input copy outside the timer.
            start = Clock::now();
            merge_sort(queue, HigherPriority{}, &metrics);
            if (queue.size() > k) queue.resize(k);
        } else if (method == "size_k_min_heap") {
            start = Clock::now();
            queue = top_k_heap(dataset.records, k, HigherPriority{}, &metrics);
        } else if (method == "bounded_insertion_buffer") {
            start = Clock::now();
            queue = top_k_buffer(dataset.records, k, HigherPriority{}, &metrics);
        } else {
            throw std::invalid_argument("Choose Merge Sort, Size-K Min-Heap, or Bounded Insertion Buffer");
        }
        const double seconds = std::chrono::duration<double>(Clock::now() - start).count();
        if (!is_sorted_by_priority(queue)) throw std::runtime_error("The resulting queue is not ordered");

        std::cout << std::setprecision(17)
                  << "{\"algorithm\":\"" << method << "\",\"rows\":" << dataset.records.size()
                  << ",\"requested_k\":" << requested_k << ",\"queue_size\":" << queue.size()
                  << ",\"seconds\":" << seconds << ",\"comparisons\":" << metrics.comparisons
                  << ",\"moves\":" << metrics.moves << ",\"transactions\":[";
        for (std::size_t i = 0; i < queue.size(); ++i) {
            if (i) std::cout << ',';
            const auto& record = queue[i];
            std::cout << "{\"id\":" << record.txn_id << ",\"risk_band\":" << record.band
                      << ",\"amount\":" << record.amount << ",\"time\":" << record.time << '}';
        }
        std::cout << "]}\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
