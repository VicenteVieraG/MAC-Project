#include <dataSet.hpp>

#include <iostream>
#include <vector>
#include <string>
#include <string_view>
#include <fstream>
#include <sstream>

void DataSet::read(const std::string_view path){
    std::ifstream file(path.data());

    if(!file){
        std::cerr<<"[Error]: No se pudo abrir el archivo"<<std::endl;
        return;
    }

    std::string line;
    std::getline(file, line); // Skip the header
    while(std::getline(file, line)){
        std::stringstream ss(line);

        std::string age, income, category;
        std::getline(ss, age, ',');
        std::getline(ss, income, ',');
        std::getline(ss, category, ',');

        this->records.push_back({
            std::stod(age),
            std::stod(income),
            category == "Camina" ? Category::camina : Category::conduce
        });
    }
}

void DataSet::print() const {
    for(const auto& [age, income, category] : this->records)
        std::cout
            <<"["
            <<age<<", "
            <<income<<", "
            <<(category == Category::camina ? "Camina" : "Conduce")
            <<"]"
            <<std::endl;
}