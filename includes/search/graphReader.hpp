#pragma once

#include <iosfwd>
#include <string_view>
#include <vector>

/**
 * @brief Read a nonempty square adjacency matrix from text.
 *
 * Each nonblank line must contain one row in the form `| value, value, ... |`.
 * Surrounding whitespace on rows and values is ignored. Uppercase `INF` means
 * no connection and is stored as positive infinity. Finite decimal weights,
 * including zero, negative values, and scientific notation, are preserved.
 * Entries retain their row/column order; symmetry and diagonal values are not
 * constrained. Format errors include a line and matrix column where possible.
 */
class GraphReader {
    /// @brief Adjacency weights indexed by source vertex, then destination vertex.
    using Matrix = std::vector<std::vector<double>>;
    public:
        /**
         * @brief Open a text file and read its adjacency matrix.
         * @param path Path to the graph file.
         * @return Nonempty square matrix of parsed weights.
         * @throws std::invalid_argument If the input is malformed, empty, ragged,
         *         or not square.
         * @throws std::runtime_error If the file cannot be opened or read.
         * @see read(std::istream&)
         */
        [[nodiscard]] static Matrix read(std::string_view path);

        /**
         * @brief Read an adjacency matrix from the stream's current position to EOF.
         * @param input Stream containing graph rows in the format described above.
         * @return Nonempty square matrix of parsed weights.
         * @throws std::invalid_argument If the input is malformed, empty, ragged,
         *         or not square.
         * @throws std::runtime_error If a stream error is detected after reading.
         * @note The stream remains open and its state flags are not cleared.
         *       Exceptions enabled by the stream's exception mask propagate,
         *       including exceptions on reaching EOF.
         */
        [[nodiscard]] static Matrix read(std::istream& input);
};
