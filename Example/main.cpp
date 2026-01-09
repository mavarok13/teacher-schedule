#include <iostream>

#include <excel_util.hpp>

int main () {
    try {
        excel_util::ConvertExcelToCsv("jobs.xlsx", "C:/Users/sam13/source/teacher-schedule/Example/build");
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}