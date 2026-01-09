#pragma once

#include <fstream>
#include <filesystem>

#include <xlnt/xlnt.hpp>

namespace fs = std::filesystem;

namespace excel_util {

void ConvertExcelToCsv(const std::string & input_path_str, const std::string & output_folder_str) {

    if (!fs::exists(input_path_str) || !fs::is_regular_file(input_path_str)) {
        throw std::runtime_error("Input Excel file not found: " + input_path_str);
    }

    if (fs::exists(output_folder_str)) {
        if (!fs::is_directory(output_folder_str)) {
            throw std::runtime_error("Output path exists and is not a directory: " + output_folder_str);
        }
    }

    fs::path input_path{input_path_str};
    fs::path output_folder{output_folder_str};

    xlnt::workbook wb;
    try {
        wb.load(input_path_str);

        for (int worksheet_idx = 0; worksheet_idx < wb.sheet_count(); ++worksheet_idx) {
            xlnt::worksheet ws = wb.sheet_by_index(worksheet_idx);

            std::ofstream ofs{output_folder_str + "/" + input_path.stem().string() + "_" + ws.title() + ".csv", std::ios::out};

            for (auto row : ws.rows(false)) {
                for (auto cell : row) {
                    ofs << cell.to_string() << ";";
                }
                ofs << "\n";
            }
        }
    } catch (const std::exception &ex) {
        throw std::runtime_error(std::string("Failed to load Excel workbook: ") + ex.what());
    }
}

} // namespace excel_util