#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>

#include <heap.hpp>

#include <algorithm>
#include <cstddef>
#include <limits>
#include <map>
#include <random>
#include <stdexcept>
#include <utility>
#include <vector>

namespace {
    void requireEntry(const Heap::Entry& actual, Heap::NodeId node, double priority) {
        REQUIRE(actual.node == node);
        REQUIRE(actual.priority == priority);
    }
}

TEST_CASE("Heap handles empty and single-node frontiers", "[search][heap]") {
    Heap heap;
    REQUIRE(heap.empty());
    REQUIRE(heap.size() == 0);
    REQUIRE_FALSE(heap.contains(0));
    REQUIRE_THROWS_AS(heap.top(), std::out_of_range);
    REQUIRE_THROWS_AS(heap.pop(), std::out_of_range);

    heap.push(0, 2.5);
    const Heap& frontier = heap;
    REQUIRE_FALSE(frontier.empty());
    REQUIRE(frontier.size() == 1);
    REQUIRE(frontier.contains(0));
    requireEntry(frontier.top(), 0, 2.5);
    requireEntry(frontier.top(), 0, 2.5);
    requireEntry(heap.pop(), 0, 2.5);
    REQUIRE(heap.empty());
    REQUIRE_FALSE(heap.contains(0));
    REQUIRE_THROWS_AS(heap.pop(), std::out_of_range);

    heap.push(0, -1.5);
    requireEntry(heap.pop(), 0, -1.5);
}

TEST_CASE("Heap extracts priorities in order regardless of insertion order", "[search][heap]") {
    const auto count = GENERATE(2U, 3U, 4U, 31U, 32U, 33U);
    const auto order = GENERATE(0, 1, 2);
    CAPTURE(count, order);
    std::vector<Heap::Entry> entries;
    for(std::size_t node = 0; node < count; ++node)
        entries.push_back({node, static_cast<double>(node) * 0.5 - 10.0});
    if(order == 1) std::reverse(entries.begin(), entries.end());
    if(order == 2) {
        std::mt19937 random(42);
        std::shuffle(entries.begin(), entries.end(), random);
    }

    Heap heap;
    for(const auto& entry : entries) heap.push(entry.node, entry.priority);
    for(std::size_t node = 0; node < count; ++node) {
        REQUIRE(heap.size() == count - node);
        requireEntry(heap.top(), node, static_cast<double>(node) * 0.5 - 10.0);
        requireEntry(heap.pop(), node, static_cast<double>(node) * 0.5 - 10.0);
        REQUIRE_FALSE(heap.contains(node));
    }
    REQUIRE(heap.empty());
}

TEST_CASE("Heap breaks equal priorities by node ID", "[search][heap]") {
    Heap heap;
    for(const auto node : {9U, 2U, 5U, 0U, 7U}) heap.push(node, 4.0);
    heap.push(1, 10.0);
    heap.decreaseKey(1, 4.0);
    for(const auto node : {0U, 1U, 2U, 5U, 7U, 9U})
        requireEntry(heap.pop(), node, 4.0);
}

TEST_CASE("Heap decreases costs after insertion and extraction", "[search][heap]") {
    Heap heap;
    for(std::size_t node = 0; node < 10; ++node)
        heap.push(node, static_cast<double>(node + 10));

    heap.decreaseKey(9, 1.0);
    heap.decreaseKey(9, 1.0);
    REQUIRE(heap.size() == 10);
    requireEntry(heap.pop(), 9, 1.0);
    // Extraction moves a leaf to the root and repairs its position index.
    heap.decreaseKey(8, 2.0);
    heap.decreaseKey(5, 3.0);
    requireEntry(heap.pop(), 8, 2.0);
    requireEntry(heap.pop(), 5, 3.0);
    heap.decreaseKey(0, -1.0);
    requireEntry(heap.pop(), 0, -1.0);
    REQUIRE_THROWS_AS(heap.decreaseKey(0, -2.0), std::out_of_range);
    heap.push(0, 0.0);
    requireEntry(heap.pop(), 0, 0.0);
    for(const auto node : {1U, 2U, 3U, 4U, 6U, 7U})
        requireEntry(heap.pop(), node, static_cast<double>(node + 10));
    REQUIRE(heap.empty());
}

TEST_CASE("Heap rejects invalid updates without changing the frontier", "[search][heap]") {
    Heap heap;
    heap.push(3, 1.0);
    heap.push(5, 2.0);
    const auto nan = std::numeric_limits<double>::quiet_NaN();

    REQUIRE_THROWS_AS(heap.push(3, -10.0), std::invalid_argument);
    REQUIRE_THROWS_AS(heap.push(7, nan), std::invalid_argument);
    REQUIRE_THROWS_AS(heap.decreaseKey(3, nan), std::invalid_argument);
    REQUIRE_THROWS_AS(heap.decreaseKey(3, 10.0), std::invalid_argument);
    REQUIRE_THROWS_AS(heap.decreaseKey(7, 0.0), std::out_of_range);
    REQUIRE(heap.size() == 2);
    REQUIRE_FALSE(heap.contains(7));
    requireEntry(heap.pop(), 3, 1.0);
    requireEntry(heap.pop(), 5, 2.0);
    REQUIRE(heap.empty());
}

TEST_CASE("Heap supports infinite priorities and sparse node IDs", "[search][heap]") {
    const auto infinity = std::numeric_limits<double>::infinity();
    const auto largestNode = std::numeric_limits<Heap::NodeId>::max();
    Heap heap;
    heap.push(largestNode, infinity);
    heap.push(0, infinity);
    heap.push(1000000, -infinity);
    heap.push(42, -0.0);
    heap.push(41, 0.0);
    heap.decreaseKey(largestNode, 0.5);

    requireEntry(heap.pop(), 1000000, -infinity);
    requireEntry(heap.pop(), 41, 0.0);
    requireEntry(heap.pop(), 42, 0.0);
    requireEntry(heap.pop(), largestNode, 0.5);
    requireEntry(heap.pop(), 0, infinity);
}

TEST_CASE("Heap can be cleared and reused", "[search][heap]") {
    Heap heap;
    heap.clear();
    heap.push(3, 1.0);
    heap.push(7, 2.0);
    heap.clear();
    REQUIRE(heap.empty());
    REQUIRE(heap.size() == 0);
    REQUIRE_FALSE(heap.contains(3));
    REQUIRE_FALSE(heap.contains(7));
    REQUIRE_THROWS_AS(heap.top(), std::out_of_range);
    REQUIRE_THROWS_AS(heap.decreaseKey(3, 0.0), std::out_of_range);
    heap.push(7, 3.0);
    heap.push(3, 4.0);
    heap.decreaseKey(3, 2.0);
    requireEntry(heap.pop(), 3, 2.0);
    requireEntry(heap.pop(), 7, 3.0);
}

TEST_CASE("Heap matches a reference frontier through mixed operations", "[search][heap]") {
    Heap heap;
    std::map<Heap::NodeId, double> reference;
    std::mt19937 random(20261029);
    const auto minimum = [&reference] {
        // A linear scan is intentionally independent of the heap algorithm.
        const auto found = std::min_element(reference.begin(), reference.end(),
            [](const auto& first, const auto& second) {
                return std::pair{first.second, first.first} <
                    std::pair{second.second, second.first};
            });
        return Heap::Entry{found->first, found->second};
    };

    for(std::size_t step = 0; step < 3000; ++step) {
        CAPTURE(step);
        const Heap::NodeId node = random() % 128;
        if(random() % 4 != 0 || reference.empty()) {
            if(reference.contains(node)) {
                reference.at(node) -= static_cast<double>(random() % 10);
                heap.decreaseKey(node, reference.at(node));
            } else {
                const auto priority = static_cast<double>(random() % 100) - 50.0;
                heap.push(node, priority);
                reference.emplace(node, priority);
            }
        } else {
            const auto expected = minimum();
            requireEntry(heap.pop(), expected.node, expected.priority);
            reference.erase(expected.node);
            REQUIRE_FALSE(heap.contains(expected.node));
        }
        REQUIRE(heap.size() == reference.size());
        REQUIRE(heap.empty() == reference.empty());
        REQUIRE(heap.contains(node) == reference.contains(node));
        if(!reference.empty()) {
            const auto expected = minimum();
            requireEntry(heap.top(), expected.node, expected.priority);
        }
    }
    while(!reference.empty()) {
        const auto expected = minimum();
        requireEntry(heap.pop(), expected.node, expected.priority);
        reference.erase(expected.node);
    }
    REQUIRE(heap.empty());
}
