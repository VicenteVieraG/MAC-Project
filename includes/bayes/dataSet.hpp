#pragma once

#include <vector>
#include <string_view>

enum class Category {
    camina,
    conduce
};

struct Person {
    int age;
    int income;
    Category category;
};

class DataSet {
    private: std::vector<Person> dataSet;
    public:
        void read(const std::string_view path);
        void print() const;
};

