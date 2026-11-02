#pragma once

#include <gridView.hpp>
#include <graphView.hpp>
#include <benchmarkView.hpp>

class Search {
    public:
        void draw();
    private:
        GridView gridView;
        GraphView graphView;
        BenchmarkView benchmarkView;
};
