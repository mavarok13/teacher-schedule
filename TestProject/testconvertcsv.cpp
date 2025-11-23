#include <filesystem>
#include <iostream>

#include "../TeacherSchedule/src/excel_utils.hpp"

namespace fs = std::filesystem;

int main () {
    setlocale(LC_ALL, "ru_RU");

    // fs::path path{"4 курс - ТИТиЛП - 2022 - вечер.xlsx"};
    fs::path path{"test.xlsx"};
    fs::path opath{"."};

    excel_utils::ConvertExcelToCsv(fs::absolute(path).c_str(), fs::absolute(opath).c_str());

    return 0;
}