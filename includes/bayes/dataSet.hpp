#pragma once

#include <vector>
#include <string_view>

enum class Category {
    camina,
    conduce
};

struct Person {
    double age;
    double income;
    Category category;
};

enum class BinAge {
    joven,
    adulto,
    mayor
};

enum class BinIncome {
    muyBajo,
    bajo,
    medio,
    alto,
    muyAlto
};

class DataSet {
    public:
        std::vector<Person> records;

        void read(const std::string_view path);
        void print() const;
};

