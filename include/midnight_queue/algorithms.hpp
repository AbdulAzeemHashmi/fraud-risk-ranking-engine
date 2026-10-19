#pragma once

#include <cstddef>
#include <cstdint>
#include <chrono>
#include <stdexcept>
#include <utility>
#include <vector>

namespace midnight_queue {

struct Metrics {
    std::uint64_t comparisons{0};
    std::uint64_t moves{0};
    std::chrono::steady_clock::time_point deadline{};
    bool has_deadline{false};

    void set_time_limit(std::chrono::seconds limit) {
        deadline = std::chrono::steady_clock::now() + limit;
        has_deadline = true;
    }
};

struct RunTimeout : std::runtime_error {
    RunTimeout() : std::runtime_error("Run exceeded its time limit") {}
};

// A move is one assignment of an input element to an algorithm-owned slot.
// Key comparisons count calls to less(a,b); index/loop tests are not key comparisons.
template <class T, class Less>
bool counted_less(const T& a, const T& b, Less less, Metrics* m) {
    if (m) {
        ++m->comparisons;
        if (m->has_deadline && (m->comparisons & 16383U) == 0 &&
            std::chrono::steady_clock::now() >= m->deadline) throw RunTimeout{};
    }
    return less(a, b);
}

template <class T, class Less>
void insertion_sort(std::vector<T>& a, Less less, Metrics* m = nullptr) {
    for (std::size_t i = 1; i < a.size(); ++i) {
        T value = std::move(a[i]);
        if (m) ++m->moves;
        std::size_t j = i;
        while (j > 0 && counted_less(value, a[j - 1], less, m)) {
            a[j] = std::move(a[j - 1]);
            if (m) ++m->moves;
            --j;
        }
        a[j] = std::move(value);
        if (m) ++m->moves;
    }
}

template <class T, class Less>
void merge_sort(std::vector<T>& a, Less less, Metrics* m = nullptr) {
    if (a.size() < 2) return;
    std::vector<T> scratch(a.size());
    auto sort_range = [&](auto&& self, std::size_t lo, std::size_t hi) -> void {
        if (hi - lo < 2) return;
        const std::size_t mid = lo + (hi - lo) / 2;
        self(self, lo, mid);
        self(self, mid, hi);
        std::size_t i = lo, j = mid, out = lo;
        while (i < mid && j < hi) {
            if (counted_less(a[j], a[i], less, m)) scratch[out++] = a[j++];
            else scratch[out++] = a[i++];
            if (m) ++m->moves;
        }
        while (i < mid) { scratch[out++] = a[i++]; if (m) ++m->moves; }
        while (j < hi) { scratch[out++] = a[j++]; if (m) ++m->moves; }
        for (std::size_t k = lo; k < hi; ++k) { a[k] = std::move(scratch[k]); if (m) ++m->moves; }
    };
    sort_range(sort_range, 0, a.size());
}

template <class T, class Less>
std::size_t lomuto_partition(std::vector<T>& a, std::size_t lo, std::size_t hi, Less less, Metrics* m) {
    const T pivot = a[hi - 1];
    std::size_t boundary = lo;
    for (std::size_t j = lo; j + 1 < hi; ++j) {
        if (counted_less(a[j], pivot, less, m)) {
            if (boundary != j) { std::swap(a[boundary], a[j]); if (m) m->moves += 3; }
            ++boundary;
        }
    }
    if (boundary != hi - 1) { std::swap(a[boundary], a[hi - 1]); if (m) m->moves += 3; }
    return boundary;
}

template <class T, class Less>
void quick_sort_naive(std::vector<T>& a, Less less, Metrics* m = nullptr) {
    auto sort_range = [&](auto&& self, std::size_t lo, std::size_t hi) -> void {
        while (hi - lo > 1) {
            const std::size_t p = lomuto_partition(a, lo, hi, less, m);
            // Recurse on the smaller side to bound stack usage; order and comparisons are unchanged.
            if (p - lo < hi - (p + 1)) { self(self, lo, p); lo = p + 1; }
            else { self(self, p + 1, hi); hi = p; }
        }
    };
    sort_range(sort_range, 0, a.size());
}

template <class T, class Less>
void quick_sort_hardened(std::vector<T>& a, Less less, Metrics* m = nullptr) {
    // Median-of-three pivot plus three-way partition; handles duplicate-heavy keys well.
    auto sort_range = [&](auto&& self, std::size_t lo, std::size_t hi) -> void {
        while (hi - lo > 1) {
            const std::size_t mid = lo + (hi - lo) / 2;
            std::size_t x = lo, y = mid, z = hi - 1;
            if (counted_less(a[y], a[x], less, m)) std::swap(x, y);
            if (counted_less(a[z], a[y], less, m)) std::swap(y, z);
            if (counted_less(a[y], a[x], less, m)) std::swap(x, y);
            const T pivot = a[y];
            std::size_t lt = lo, i = lo, gt = hi;
            while (i < gt) {
                if (counted_less(a[i], pivot, less, m)) {
                    if (lt != i) { std::swap(a[lt], a[i]); if (m) m->moves += 3; }
                    ++lt; ++i;
                } else if (counted_less(pivot, a[i], less, m)) {
                    --gt;
                    if (i != gt) { std::swap(a[i], a[gt]); if (m) m->moves += 3; }
                } else ++i;
            }
            if (lt - lo < hi - gt) { self(self, lo, lt); lo = gt; }
            else { self(self, gt, hi); hi = lt; }
        }
    };
    sort_range(sort_range, 0, a.size());
}

template <class T, class Less>
void sift_down(std::vector<T>& a, std::size_t root, std::size_t count, Less less, Metrics* m) {
    while (root < count / 2) {
        std::size_t child = root * 2 + 1;
        if (child + 1 < count && counted_less(a[child], a[child + 1], less, m)) ++child;
        if (!counted_less(a[root], a[child], less, m)) return;
        std::swap(a[root], a[child]); if (m) m->moves += 3;
        root = child;
    }
}

template <class T, class Less>
void heap_sort(std::vector<T>& a, Less less, Metrics* m = nullptr) {
    for (std::size_t start = a.size() / 2; start > 0; --start) sift_down(a, start - 1, a.size(), less, m);
    for (std::size_t end = a.size(); end > 1; --end) {
        std::swap(a[0], a[end - 1]); if (m) m->moves += 3;
        sift_down(a, 0, end - 1, less, m);
    }
}

// Return exactly the first min(K,n) records, ordered by priority.
template <class T, class Less>
std::vector<T> top_k_heap(const std::vector<T>& input, std::size_t k, Less higher, Metrics* m = nullptr) {
    if (k == 0 || input.empty()) return {};
    std::vector<T> heap;
    heap.reserve(k);
    auto sift_up = [&](std::size_t i) {
        while (i > 0) {
            std::size_t p = (i - 1) / 2;
            if (!counted_less(heap[p], heap[i], higher, m)) break;
            std::swap(heap[i], heap[p]); if (m) m->moves += 3;
            i = p;
        }
    };
    for (const T& item : input) {
        if (heap.size() < k) { heap.push_back(item); if (m) ++m->moves; sift_up(heap.size() - 1); }
        else if (counted_less(item, heap.front(), higher, m)) {
            heap.front() = item; if (m) ++m->moves;
            sift_down(heap, 0, heap.size(), higher, m);
        }
    }
    // With `higher` as the heap comparator, the root is the lowest priority; heapsort
    // then emits the retained records in the requested high-to-low order.
    heap_sort(heap, higher, m);
    return heap;
}

template <class T, class Less>
std::vector<T> top_k_buffer(const std::vector<T>& input, std::size_t k, Less higher, Metrics* m = nullptr) {
    std::vector<T> out;
    if (k == 0) return out;
    out.reserve(k);
    for (const T& item : input) {
        auto it = out.begin();
        while (it != out.end() && !counted_less(item, *it, higher, m)) ++it;
        if (it == out.end() && out.size() == k) continue;
        const auto position = static_cast<std::size_t>(it - out.begin());
        if (out.size() < k) {
            const auto old_size = out.size();
            out.push_back(item);
            if (m) ++m->moves;
            for (std::size_t i = old_size; i > position; --i) {
                out[i] = std::move(out[i - 1]);
                if (m) ++m->moves;
            }
            if (position < old_size) {
                out[position] = item;
                if (m) ++m->moves;
            }
        } else {
            for (std::size_t i = out.size() - 1; i > position; --i) {
                out[i] = std::move(out[i - 1]);
                if (m) ++m->moves;
            }
            out[position] = item;
            if (m) ++m->moves;
        }
    }
    return out;
}

} // namespace midnight_queue
