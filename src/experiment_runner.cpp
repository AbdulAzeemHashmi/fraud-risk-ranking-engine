#include "midnight_queue/algorithms.hpp"
#include "midnight_queue/dataset.hpp"
#include "midnight_queue/experiment_data.hpp"
#include "midnight_queue/svg_plot.hpp"

#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <functional>
#include <iomanip>
#include <iostream>
#include <limits>
#include <map>
#include <numeric>
#include <optional>
#include <set>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

using namespace midnight_queue;
using Clock = std::chrono::steady_clock;
constexpr int kRoll = 2013;
constexpr int kPersonalK = 230;
constexpr int kTrials = 5;
constexpr int kTimeoutSeconds = 300;

struct ResultRow {
    std::string problem, condition, algorithm, status;
    std::size_t n{0}, k{0}, window_minutes{0}, window_index{0}, trial{0};
    double seconds{0}, cumulative_seconds{0};
    std::uint64_t comparisons{0}, moves{0}, extra_memory_bytes{0};
    bool stable{false};
    std::uint64_t captured_frauds{0}, peak_memory_bytes{0};
};

std::string csv_quote(const std::string& value) {
    if (value.find_first_of(",\"\r\n") == std::string::npos) return value;
    std::string out = "\"";
    for (char c : value) { out += c; if (c == '"') out += '"'; }
    return out + '"';
}

void write_csv_row(std::ostream& out, const ResultRow& r) {
    out << r.problem << ',' << csv_quote(r.condition) << ',' << r.n << ',' << r.k << ','
        << r.window_minutes << ',' << r.window_index << ',' << csv_quote(r.algorithm) << ',' << r.trial << ','
        << std::setprecision(10) << r.seconds << ',' << r.cumulative_seconds << ','
        << r.comparisons << ',' << r.moves << ',' << r.extra_memory_bytes << ',' << (r.stable ? "true" : "false") << ','
        << r.status << ',' << r.captured_frauds << ',' << r.peak_memory_bytes << '\n';
}

struct SortMethod {
    std::string name;
    bool stable;
    std::function<void(std::vector<Record>&, HigherPriority, Metrics*)> sort;
};

std::vector<SortMethod> full_sort_methods() {
    return {
        {"Insertion Sort", true, [](auto& a, auto c, auto* m) { insertion_sort(a, c, m); }},
        {"Merge Sort", true, [](auto& a, auto c, auto* m) { merge_sort(a, c, m); }},
        {"Naive Quick Sort", false, [](auto& a, auto c, auto* m) { quick_sort_naive(a, c, m); }},
        {"Hardened Quick Sort", false, [](auto& a, auto c, auto* m) { quick_sort_hardened(a, c, m); }},
        {"Heap Sort", false, [](auto& a, auto c, auto* m) { heap_sort(a, c, m); }}
    };
}

std::uint64_t estimated_sort_memory(const std::string& method, std::size_t n) {
    if (method == "Merge Sort") return static_cast<std::uint64_t>(n) * sizeof(Record);
    if (method == "Naive Quick Sort" || method == "Hardened Quick Sort") {
        const auto depth = n < 2 ? 1u : static_cast<unsigned>(std::ceil(std::log2(static_cast<double>(n))));
        return static_cast<std::uint64_t>(depth) * 4 * sizeof(std::size_t); // Estimated bounded call stack.
    }
    return 0;
}

std::vector<std::size_t> get_sizes(std::size_t total, bool smoke) {
    if (smoke) return {std::min<std::size_t>(1000, total)};
    std::vector<std::size_t> sizes{1000, 10000, 100000, total};
    sizes.erase(std::remove_if(sizes.begin(), sizes.end(), [total](auto n) { return n > total; }), sizes.end());
    sizes.erase(std::unique(sizes.begin(), sizes.end()), sizes.end());
    return sizes;
}

struct Prediction { std::string winner, loser, reason; };

Prediction predict(Condition c) {
    switch (c) {
        case Condition::C1Arrival:
            return {"Merge Sort", "Insertion Sort", "The arrival order has no known relation to the complete triage key. Merge Sort keeps n log n work."};
        case Condition::C2Shuffled:
            return {"Hardened Quick Sort", "Insertion Sort", "The shuffle removes existing order; three way partitioning protects Quick Sort and insertion can shift many records."};
        case Condition::C3PreRanked:
            return {"Insertion Sort", "Naive Quick Sort", "A ranked input should need few insertion shifts, while the last pivot on sorted data creates unbalanced partitions."};
        case Condition::C4ReverseRanked:
            return {"Heap Sort", "Naive Quick Sort", "Heap Sort keeps n log n work; reverse order is a worst case pattern for the last pivot."};
        case Condition::C5NearlyRanked:
            return {"Insertion Sort", "Naive Quick Sort", "Only one percent of records were moved, so insertion shifts should stay limited; the last pivot remains sensitive to long ordered runs."};
        case Condition::C6BandOnly:
            return {"Hardened Quick Sort", "Naive Quick Sort", "Band-only keys have many ties; three way partition handles equal keys while Lomuto can repeatedly split off one item."};
    }
    throw std::invalid_argument("Unknown condition");
}

std::string condition_id(Condition c) { return condition_name(c); }

struct CMedian {
    Condition condition;
    std::size_t n;
    std::string algorithm;
    double seconds;
    std::uint64_t comparisons, moves, memory;
    bool stable, dnf;
};

double median(std::vector<double> values) {
    if (values.empty()) return std::numeric_limits<double>::quiet_NaN();
    std::sort(values.begin(), values.end());
    const auto middle = values.size() / 2;
    return values.size() % 2 ? values[middle] : (values[middle - 1] + values[middle]) / 2.0;
}

std::uint64_t median(std::vector<std::uint64_t> values) {
    if (values.empty()) return 0;
    std::sort(values.begin(), values.end());
    return values[values.size() / 2];
}

bool is_sorted_by(const std::vector<Record>& records, HigherPriority comp) {
    for (std::size_t i = 1; i < records.size(); ++i) if (comp(records[i], records[i - 1])) return false;
    return true;
}

void log_prediction(std::ostream& out, const std::string& round, const std::string& phase,
                    std::size_t n, Condition c, const Prediction& p,
                    const std::string& actual_winner = "", const std::string& actual_loser = "",
                    const std::string& winner_correct = "", const std::string& loser_correct = "") {
    out << csv_quote(round) << ',' << phase << ',' << n << ',' << condition_id(c) << ','
        << csv_quote(p.winner) << ',' << csv_quote(p.loser) << ',' << csv_quote(p.reason) << ','
        << csv_quote(actual_winner) << ',' << csv_quote(actual_loser) << ',' << winner_correct << ',' << loser_correct << '\n';
    out.flush();
}

std::vector<CMedian> run_problem_c(const Dataset& dataset, bool smoke,
                                   std::ostream& results, std::ostream& predictions) {
    const std::array<Condition, 6> conditions{Condition::C1Arrival, Condition::C2Shuffled,
        Condition::C3PreRanked, Condition::C4ReverseRanked, Condition::C5NearlyRanked, Condition::C6BandOnly};
    const auto methods = full_sort_methods();
    auto sizes = get_sizes(dataset.records.size(), smoke);
    std::vector<CMedian> summaries;
    std::mt19937_64 rng(kRoll);
    std::size_t round_number = 0;
    for (const auto n : sizes) {
        const auto c1 = sample_in_time_order(dataset.records, n, rng);
        for (const auto condition : conditions) {
            ++round_number;
            const auto prediction = predict(condition);
            const auto round = "C" + std::to_string(round_number);
            log_prediction(predictions, round, "prediction", n, condition, prediction);
            const auto input = make_condition(c1, condition, rng);
            const auto comparator = comparator_for(condition);
            struct One { std::string name; double seconds; std::uint64_t comparisons, moves; bool dnf, stable; };
            std::vector<One> observed;
            for (const auto& method : methods) {
                std::vector<double> times;
                std::vector<std::uint64_t> comparisons, moves;
                bool dnf = false;
                for (int trial = 1; trial <= kTrials; ++trial) {
                    auto working = input; // The fresh copy is outside the timed section.
                    Metrics metrics;
                    metrics.set_time_limit(std::chrono::seconds(kTimeoutSeconds));
                    const auto start = Clock::now();
                    try {
                        method.sort(working, comparator, &metrics);
                        const auto finish = Clock::now();
                        const double seconds = std::chrono::duration<double>(finish - start).count();
                        if (!is_sorted_by(working, comparator)) throw std::runtime_error("sort result is not ordered");
                        times.push_back(seconds); comparisons.push_back(metrics.comparisons); moves.push_back(metrics.moves);
                        ResultRow row{"C", condition_id(condition), method.name, "ok", n, 0, 0, 0,
                            static_cast<std::size_t>(trial), seconds, 0, metrics.comparisons, metrics.moves,
                            estimated_sort_memory(method.name, n), method.stable};
                        write_csv_row(results, row);
                    } catch (const RunTimeout&) {
                        const double seconds = std::chrono::duration<double>(Clock::now() - start).count();
                        ResultRow row{"C", condition_id(condition), method.name, "DNF", n, 0, 0, 0,
                            static_cast<std::size_t>(trial), seconds, 0, metrics.comparisons, metrics.moves,
                            estimated_sort_memory(method.name, n), method.stable};
                        write_csv_row(results, row);
                        dnf = true;
                        break;
                    }
                }
                const double time = median(times);
                const auto cmp = median(comparisons), mv = median(moves);
                summaries.push_back({condition, n, method.name, time, cmp, mv,
                    estimated_sort_memory(method.name, n), method.stable, dnf});
                ResultRow summary{"C", condition_id(condition), method.name, dnf ? "DNF" : "median",
                    n, 0, 0, 0, 0, time, 0, cmp, mv, estimated_sort_memory(method.name, n), method.stable};
                write_csv_row(results, summary);
                observed.push_back({method.name, time, cmp, mv, dnf, method.stable});
            }
            std::string winner = "not measured", loser = "not measured";
            double best = std::numeric_limits<double>::infinity(), worst = -1;
            bool loser_dnf = false;
            for (const auto& item : observed) {
                if (!item.dnf && item.seconds < best) { best = item.seconds; winner = item.name; }
                if (item.dnf) { loser = item.name; loser_dnf = true; }
                else if (!loser_dnf && item.seconds > worst) { worst = item.seconds; loser = item.name; }
            }
            const auto correct = [](const std::string& actual, const std::string& predicted) {
                return actual == predicted ? "true" : "false";
            };
            log_prediction(predictions, round, "result", n, condition, prediction, winner, loser,
                           correct(winner, prediction.winner), correct(loser, prediction.loser));
            std::cout << "Problem C " << round << " n=" << n << " " << condition_id(condition)
                      << " winner=" << winner << " loser=" << loser << '\n';
        }
    }
    return summaries;
}

void make_problem_c_plots(const std::vector<CMedian>& rows, const std::filesystem::path& outdir) {
    const std::vector<std::string> names{"Insertion Sort", "Merge Sort", "Naive Quick Sort", "Hardened Quick Sort", "Heap Sort"};
    const std::vector<std::string> colors{"#2455a4", "#dc7927", "#2a8f55", "#a23a8b", "#555b66"};
    std::vector<PlotSeries> time, comparisons, moves, normalized_nlogn, normalized_n2;
    for (std::size_t mi = 0; mi < names.size(); ++mi) {
        PlotSeries t{names[mi], colors[mi], {}}, c{names[mi], colors[mi], {}}, m{names[mi], colors[mi], {}}, nlogn{names[mi], colors[mi], {}}, n2{names[mi], colors[mi], {}};
        for (const auto& r : rows) if (r.condition == Condition::C2Shuffled && r.algorithm == names[mi] && !r.dnf && std::isfinite(r.seconds)) {
            const double n = static_cast<double>(r.n);
            t.points.emplace_back(n, r.seconds);
            if (r.comparisons) c.points.emplace_back(n, static_cast<double>(r.comparisons));
            if (r.moves) m.points.emplace_back(n, static_cast<double>(r.moves));
            nlogn.points.emplace_back(n, r.seconds / (n * std::log2(n)));
            n2.points.emplace_back(n, r.seconds / (n * n));
        }
        time.push_back(std::move(t)); comparisons.push_back(std::move(c)); moves.push_back(std::move(m));
        normalized_nlogn.push_back(std::move(nlogn)); normalized_n2.push_back(std::move(n2));
    }
    write_line_plot((outdir / "C2_time_vs_n.svg").string(), "Execution Time vs Input Size on C2",
        "Input size, n (transactions)", "Median execution time (seconds)",
        "Figure C1. Log scale on both axes. DNF results are omitted.", time, true, true);
    write_line_plot((outdir / "C2_comparisons_vs_n.svg").string(), "Key Comparisons vs Input Size on C2",
        "Input size, n (transactions)", "Key comparisons (count)",
        "Figure C2. Median key comparisons. DNF results are omitted.", comparisons, true, true);
    write_line_plot((outdir / "C2_moves_vs_n.svg").string(), "Element Moves vs Input Size on C2",
        "Input size, n (transactions)", "Element moves (count)",
        "Figure C3. Median element moves. DNF results are omitted.", moves, true, true);
    write_line_plot((outdir / "C2_time_over_nlogn.svg").string(), "Time Divided by n log2 n on C2",
        "Input size, n (transactions)", "Time / (n log2 n), seconds per operation unit",
        "Figure C4. A flatter curve is consistent with n log n growth. DNF results are omitted.", normalized_nlogn, true, false);
    write_line_plot((outdir / "C2_time_over_n_squared.svg").string(), "Time Divided by n Squared on C2",
        "Input size, n (transactions)", "Time / n squared, seconds per operation unit",
        "Figure C5. A flatter curve is consistent with n squared growth. DNF results are omitted.", normalized_n2, true, false);

    const std::array<Condition, 6> conditions{Condition::C1Arrival, Condition::C2Shuffled,
        Condition::C3PreRanked, Condition::C4ReverseRanked, Condition::C5NearlyRanked, Condition::C6BandOnly};
    std::vector<std::string> categories{"C1", "C2", "C3", "C4", "C5", "C6"};
    std::vector<std::vector<double>> bars(names.size(), std::vector<double>(conditions.size(), 0));
    for (std::size_t si = 0; si < names.size(); ++si) for (std::size_t ci = 0; ci < conditions.size(); ++ci) {
        for (const auto& r : rows) if (r.condition == conditions[ci] && r.n == 10000 && r.algorithm == names[si] && !r.dnf) bars[si][ci] = r.seconds;
    }
    write_grouped_bar_plot((outdir / "time_at_10000_by_condition.svg").string(),
        "Execution Time at n = 10,000 by Input Condition", "Input condition", "Median execution time (seconds)",
        "Figure C6. A zero height bar means that the contender was DNF or no 10,000 row was run.",
        categories, names, colors, bars);
}

struct TopMethod {
    std::string name;
    bool stable;
    std::function<std::vector<Record>(const std::vector<Record>&, std::size_t, Metrics*)> run;
};

struct DSummary {
    std::string name, status;
    double seconds;
    std::uint64_t comparisons, moves, memory;
};

std::vector<TopMethod> top_methods() {
    return {
        {"Full Sort (Merge Sort)", true, [](const auto& input, std::size_t k, Metrics* m) {
            auto result = input; merge_sort(result, HigherPriority{}, m); if (result.size() > k) result.resize(k); return result;
        }},
        {"Size-K Min-Heap", false, [](const auto& input, std::size_t k, Metrics* m) {
            return top_k_heap(input, k, HigherPriority{}, m);
        }},
        {"Bounded Insertion Buffer", true, [](const auto& input, std::size_t k, Metrics* m) {
            return top_k_buffer(input, k, HigherPriority{}, m);
        }}
    };
}

std::uint64_t estimated_top_memory(const std::string& method, std::size_t n, std::size_t k) {
    if (method == "Full Sort (Merge Sort)") return static_cast<std::uint64_t>(n) * sizeof(Record);
    return static_cast<std::uint64_t>(k) * sizeof(Record);
}

std::uint64_t captured_frauds(const std::vector<Record>& records) {
    return static_cast<std::uint64_t>(std::count_if(records.begin(), records.end(), [](const Record& r) { return r.fraud == 1; }));
}

std::vector<DSummary> run_d_trials(const std::vector<Record>& input, std::size_t n, std::size_t k,
                                   const std::string& condition, std::ostream& results, int trials = kTrials) {
    const auto methods = top_methods();
    auto reference = input;
    merge_sort(reference, HigherPriority{});
    if (reference.size() > k) reference.resize(k);
    std::vector<DSummary> summaries;
    for (const auto& method : methods) {
        std::vector<double> times;
        std::vector<std::uint64_t> comps, moves;
        bool dnf = false;
        std::uint64_t captured = 0;
        for (int trial = 1; trial <= trials; ++trial) {
            Metrics metrics;
            std::vector<Record> output;
            Clock::time_point start;
            try {
                if (method.name == "Full Sort (Merge Sort)") {
                    output = input; // Match the fresh-copy rule and keep input preparation out of timing.
                    metrics.set_time_limit(std::chrono::seconds(kTimeoutSeconds));
                    start = Clock::now();
                    merge_sort(output, HigherPriority{}, &metrics);
                    if (output.size() > k) output.resize(k);
                } else {
                    metrics.set_time_limit(std::chrono::seconds(kTimeoutSeconds));
                    start = Clock::now();
                    output = method.run(input, k, &metrics);
                }
                const auto end = Clock::now();
                const double seconds = std::chrono::duration<double>(end - start).count();
                if (output.size() != reference.size()) throw std::runtime_error("Top K result has incorrect size");
                for (std::size_t i = 0; i < output.size(); ++i)
                    if (output[i].txn_id != reference[i].txn_id) throw std::runtime_error("Top K result differs from full-sort reference");
                captured = captured_frauds(output);
                times.push_back(seconds); comps.push_back(metrics.comparisons); moves.push_back(metrics.moves);
                write_csv_row(results, ResultRow{"D", condition, method.name, "ok", n, k, 0, 0,
                    static_cast<std::size_t>(trial), seconds, 0, metrics.comparisons, metrics.moves,
                    estimated_top_memory(method.name, n, k), method.stable, captured});
            } catch (const RunTimeout&) {
                const double seconds = std::chrono::duration<double>(Clock::now() - start).count();
                write_csv_row(results, ResultRow{"D", condition, method.name, "DNF", n, k, 0, 0,
                    static_cast<std::size_t>(trial), seconds, 0, metrics.comparisons, metrics.moves,
                    estimated_top_memory(method.name, n, k), method.stable, captured});
                dnf = true;
                break;
            }
        }
        const std::string status = dnf ? "DNF" : "median";
        const double med_time = median(times);
        const auto med_comparisons = median(comps), med_moves = median(moves);
        const auto memory = estimated_top_memory(method.name, n, k);
        write_csv_row(results, ResultRow{"D", condition, method.name, status, n, k, 0, 0, 0,
            med_time, 0, med_comparisons, med_moves, memory, method.stable, captured});
        summaries.push_back({method.name, status, med_time, med_comparisons, med_moves, memory});
        std::cout << "Problem D " << condition << " n=" << n << " K=" << k << " " << method.name
                  << " median_seconds=" << median(times) << " status=" << (dnf ? "DNF" : "ok") << '\n';
    }
    return summaries;
}

std::size_t find_half_capture_k(const std::vector<Record>& ranked, std::size_t fraud_total,
                                std::ofstream& curve) {
    curve << "K,frauds_captured,fraction_of_all_frauds\n";
    std::size_t captured = 0;
    std::size_t answer = ranked.size();
    for (std::size_t i = 0; i < ranked.size(); ++i) {
        if (ranked[i].fraud == 1) ++captured;
        const double fraction = fraud_total ? static_cast<double>(captured) / fraud_total : 0.0;
        curve << i + 1 << ',' << captured << ',' << std::setprecision(10) << fraction << '\n';
        if (answer == ranked.size() && captured * 2 >= fraud_total) answer = i + 1;
    }
    return answer;
}

void run_problem_d(const Dataset& dataset, bool smoke, const std::filesystem::path& outdir,
                   std::ostream& results) {
    std::vector<Record> ranked = dataset.records;
    merge_sort(ranked, HigherPriority{});
    const auto all_frauds = captured_frauds(dataset.records);
    std::ofstream curve(outdir / "fraud_capture_curve.csv");
    if (!curve) throw std::runtime_error("Cannot create fraud_capture_curve.csv");
    const std::size_t half_k = find_half_capture_k(ranked, all_frauds, curve);
    curve.close();
    PlotSeries capture{"Triage ranking", "#2455a4", {}};
    {
        std::ifstream in(outdir / "fraud_capture_curve.csv");
        std::string line;
        std::getline(in, line);
        std::size_t k = 0, count = 0;
        double fraction = 0;
        char comma;
        while (in >> k >> comma >> count >> comma >> fraction)
            if (k % 250 == 0 || k == half_k || k == 1 || k == dataset.records.size())
                capture.points.emplace_back(static_cast<double>(k), fraction);
    }
    write_line_plot((outdir / "fraud_capture_vs_k.svg").string(), "Fraud Capture as Queue Size Grows",
        "Queue size, K (transactions)", "Frauds captured (fraction of all frauds)",
        "Figure D1. Fraud labels are used only after ranking for evaluation. K = " + std::to_string(half_k) + " is the first point at or above one half.",
        {capture}, true, false);
    std::ofstream half(outdir / "half_capture_cost.csv");
    half << "condition,n,K,method,median_seconds,comparisons,moves,extra_memory_bytes,status\n";

    std::vector<std::size_t> sizes = smoke ? std::vector<std::size_t>{std::min<std::size_t>(1000, dataset.records.size())}
        : std::vector<std::size_t>{100000, dataset.records.size()};
    sizes.erase(std::remove_if(sizes.begin(), sizes.end(), [&](auto n) { return n > dataset.records.size(); }), sizes.end());
    sizes.erase(std::unique(sizes.begin(), sizes.end()), sizes.end());
    std::mt19937_64 rng(kRoll);
    for (auto n : sizes) {
        auto c1 = sample_in_time_order(dataset.records, n, rng);
        auto c2 = make_condition(c1, Condition::C2Shuffled, rng);
        std::vector<std::size_t> ks{10, 50, static_cast<std::size_t>(kPersonalK), 500, 5000, n / 10};
        std::sort(ks.begin(), ks.end());
        ks.erase(std::unique(ks.begin(), ks.end()), ks.end());
        const std::vector<std::pair<std::string, const std::vector<Record>*>> condition_inputs{{"C1", &c1}, {"C2", &c2}};
        for (const auto& condition_input : condition_inputs) {
            for (auto k : ks) run_d_trials(*condition_input.second, n, std::min(k, n), condition_input.first, results);
        }
    }
    const auto c2_full = make_condition(dataset.records, Condition::C2Shuffled, rng);
    const std::vector<std::pair<std::string, const std::vector<Record>*>> half_conditions{{"C1", &dataset.records}, {"C2", &c2_full}};
    if (!smoke) {
        for (const auto& condition_input : half_conditions) {
            const auto summaries = run_d_trials(*condition_input.second, dataset.records.size(), half_k, condition_input.first, results);
            for (const auto& item : summaries) {
                half << condition_input.first << ',' << dataset.records.size() << ',' << half_k << ','
                     << csv_quote(item.name) << ',' << item.seconds << ',' << item.comparisons << ','
                     << item.moves << ',' << item.memory << ',' << item.status << '\n';
            }
        }
    } else half << "smoke mode skips full data half capture cost runs\n";
    std::cout << "Problem D smallest K capturing at least half of the " << all_frauds << " frauds: " << half_k << '\n';
}

class PersistentTopK {
public:
    PersistentTopK(std::size_t limit, Metrics* metrics) : limit_(limit), metrics_(metrics) { heap_.reserve(limit); }

    void add(const Record& item) {
        if (limit_ == 0) return;
        if (heap_.size() < limit_) {
            heap_.push_back(item);
            if (metrics_) ++metrics_->moves;
            sift_up(heap_.size() - 1);
        } else if (counted_less(item, heap_.front(), higher_, metrics_)) {
            heap_.front() = item;
            if (metrics_) ++metrics_->moves;
            sift_down(heap_, 0, heap_.size(), higher_, metrics_);
        }
    }

    std::vector<Record> publish() const {
        auto result = heap_;
        heap_sort(result, higher_, metrics_);
        return result;
    }

    std::size_t size() const { return heap_.size(); }
    std::uint64_t memory_bytes() const { return static_cast<std::uint64_t>(heap_.capacity()) * sizeof(Record); }

private:
    void sift_up(std::size_t i) {
        while (i > 0) {
            const std::size_t parent = (i - 1) / 2;
            if (!counted_less(heap_[parent], heap_[i], higher_, metrics_)) break;
            std::swap(heap_[parent], heap_[i]);
            if (metrics_) metrics_->moves += 3;
            i = parent;
        }
    }

    std::size_t limit_;
    Metrics* metrics_;
    HigherPriority higher_{};
    std::vector<Record> heap_;
};

std::vector<std::vector<Record>> split_into_windows(const std::vector<Record>& records, std::size_t window_seconds) {
    if (records.empty()) return {};
    double max_time = 0;
    for (const auto& r : records) max_time = std::max(max_time, r.time);
    const std::size_t count = static_cast<std::size_t>(std::floor(max_time / window_seconds)) + 1;
    std::vector<std::vector<Record>> windows(count);
    for (const auto& r : records) {
        const auto index = std::min<std::size_t>(static_cast<std::size_t>(r.time / window_seconds), count - 1);
        windows[index].push_back(r);
    }
    return windows;
}

std::vector<Record> merge_top_k_lists(const std::vector<Record>& old_top,
                                      const std::vector<Record>& new_sorted,
                                      std::size_t k, Metrics* metrics) {
    std::vector<Record> result;
    result.reserve(std::min(k, old_top.size() + new_sorted.size()));
    std::size_t i = 0, j = 0;
    while (result.size() < k && (i < old_top.size() || j < new_sorted.size())) {
        if (j == new_sorted.size() || (i < old_top.size() && counted_less(new_sorted[j], old_top[i], HigherPriority{}, metrics))) {
            result.push_back(old_top[i++]);
        } else {
            result.push_back(new_sorted[j++]);
        }
        if (metrics) ++metrics->moves;
    }
    return result;
}

struct EPoint { std::size_t window_minutes, window_index, trial; std::string strategy; double cumulative; std::uint64_t peak; };

void run_problem_e(const Dataset& dataset, bool smoke, const std::filesystem::path& outdir,
                   std::ostream& results) {
    const std::size_t k = kPersonalK;
    const std::array<std::pair<std::size_t, std::size_t>, 2> window_sizes{{{60, 3600}, {10, 600}}};
    const std::array<std::string, 3> names{{"Re-rank All with Merge Sort", "Maintain Size-K Min-Heap", "Merge Sorted Window into Top-K"}};
    const std::array<std::string, 3> colors{{"#2455a4", "#dc7927", "#2a8f55"}};
    std::vector<EPoint> points;
    std::ofstream memory_csv(outdir / "live_feed_peak_memory.csv");
    memory_csv << "window_minutes,strategy,peak_extra_memory_bytes,status\n";

    auto records = dataset.records;
    if (smoke && records.size() > 10000) records.resize(10000);
    for (const auto [minutes, seconds_per_window] : window_sizes) {
        const auto windows = split_into_windows(records, seconds_per_window);
        std::array<std::uint64_t, 3> peak_memory{};
        for (std::size_t strategy = 0; strategy < names.size(); ++strategy) {
            std::array<std::vector<double>, kTrials> cumulative_by_trial;
            std::array<std::vector<std::uint64_t>, kTrials> peak_by_trial;
            for (int trial = 1; trial <= kTrials; ++trial) {
                Metrics metrics;
                metrics.set_time_limit(std::chrono::seconds(kTimeoutSeconds));
                std::vector<Record> seen, current_top;
                PersistentTopK heap(k, &metrics);
                double cumulative = 0;
                std::uint64_t peak = 0;
                std::size_t transactions_seen = 0;
                for (std::size_t wi = 0; wi < windows.size(); ++wi) {
                    const auto begin = Clock::now();
                    std::size_t seen_count = 0;
                    try {
                        if (strategy == 0) {
                            seen.insert(seen.end(), windows[wi].begin(), windows[wi].end());
                            merge_sort(seen, HigherPriority{}, &metrics);
                            seen_count = seen.size();
                            peak = std::max<std::uint64_t>(peak, 2ULL * seen.capacity() * sizeof(Record));
                        } else if (strategy == 1) {
                            for (const auto& record : windows[wi]) heap.add(record);
                            current_top = heap.publish();
                            peak = std::max<std::uint64_t>(peak, heap.memory_bytes() + current_top.capacity() * sizeof(Record));
                        } else {
                            auto sorted_window = windows[wi];
                            merge_sort(sorted_window, HigherPriority{}, &metrics);
                            current_top = merge_top_k_lists(current_top, sorted_window, k, &metrics);
                            const std::uint64_t w = windows[wi].capacity() * sizeof(Record);
                            const std::uint64_t top = current_top.capacity() * sizeof(Record);
                            const std::uint64_t current = k * sizeof(Record);
                            peak = std::max<std::uint64_t>(peak, std::max(2 * w + current, w + top + current));
                        }
                        const auto end = Clock::now();
                        const double elapsed = std::chrono::duration<double>(end - begin).count();
                        cumulative += elapsed;
                        transactions_seen += windows[wi].size();
                        seen_count = transactions_seen;
                        const std::uint64_t extra = strategy == 0
                            ? 2ULL * seen.capacity() * sizeof(Record)
                            : (strategy == 1 ? heap.memory_bytes() + current_top.capacity() * sizeof(Record) : peak);
                        peak = std::max(peak, extra);
                        peak_memory[strategy] = std::max(peak_memory[strategy], peak);
                        cumulative_by_trial[trial - 1].push_back(cumulative);
                        peak_by_trial[trial - 1].push_back(peak);
                        ResultRow row{"E", minutes == 60 ? "1-hour" : "10-minute", names[strategy], "ok",
                            seen_count, k, minutes, wi + 1, static_cast<std::size_t>(trial), elapsed, cumulative,
                            metrics.comparisons, metrics.moves, 0, strategy != 1, 0, peak};
                        write_csv_row(results, row);
                    } catch (const RunTimeout&) {
                        const double elapsed = std::chrono::duration<double>(Clock::now() - begin).count();
                        cumulative += elapsed;
                        write_csv_row(results, ResultRow{"E", minutes == 60 ? "1-hour" : "10-minute", names[strategy], "DNF",
                            seen_count, k, minutes, wi + 1, static_cast<std::size_t>(trial), elapsed, cumulative,
                            metrics.comparisons, metrics.moves, 0, strategy != 1, 0, peak});
                        break;
                    }
                }
                for (std::size_t wi = 0; wi < cumulative_by_trial[trial - 1].size(); ++wi)
                    points.push_back({minutes, wi + 1, static_cast<std::size_t>(trial), names[strategy], cumulative_by_trial[trial - 1][wi], peak_by_trial[trial - 1][wi]});
            }
            memory_csv << minutes << ',' << csv_quote(names[strategy]) << ',' << peak_memory[strategy] << ",measured\n";
        }
        std::vector<PlotSeries> series;
        for (std::size_t strategy = 0; strategy < names.size(); ++strategy) {
            PlotSeries line{names[strategy], colors[strategy], {}};
            for (std::size_t window = 1; window <= windows.size(); ++window) {
                std::vector<double> values;
                for (const auto& p : points) if (p.window_minutes == minutes && p.window_index == window && p.strategy == names[strategy]) values.push_back(p.cumulative);
                if (!values.empty()) line.points.emplace_back(static_cast<double>(window), median(values));
            }
            series.push_back(std::move(line));
        }
        const std::string label = minutes == 60 ? "1_hour" : "10_minute";
        write_line_plot((outdir / ("E_cumulative_time_" + label + ".svg")).string(),
            "Live Feed Cumulative Time, " + std::to_string(minutes) + " Minute Windows",
            "Window number (count)", "Cumulative execution time (seconds)",
            "Figure E. Median of five trials. The full refresh includes sorting all records seen so far.", series, false, false);
    }
    memory_csv.close();
}

int main(int argc, char** argv) {
    if (argc < 2) {
        std::cerr << "Usage: queue_experiments <creditcard.csv> [--problem C|D|E|all] [--smoke] [--out directory]\n";
        return 2;
    }
    std::string problem = "all";
    bool smoke = false;
    std::filesystem::path outdir = "results/experiments";
    for (int i = 2; i < argc; ++i) {
        const std::string arg = argv[i];
        if (arg == "--smoke") smoke = true;
        else if (arg == "--problem" && i + 1 < argc) problem = argv[++i];
        else if (arg == "--out" && i + 1 < argc) outdir = argv[++i];
        else { std::cerr << "Unknown option: " << arg << '\n'; return 2; }
    }
    if (problem != "all" && problem != "C" && problem != "D" && problem != "E") {
        std::cerr << "Problem must be C, D, E, or all.\n";
        return 2;
    }
    try {
        std::filesystem::create_directories(outdir);
        const auto dataset = load_dataset(argv[1]); // Loading and score calculation are outside experiment timers.
        std::ofstream results(outdir / "results.csv");
        if (!results) throw std::runtime_error("Cannot create results.csv");
        results << "problem,condition,n,k,window_minutes,window_index,algorithm,trial,seconds,cumulative_seconds,comparisons,moves,extra_memory_bytes,stable,status,captured_frauds,peak_extra_memory_bytes\n";
        std::ofstream predictions(outdir / "prediction_log.csv");
        if (!predictions) throw std::runtime_error("Cannot create prediction_log.csv");
        predictions << "round,phase,n,condition,predicted_winner,predicted_loser,reason,actual_winner,actual_loser,winner_prediction_correct,loser_prediction_correct\n";

        if (problem == "all" || problem == "C") {
            auto c = run_problem_c(dataset, smoke, results, predictions);
            make_problem_c_plots(c, outdir);
        }
        if (problem == "all" || problem == "D") run_problem_d(dataset, smoke, outdir, results);
        if (problem == "all" || problem == "E") run_problem_e(dataset, smoke, outdir, results);
        std::cout << "Saved experiment outputs to " << outdir.string() << '\n';
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Experiment error: " << e.what() << '\n';
        return 1;
    }
}
