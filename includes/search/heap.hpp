#pragma once

#include <cstddef>
#include <unordered_map>
#include <vector>

/**
 * @brief Binary min-heap for a Dijkstra or A* search frontier.
 *
 * Each node appears at most once. Lower priorities are extracted first, with
 * ties resolved by the smaller node ID. Storage is O(n) for n queued nodes.
 * Finite and infinite priorities are supported; NaN priorities are rejected.
 *
 * @note Observers read heap memory, so GNU @c pure is applicable whereas
 *       GNU @c const is not. Operations that can throw retain their exceptions.
 */
class Heap {
    public:
        /// @brief Identifier of a node in the search graph.
        using NodeId = std::size_t;

        /// @brief A queued node and its search priority.
        struct [[nodiscard("Use the extracted node and its priority.")]] Entry {
            NodeId node;     ///< Identifier of the queued node.
            double priority; ///< Accumulated cost g(n), or estimated total g(n) + h(n).
        };

        /**
         * @brief Check whether the frontier is empty in O(1) time.
         * @return True if no nodes are queued.
         */
        [[nodiscard("Use the heap's empty state.")]] [[gnu::pure]] [[gnu::always_inline]]
        auto empty() const noexcept -> bool { return this->entries.empty(); }

        /**
         * @brief Query the frontier size in O(1) time.
         * @return Number of queued nodes.
         */
        [[nodiscard("Use the number of queued nodes.")]] [[gnu::pure]] [[gnu::always_inline]]
        auto size() const noexcept -> std::size_t { return this->entries.size(); }

        /**
         * @brief Check node membership in expected O(1) time.
         * @param node Node ID to look up.
         * @return True if the node is currently queued.
         */
        [[nodiscard("Use the node's membership state.")]] [[gnu::pure]]
        bool contains(NodeId node) const noexcept;

        /**
         * @brief Insert a node in amortized expected O(log n) time.
         * @param node Node ID not already in the frontier.
         * @param priority Search priority; finite values and infinities are allowed.
         * @throws std::invalid_argument If the node already exists or priority is NaN.
         * @throws std::bad_alloc If storage allocation fails.
         * @note Failed insertion leaves the queued nodes and priorities unchanged.
         */
        void push(NodeId node, double priority);

        /**
         * @brief Inspect the minimum entry without removing it in O(1) time.
         * @return Const reference to the entry with the lowest priority.
         * @throws std::out_of_range If the heap is empty.
         * @warning Do not retain the returned reference across heap mutations.
         */
        [[nodiscard("Use the minimum entry before mutating the heap.")]]
        const Entry& top() const;

        /**
         * @brief Remove the minimum entry in expected O(log n) time.
         * @return Removed entry by value; its node may subsequently be reinserted.
         * @throws std::out_of_range If the heap is empty.
         */
        [[nodiscard("Use the removed entry, or explicitly discard it with (void).")]]
        Entry pop();

        /**
         * @brief Lower a queued node's priority in expected O(log n) time.
         * @param node Node ID already in the frontier.
         * @param priority New priority, less than or equal to the current priority.
         * @throws std::out_of_range If the node is missing.
         * @throws std::invalid_argument If priority is NaN or exceeds the current value.
         * @note Equal priorities preserve ordering. Rejected updates leave the
         *       queued nodes and priorities unchanged.
         */
        void decreaseKey(NodeId node, double priority);

        /**
         * @brief Remove all nodes so the heap can be reused.
         * @post empty() is true and size() is zero.
         */
        void clear() noexcept;

    private:
        /// @brief Array representation of the binary min-heap.
        std::vector<Entry> entries;

        /// @brief Node-to-index lookup maintained through swaps for decreaseKey().
        std::unordered_map<NodeId, std::size_t> positions;

        /**
         * @brief Compare entries by priority, then by node ID.
         * @param first Entry whose precedence is being tested.
         * @param second Entry to compare against.
         * @return True if first should be extracted before second.
         */
        [[nodiscard]] [[gnu::pure]] [[gnu::access(read_only, 1)]] [[gnu::access(read_only, 2)]]
        static bool precedes(const Entry& first, const Entry& second) noexcept;

        /**
         * @brief Exchange two entries and update their position records.
         * @param first Index of the first entry.
         * @param second Index of the second entry.
         * @pre Both indices are valid and both nodes have position records.
         */
        void swapEntries(std::size_t first, std::size_t second);

        /**
         * @brief Restore heap order upward after insertion or a priority decrease.
         * @param index Index of the entry to move toward the root.
         * @pre The index is valid and only its ancestor relationships may be unordered.
         */
        void siftUp(std::size_t index);

        /**
         * @brief Restore heap order downward after replacing the root.
         * @param index Index of the entry to move toward the leaves.
         * @pre The index is valid and its child subtrees already satisfy heap order.
         */
        void siftDown(std::size_t index);
};
