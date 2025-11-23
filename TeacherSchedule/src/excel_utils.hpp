#pragma once

#include <sstream>
#include <fstream>
#include <string>
#include <filesystem>
#include <algorithm>

#include <boost/algorithm/string/replace.hpp>

#include <xlnt/xlnt.hpp>

namespace fs = std::filesystem;

using namespace std::literals;

namespace excel_utils {

// * All supported excel file formats
constexpr char const * SUPPORTED_EXCEL_FILE_FORMATS[]{
	".xlsx",
	".xlsm",
	".xls",
};

void ConvertExcelToCsv(std::string_view input_excel_file, std::string_view output_dir) {
	fs::path input_path(input_excel_file);
	fs::path output_dir_path(output_dir);

	if (fs::exists(input_path) && fs::exists(output_dir_path)) {
//	*	* Check output directory path 
		if (output_dir_path.has_extension()) {
			throw std::logic_error("Invalid output directory path");
		}
//	*	* Check input file
		unsigned supported_excel_formats_count = sizeof(SUPPORTED_EXCEL_FILE_FORMATS)/sizeof(SUPPORTED_EXCEL_FILE_FORMATS[0]);
		std::string input_file_extension = input_path.extension().string();
		auto seff_begin = SUPPORTED_EXCEL_FILE_FORMATS;
		auto seff_end = SUPPORTED_EXCEL_FILE_FORMATS + supported_excel_formats_count;
		if (std::find_if(seff_begin, seff_end, [&input_file_extension](const char * const str) { return std::strcmp(input_file_extension.c_str(), str);}) == seff_end) {
			throw std::logic_error("Invalid input excel file format");
		}

//	*	* Format output directory path (add "/" in the end)
		if (output_dir_path.has_filename() && output_dir_path.filename().string() == "") {
			output_dir_path = output_dir_path.parent_path();
		}

		xlnt::workbook wb;
		wb.load(input_path.string());
		for (int sheet_idx = 0; sheet_idx < wb.sheet_count(); ++sheet_idx) {
			auto ws = wb.sheet_by_index(sheet_idx);

//	*	*	* Create path to temp csv files
			fs::path output_csv_file_path{ output_dir_path };
			//output_csv_file = input_path.stem().string() + std::to_string(sheet_idx) + ".csv"s;
			output_csv_file_path /= input_path.stem();
			output_csv_file_path += std::to_string(sheet_idx);
			output_csv_file_path.replace_extension("csv");
			std::ofstream ofs(output_csv_file_path);

			std::stringstream ss;
			for (auto row : ws.rows(false)) {
				for (auto cell : row) {
					std::string value = cell.to_string();
					
					boost::replace_all(value, "\"", "\\\"");

					ss << value << ';';
				}
				ss << "\n";
			}

			ofs << ss.str();

			ofs.close();
		}
	} else if (!fs::exists(input_path)) {
		throw std::logic_error("Couldn't find input file:" + input_path.string());
	} else if (!fs::exists(output_dir_path)) {
		throw std::logic_error("Couldn't find output directory: " + output_dir_path.string());
	}
}

} // namespace excel_utils