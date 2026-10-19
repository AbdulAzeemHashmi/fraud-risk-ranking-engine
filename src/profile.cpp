#include "midnight_queue/algorithms.hpp"
#include "midnight_queue/dataset.hpp"

#include <algorithm>
#include <array>
#include <cstdint>
#include <iostream>
#include <map>
#include <stdexcept>
#include <string>
#include <vector>

int main(int argc, char** argv) {
    if (argc != 2) {
        std::cerr << "Usage: queue_profile <creditcard.csv>\n";
        return 2;
    }
    try {
        auto dataset = midnight_queue::load_dataset(argv[1]);
        std::array<std::uint64_t, 10> band_count{};
        std::array<std::uint64_t, 10> fraud_by_band{};
        std::map<double, std::uint64_t> amounts;
        std::uint64_t frauds = 0;
        bool time_ascending = true;
        for (std::size_t i = 0; i < dataset.records.size(); ++i) {
            const auto& r = dataset.records[i];
            ++band_count.at(static_cast<std::size_t>(r.band));
            ++amounts[r.amount];
            if (r.fraud == 1) { ++frauds; ++fraud_by_band.at(static_cast<std::size_t>(r.band)); }
            if (i && dataset.records[i].time < dataset.records[i - 1].time) time_ascending = false;
        }
        std::vector<std::uint64_t> frequencies;
        for (const auto& item : amounts) frequencies.push_back(item.second);
        std::sort(frequencies.begin(), frequencies.end(), std::greater<std::uint64_t>());
        std::uint64_t top_ten = 0;
        for (std::size_t i = 0; i < std::min<std::size_t>(10, frequencies.size()); ++i) top_ten += frequencies[i];

        std::cout << "transactions," << dataset.records.size() << "\nfrauds," << frauds
                  << "\nroll_number,2013\nK,46\namount_distinct," << amounts.size()
                  << "\ntop_10_amount_share," << static_cast<double>(top_ten) / dataset.records.size()
                  << "\nC1_time_ascending," << (time_ascending ? "true" : "false") << "\n\nband,count,fraud_count\n";
        for (std::size_t band = 0; band < band_count.size(); ++band)
            std::cout << band << ',' << band_count[band] << ',' << fraud_by_band[band] << '\n';

        auto ranked = dataset.records;
        midnight_queue::Metrics metrics;
        midnight_queue::merge_sort(ranked, midnight_queue::HigherPriority{}, &metrics);
        bool ranked_correct = true;
        for (std::size_t i = 1; i < ranked.size(); ++i)
            if (midnight_queue::HigherPriority{}(ranked[i], ranked[i - 1])) { ranked_correct = false; break; }
        std::cout << "\nC3_is_triage_order," << (ranked_correct ? "true" : "false") << '\n';
        return ranked_correct ? 0 : 1;
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << '\n';
        return 1;
    }
}
