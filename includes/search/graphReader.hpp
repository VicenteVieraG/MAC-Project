#pragma once

#include <iosfwd>
#include <string_view>
#include <vector>

class GraphReader {
    using Matrix = std::vector<std::vector<double>>;
    public:
        /*
            Reads a nonempty square matrix, one | value, value, ... | row per line.
            INF means no connection and is stored as positive infinity. Finite
            weights (including zero and negative values) are preserved as written.
            Whitespace and blank lines are ignored; decimal and exponent forms work.
            Throws std::invalid_argument for malformed input, std::runtime_error
            for file/stream errors. Format errors include line/column where possible.
         */
        [[nodiscard]] static Matrix read(std::string_view path);
        [[nodiscard]] static Matrix read(std::istream& input);
};
