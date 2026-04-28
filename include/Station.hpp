#pragma once
#include <string>
#include <iostream>

struct Station {
    std::string name;
    std::string url;

    void print() const {
        std::cout << "Name: " << name << "\n" << "URL:  " << url << "\n\n";
    }
};
