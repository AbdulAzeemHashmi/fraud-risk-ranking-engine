#pragma once

#include <algorithm>
#include <cmath>
#include <fstream>
#include <iomanip>
#include <limits>
#include <string>
#include <utility>
#include <vector>

namespace midnight_queue {

struct PlotSeries {
    std::string name;
    std::string color;
    std::vector<std::pair<double, double>> points;
};

inline std::string svg_escape(const std::string& value) {
    std::string out;
    for (char c : value) {
        if (c == '&') out += "&amp;";
        else if (c == '<') out += "&lt;";
        else if (c == '>') out += "&gt;";
        else if (c == '"') out += "&quot;";
        else out += c;
    }
    return out;
}

inline bool write_line_plot(const std::string& path, const std::string& title,
                            const std::string& x_label, const std::string& y_label,
                            const std::string& caption, const std::vector<PlotSeries>& series,
                            bool log_x = false, bool log_y = false) {
    constexpr double width = 1000, height = 640;
    constexpr double left = 100, right = 300, top = 70, bottom = 120;
    const double plot_w = width - left - right, plot_h = height - top - bottom;
    double xmin = std::numeric_limits<double>::infinity(), xmax = 0;
    double ymin = std::numeric_limits<double>::infinity(), ymax = 0;
    for (const auto& s : series) for (auto [x, y] : s.points) {
        if ((log_x && x <= 0) || (log_y && y <= 0)) continue;
        xmin = std::min(xmin, x); xmax = std::max(xmax, x);
        ymin = std::min(ymin, y); ymax = std::max(ymax, y);
    }
    if (!std::isfinite(xmin) || !std::isfinite(ymin)) return false;
    auto transform = [](double v, bool logarithmic) { return logarithmic ? std::log10(v) : v; };
    double tx0 = transform(xmin, log_x), tx1 = transform(xmax, log_x);
    double ty0 = transform(ymin, log_y), ty1 = transform(ymax, log_y);
    if (tx0 == tx1) { tx0 -= 0.5; tx1 += 0.5; }
    if (ty0 == ty1) { ty0 -= 0.5; ty1 += 0.5; }
    const auto px = [&](double x) { return left + (transform(x, log_x) - tx0) / (tx1 - tx0) * plot_w; };
    const auto py = [&](double y) { return top + plot_h - (transform(y, log_y) - ty0) / (ty1 - ty0) * plot_h; };

    std::ofstream out(path);
    if (!out) return false;
    out << "<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"1000\" height=\"640\" viewBox=\"0 0 1000 640\">\n"
        << "<rect width=\"100%\" height=\"100%\" fill=\"white\"/>\n"
        << "<text x=\"500\" y=\"34\" text-anchor=\"middle\" font-family=\"Arial\" font-size=\"22\">" << svg_escape(title) << "</text>\n";
    for (int t = 0; t <= 4; ++t) {
        const double fx = t / 4.0, fy = t / 4.0;
        const double x = left + fx * plot_w, y = top + fy * plot_h;
        const double xv = log_x ? std::pow(10.0, tx0 + fx * (tx1 - tx0)) : xmin + fx * (xmax - xmin);
        const double yv = log_y ? std::pow(10.0, ty1 - fy * (ty1 - ty0)) : ymax - fy * (ymax - ymin);
        out << "<line x1=\"" << x << "\" y1=\"" << top << "\" x2=\"" << x << "\" y2=\"" << top + plot_h << "\" stroke=\"#e3e3e3\"/>\n"
            << "<line x1=\"" << left << "\" y1=\"" << y << "\" x2=\"" << left + plot_w << "\" y2=\"" << y << "\" stroke=\"#e3e3e3\"/>\n"
            << "<text x=\"" << x << "\" y=\"" << top + plot_h + 24 << "\" text-anchor=\"middle\" font-family=\"Arial\" font-size=\"12\">" << std::setprecision(3) << xv << "</text>\n"
            << "<text x=\"" << left - 12 << "\" y=\"" << y + 4 << "\" text-anchor=\"end\" font-family=\"Arial\" font-size=\"12\">" << std::setprecision(3) << yv << "</text>\n";
    }
    out << "<line x1=\"" << left << "\" y1=\"" << top + plot_h << "\" x2=\"" << left + plot_w << "\" y2=\"" << top + plot_h << "\" stroke=\"black\"/>\n"
        << "<line x1=\"" << left << "\" y1=\"" << top << "\" x2=\"" << left << "\" y2=\"" << top + plot_h << "\" stroke=\"black\"/>\n"
        << "<text x=\"" << left + plot_w / 2 << "\" y=\"" << top + plot_h + 54 << "\" text-anchor=\"middle\" font-family=\"Arial\" font-size=\"15\">" << svg_escape(x_label) << "</text>\n"
        << "<text x=\"22\" y=\"" << top + plot_h / 2 << "\" transform=\"rotate(-90 22 " << top + plot_h / 2 << ")\" text-anchor=\"middle\" font-family=\"Arial\" font-size=\"15\">" << svg_escape(y_label) << "</text>\n";
    for (std::size_t si = 0; si < series.size(); ++si) {
        const auto& s = series[si];
        bool have_previous = false;
        double previous_x = 0, previous_y = 0;
        for (auto [x, y] : s.points) {
            if ((log_x && x <= 0) || (log_y && y <= 0)) { have_previous = false; continue; }
            if (have_previous) {
                out << "<line x1=\"" << px(previous_x) << "\" y1=\"" << py(previous_y) << "\" x2=\"" << px(x) << "\" y2=\"" << py(y) << "\" stroke=\"" << s.color << "\" stroke-width=\"2\"/>\n";
            }
            out << "<circle cx=\"" << px(x) << "\" cy=\"" << py(y) << "\" r=\"3\" fill=\"" << s.color << "\"/>\n";
            previous_x = x;
            previous_y = y;
            have_previous = true;
        }
        out << "<line x1=\"740\" y1=\"" << 92 + si * 25 << "\" x2=\"765\" y2=\"" << 92 + si * 25 << "\" stroke=\"" << s.color << "\" stroke-width=\"3\"/>\n"
            << "<text x=\"773\" y=\"" << 97 + si * 25 << "\" font-family=\"Arial\" font-size=\"13\">" << svg_escape(s.name) << "</text>\n";
    }
    out << "<text x=\"500\" y=\"615\" text-anchor=\"middle\" font-family=\"Arial\" font-size=\"12\">" << svg_escape(caption) << "</text>\n</svg>\n";
    return static_cast<bool>(out);
}

inline bool write_grouped_bar_plot(const std::string& path, const std::string& title,
                                   const std::string& x_label, const std::string& y_label,
                                   const std::string& caption,
                                   const std::vector<std::string>& categories,
                                   const std::vector<std::string>& names,
                                   const std::vector<std::string>& colors,
                                   const std::vector<std::vector<double>>& values) {
    constexpr double width = 1000, height = 640;
    constexpr double left = 100, right = 300, top = 70, bottom = 120;
    const double plot_w = width - left - right, plot_h = height - top - bottom;
    double ymax = 0;
    for (const auto& row : values) for (double value : row) ymax = std::max(ymax, value);
    if (categories.empty() || names.empty() || ymax <= 0) return false;
    ymax *= 1.08;
    std::ofstream out(path);
    if (!out) return false;
    out << "<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"1000\" height=\"640\" viewBox=\"0 0 1000 640\">\n"
        << "<rect width=\"100%\" height=\"100%\" fill=\"white\"/>\n"
        << "<text x=\"500\" y=\"34\" text-anchor=\"middle\" font-family=\"Arial\" font-size=\"22\">" << svg_escape(title) << "</text>\n";
    for (int t = 0; t <= 4; ++t) {
        const double fraction = t / 4.0;
        const double y = top + plot_h - fraction * plot_h;
        out << "<line x1=\"" << left << "\" y1=\"" << y << "\" x2=\"" << left + plot_w << "\" y2=\"" << y << "\" stroke=\"#e3e3e3\"/>\n"
            << "<text x=\"" << left - 12 << "\" y=\"" << y + 4 << "\" text-anchor=\"end\" font-family=\"Arial\" font-size=\"12\">" << std::setprecision(3) << ymax * fraction << "</text>\n";
    }
    const double group_w = plot_w / categories.size();
    const double bar_w = group_w * 0.78 / names.size();
    for (std::size_t ci = 0; ci < categories.size(); ++ci) {
        const double group_left = left + ci * group_w + group_w * 0.11;
        for (std::size_t si = 0; si < names.size(); ++si) {
            const double value = values[si][ci];
            const double h = value / ymax * plot_h;
            const double x = group_left + si * bar_w;
            out << "<rect x=\"" << x << "\" y=\"" << top + plot_h - h << "\" width=\"" << bar_w * 0.92 << "\" height=\"" << h << "\" fill=\"" << colors[si] << "\"/>\n";
        }
        out << "<text x=\"" << left + (ci + 0.5) * group_w << "\" y=\"" << top + plot_h + 24 << "\" text-anchor=\"middle\" font-family=\"Arial\" font-size=\"13\">" << svg_escape(categories[ci]) << "</text>\n";
    }
    out << "<line x1=\"" << left << "\" y1=\"" << top + plot_h << "\" x2=\"" << left + plot_w << "\" y2=\"" << top + plot_h << "\" stroke=\"black\"/>\n"
        << "<line x1=\"" << left << "\" y1=\"" << top << "\" x2=\"" << left << "\" y2=\"" << top + plot_h << "\" stroke=\"black\"/>\n"
        << "<text x=\"" << left + plot_w / 2 << "\" y=\"" << top + plot_h + 54 << "\" text-anchor=\"middle\" font-family=\"Arial\" font-size=\"15\">" << svg_escape(x_label) << "</text>\n"
        << "<text x=\"22\" y=\"" << top + plot_h / 2 << "\" transform=\"rotate(-90 22 " << top + plot_h / 2 << ")\" text-anchor=\"middle\" font-family=\"Arial\" font-size=\"15\">" << svg_escape(y_label) << "</text>\n";
    for (std::size_t si = 0; si < names.size(); ++si) {
        out << "<rect x=\"740\" y=\"" << 92 + si * 25 << "\" width=\"17\" height=\"15\" fill=\"" << colors[si] << "\"/>\n"
            << "<text x=\"765\" y=\"" << 105 + si * 25 << "\" font-family=\"Arial\" font-size=\"13\">" << svg_escape(names[si]) << "</text>\n";
    }
    out << "<text x=\"500\" y=\"615\" text-anchor=\"middle\" font-family=\"Arial\" font-size=\"12\">" << svg_escape(caption) << "</text>\n</svg>\n";
    return static_cast<bool>(out);
}

} // namespace midnight_queue
