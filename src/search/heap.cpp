#include <heap.hpp>

#include <cmath>
#include <stdexcept>
#include <utility>

namespace {
    /**
     * @brief Throw an invalid-argument exception outside the normal execution path.
     * @param message Non-null, null-terminated error message.
     * @throws std::invalid_argument Always.
     * @note noreturn permits throwing; this helper must not be noexcept.
     */
    [[noreturn]] [[gnu::cold]] [[gnu::noinline]] [[gnu::nonnull(1)]] [[gnu::access(read_only, 1)]]
    void invalidArgument(const char* message) {
        throw std::invalid_argument(message);
    }

    /**
     * @brief Throw an out-of-range exception outside the normal execution path.
     * @param message Non-null, null-terminated error message.
     * @throws std::out_of_range Always.
     * @note noreturn permits throwing; this helper must not be noexcept.
     */
    [[noreturn]] [[gnu::cold]] [[gnu::noinline]] [[gnu::nonnull(1)]] [[gnu::access(read_only, 1)]]
    void outOfRange(const char* message) {
        throw std::out_of_range(message);
    }

    /**
     * @brief Validate that a priority can participate in heap ordering.
     * @param priority Value to validate; infinities are accepted.
     * @throws std::invalid_argument If priority is NaN.
     */
    void validatePriority(double priority) {
        if(std::isnan(priority)) [[unlikely]]
            invalidArgument("Heap priorities cannot be NaN.");
    }
}

bool Heap::contains(NodeId node) const noexcept {
    return this->positions.contains(node);
}

void Heap::push(NodeId node, double priority) {
    validatePriority(priority);
    if(this->contains(node)) [[unlikely]]
        invalidArgument("The node is already in the heap.");

    this->entries.push_back({node, priority});
    try {
        this->positions.emplace(node, this->entries.size() - 1);
    } catch(...) {
        // Keep both containers consistent if allocating the index fails.
        this->entries.pop_back();
        throw;
    }
    this->siftUp(this->entries.size() - 1);
}

const Heap::Entry& Heap::top() const {
    if(this->empty()) [[unlikely]]
        outOfRange("Cannot inspect an empty heap.");
    return this->entries.front();
}

Heap::Entry Heap::pop() {
    if(this->empty()) [[unlikely]]
        outOfRange("Cannot pop an empty heap.");

    const Entry minimum = this->entries.front();
    this->positions.erase(minimum.node);
    if(this->entries.size() == 1) {
        this->entries.pop_back();
        return minimum;
    }

    // Fill the root with the last leaf, then restore the heap order downward.
    this->entries.front() = this->entries.back();
    this->entries.pop_back();
    this->positions.at(this->entries.front().node) = 0;
    this->siftDown(0);
    return minimum;
}

void Heap::decreaseKey(NodeId node, double priority) {
    validatePriority(priority);
    const auto found = this->positions.find(node);
    if(found == this->positions.end()) [[unlikely]]
        outOfRange("The node is not in the heap.");

    const auto index = found->second;
    if(priority > this->entries[index].priority) [[unlikely]]
        invalidArgument("decreaseKey cannot increase a priority.");
    this->entries[index].priority = priority;
    this->siftUp(index);
}

void Heap::clear() noexcept {
    this->entries.clear();
    this->positions.clear();
}

bool Heap::precedes(const Entry& first, const Entry& second) noexcept {
    return first.priority < second.priority ||
        (first.priority == second.priority && first.node < second.node);
}

void Heap::swapEntries(std::size_t first, std::size_t second) {
    std::swap(this->entries[first], this->entries[second]);
    this->positions.at(this->entries[first].node) = first;
    this->positions.at(this->entries[second].node) = second;
}

void Heap::siftUp(std::size_t index) {
    while(index > 0) {
        const auto parent = (index - 1) / 2;
        if(!this->precedes(this->entries[index], this->entries[parent])) break;
        this->swapEntries(index, parent);
        index = parent;
    }
}

void Heap::siftDown(std::size_t index) {
    // Only internal nodes have children. This bound also avoids overflow in
    // the child-index calculation for leaves.
    while(index < this->entries.size() / 2) {
        auto child = 2 * index + 1;
        const auto right = child + 1;
        if(right < this->entries.size() && this->precedes(this->entries[right], this->entries[child]))
            child = right;
        if(!this->precedes(this->entries[child], this->entries[index])) break;
        this->swapEntries(index, child);
        index = child;
    }
}
