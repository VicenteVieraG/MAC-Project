#include <graphReader.hpp>

#include <charconv>
#include <cmath>
#include <cstddef>
#include <fstream>
#include <istream>
#include <limits>
#include <stdexcept>
#include <string>
#include <system_error>
#include <utility>

namespace {
    std::string_view trim(std::string_view text) {
        constexpr std::string_view whitespace = " \t\r\n\f\v";
        const auto first = text.find_first_not_of(whitespace);
        if(first == std::string_view::npos) return {};
        const auto last = text.find_last_not_of(whitespace);
        return text.substr(first, last - first + 1);
    }

    double parseWeight(std::string_view token, std::size_t line, std::size_t column) {
        token = trim(token);
        if(token == "INF") return std::numeric_limits<double>::infinity();

        const auto error = [line, column] {
            return std::invalid_argument(
                "Invalid graph at line " + std::to_string(line) + ", column " +
                std::to_string(column) + ": expected a finite number or INF.");
        };
        if(token.empty()) throw error();

        // from_chars is locale-independent but does not accept a leading '+'.
        if(token.front() == '+') {
            token.remove_prefix(1);
            if(token.empty() || token.front() == '+' || token.front() == '-')
                throw error();
        }

        double weight{};
        const auto* end = token.data() + token.size();
        const auto result = std::from_chars(token.data(), end, weight);
        if(result.ec != std::errc{} || result.ptr != end || !std::isfinite(weight))
            throw error();
        return weight;
    }

    std::vector<double> parseRow(std::string_view row, std::size_t line) {
        if(row.size() < 2 || row.front() != '|' || row.back() != '|') {
            throw std::invalid_argument(
                "Invalid graph at line " + std::to_string(line) +
                ": each row must start and end with '|'.");
        }
        row.remove_prefix(1);
        row.remove_suffix(1);

        std::vector<double> weights;
        while(true) {
            const auto comma = row.find(',');
            weights.push_back(parseWeight(row.substr(0, comma), line, weights.size() + 1));
            if(comma == std::string_view::npos) break;
            row.remove_prefix(comma + 1);
        }
        return weights;
    }
};

GraphReader::Matrix GraphReader::read(std::string_view path) {
    std::ifstream file{std::string(path)};
    if(!file.is_open())
        throw std::runtime_error("Could not open graph file: " + std::string(path));
    return read(file);
}

GraphReader::Matrix GraphReader::read(std::istream& input) {
    Matrix matrix;
    std::string line;
    std::size_t lineNumber{};
    while(std::getline(input, line)) {
        ++lineNumber;
        const auto rowText = trim(line);
        if(rowText.empty()) continue;

        auto row = parseRow(rowText, lineNumber);
        if(!matrix.empty() && row.size() != matrix.front().size()) {
            throw std::invalid_argument(
                "Invalid graph at line " + std::to_string(lineNumber) + ": expected " +
                std::to_string(matrix.front().size()) + " columns, got " +
                std::to_string(row.size()) + ".");
        }
        matrix.push_back(std::move(row));
    }

    if(input.bad() || (input.fail() && !input.eof()))
        throw std::runtime_error("Could not read graph input.");
    if(matrix.empty())
        throw std::invalid_argument("Invalid graph: the matrix is empty.");
    if(matrix.size() != matrix.front().size()) {
        throw std::invalid_argument(
            "Invalid graph: expected a square matrix, got " +
            std::to_string(matrix.size()) + " rows and " +
            std::to_string(matrix.front().size()) + " columns.");
    }
    return matrix;
}
