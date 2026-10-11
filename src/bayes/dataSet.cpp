#include <dataSet.hpp>

#include <vector>
#include <string_view>
#include <iostream>
#include <string>
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

        this->dataSet.push_back({
            std::stoi(age),
            std::stoi(income),
            category == "Camina" ? Category::camina : Category::conduce
        });
    }
}

void DataSet::print() const {
    for(const auto& [age, income, category] : this->dataSet)
        std::cout
            <<"["
            <<age<<", "
            <<income<<", "
            <<(category == Category::camina ? "Camina" : "Conduce")
            <<"]"
            <<std::endl;
}