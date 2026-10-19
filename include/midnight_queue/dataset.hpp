#pragma once

#include "midnight_queue/record.hpp"

#include <array>
#include <cmath>
#include <cstddef>
#include <fstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace midnight_queue {

struct Dataset {
    std::vector<Record> records;
    std::array<double, 28> means{};
    std::array<double, 28> stddevs{};
};

inline std::vector<std::string> split_csv_row(const std::string& line) {
    std::vector<std::string> fields;
    std::string field;
    bool quoted = false;
    for (std::size_t i = 0; i < line.size(); ++i) {
        const char c = line[i];
        if (c == '"') {
            if (quoted && i + 1 < line.size() && line[i + 1] == '"') { field.push_back('"'); ++i; }
            else quoted = !quoted;
        } else if (c == ',' && !quoted) {
            fields.push_back(field);
            field.clear();
        } else field.push_back(c);
    }
    if (quoted) throw std::runtime_error("Malformed CSV: unterminated quoted field");
    fields.push_back(field);
    return fields;
}

inline double parse_number(const std::string& value, std::size_t line_number) {
    std::size_t consumed = 0;
    try {
        const double number = std::stod(value, &consumed);
        if (consumed != value.size() || !std::isfinite(number)) throw std::runtime_error("invalid numeric value");
        return number;
    } catch (const std::exception&) {
        throw std::runtime_error("Invalid numeric CSV field on line " + std::to_string(line_number));
    }
}

inline Dataset load_dataset(const std::string& path) {
    std::ifstream input(path);
    if (!input) throw std::runtime_error("Cannot open dataset: " + path);
    std::string line;
    if (!std::getline(input, line)) throw std::runtime_error("Dataset is empty");
    const auto header = split_csv_row(line);
    if (header.size() != 31 || header.front() != "Time" || header[1] != "V1" || header[28] != "V28" ||
        header[29] != "Amount" || header[30] != "Class")
        throw std::runtime_error("Expected Kaggle creditcard.csv columns Time,V1..V28,Amount,Class");

    struct Raw { double time; std::array<double, 28> v; double amount; int fraud; };
    std::vector<Raw> raw;
    std::array<long double, 28> sums{}, squares{};
    std::uint64_t id = 0;
    while (std::getline(input, line)) {
        ++id;
        if (line.empty()) continue;
        const auto f = split_csv_row(line);
        if (f.size() != 31) throw std::runtime_error("Expected 31 columns on line " + std::to_string(id + 1));
        Raw row{};
        row.time = parse_number(f[0], id + 1);
        for (std::size_t j = 0; j < 28; ++j) {
            row.v[j] = parse_number(f[j + 1], id + 1);
            sums[j] += row.v[j];
            squares[j] += static_cast<long double>(row.v[j]) * row.v[j];
        }
        row.amount = parse_number(f[29], id + 1);
        const double class_value = parse_number(f[30], id + 1);
        if (class_value != 0.0 && class_value != 1.0)
            throw std::runtime_error("Class must be 0 or 1 on line " + std::to_string(id + 1));
        row.fraud = static_cast<int>(class_value);
        raw.push_back(row);
    }
    if (raw.empty()) throw std::runtime_error("Dataset has a header but no transaction rows");

    Dataset result;
    const long double count = raw.size();
    for (std::size_t j = 0; j < 28; ++j) {
        result.means[j] = static_cast<double>(sums[j] / count);
        const long double variance = squares[j] / count - (sums[j] / count) * (sums[j] / count);
        result.stddevs[j] = std::sqrt(static_cast<double>(variance > 0 ? variance : 0));
    }
    result.records.reserve(raw.size());
    for (std::size_t i = 0; i < raw.size(); ++i) {
        long double risk = 0;
        for (std::size_t j = 0; j < 28; ++j) {
            const double z = result.stddevs[j] == 0 ? 0 : (raw[i].v[j] - result.means[j]) / result.stddevs[j];
            risk += static_cast<long double>(z) * z;
        }
        int band = 0;
        if (risk > 0) band = static_cast<int>(std::floor(std::log2(static_cast<double>(risk))));
        if (band < 0) band = 0;
        if (band > 9) band = 9;
        result.records.push_back(Record{static_cast<std::uint64_t>(i + 1), raw[i].time, raw[i].amount, band, raw[i].fraud});
    }
    return result;
}

} // namespace midnight_queue
