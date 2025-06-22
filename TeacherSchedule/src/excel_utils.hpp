#pragma once

#include <sstream>
#include <fstream>
#include <string>
#include <filesystem>

#include <xlnt/xlnt.hpp>

namespace fs = std::filesystem;

using namespace std::literals;

namespace excel_utils {

void ConvertExcelToCsv(std::string_view input_excel_file, std::string_view output_dir) {
	fs::path input_path(input_excel_file);

	if (fs::exists(input_path) && fs::exists(output_dir)) {
		xlnt::workbook wb;
		wb.load(input_path.c_str());
		for (int sheet_idx = 0; sheet_idx < wb.sheet_count(); ++sheet_idx) {
			auto ws = wb.sheet_by_index(sheet_idx);

			std::string output_csv_file{output_dir.data(), output_dir.size()};
			output_csv_file += input_path.stem().string() + "_"s + std::to_string(sheet_idx) + ".csv"s;
			std::ofstream ofs(output_csv_file);

			for (auto row : ws.rows(false)) {
				bool empty_row = true;

				std::stringstream ss;
				for (auto cell : row) {
					std::string value = cell.to_string();

					if (!value.empty()) {
						empty_row = false;
					}
					ss << value << ';';
				}

				if (!empty_row) {
					ofs << ss.str() << '\n';
				}
			}

			ofs.close();
		}
	} else {
		throw std::exception("Incorrect input and output paths");
	}
}

} // namespace excel_utils