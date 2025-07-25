// ! ISSUES !
// - TROUBLE WITH WSTRING AFTER UPPERCASE CONVERTATION
// - IOSTREAM HEADER INCLUDE WHEN DEBUG
// - COMPLETE CONVERTER CLASS

#pragma once

#include <algorithm>
#include <filesystem>
#include <string>

// ##### ADD FOR DEBUG (IOSTREAM HEADER INCLUDE WHEN DEBUG) ##### //
#include <iostream>
// ############################################################## //

#include <string_converter.hpp>
#include <core.hpp>

#include <rapidcsv.h>

namespace fs = std::filesystem;

namespace app {

constexpr unsigned WORK_DAYS_COUNT = 6u;
constexpr unsigned DAY_PAIRS_COUNT = 8u;
const std::wstring DAYS_NAMES[WORK_DAYS_COUNT]{L"ом", L"бр", L"яп", L"вр", L"ор", L"яа"};

const size_t CELL_OFFSET_TO_NOT_EVEN_TEACHER_NAME = 5;
const size_t CELL_OFFSET_TO_EVEN_TEACHER_NAME = 8;
const size_t CELL_OFFSET_TO_NOT_EVEN_SUBJECT_NAME = 6;
const size_t CELL_OFFSET_TO_EVEN_SUBJECT_NAME = 7;
const size_t CELL_OFFSET_TO_NOT_EVEN_SUBJECT_TYPE = 4;
const size_t CELL_OFFSET_TO_EVEN_SUBJECT_TYPE = 9;
const size_t CELL_OFFSET_TO_NOT_EVEN_AUDITORIUM = 3;
const size_t CELL_OFFSET_TO_EVEN_AUDITORIUM = 10;
const size_t CELL_OFFSET_TO_NOT_EVEN_TIME = 2;
const size_t CELL_OFFSET_TO_EVEN_TIME = 11;

class TableWeekMarkup {
public:
	explicit TableWeekMarkup(fs::path table_path) : table_path{ table_path } {
		std::fill(days_cells, days_cells+app::WORK_DAYS_COUNT, std::pair<int,int>(-1,-1));
	}

	TableWeekMarkup(fs::path table_path, std::pair<int, int>days_cells[WORK_DAYS_COUNT]) : table_path{ table_path } {
		for (int day_cell_idx = 0; day_cell_idx < WORK_DAYS_COUNT; ++day_cell_idx) {
			this->days_cells[day_cell_idx] = days_cells[day_cell_idx];
		}
	}

	const auto GetTablePath() const {
		return table_path;
	}

	const auto GetDaysCells() const {
		return days_cells;
	}
private:
	fs::path table_path;
	std::pair<int, int> days_cells[WORK_DAYS_COUNT];
};

TableWeekMarkup MarkupTable(std::string_view input_table_file) {
	// ##### FIX THIS (COMPLETE CONVERTER CLASS) ##### //
	default_converter converter;
	// ############################################### //

	fs::path input_file_path{input_table_file};

	if (fs::exists(input_file_path)) {
		rapidcsv::Document doc(input_file_path.string(), rapidcsv::LabelParams(-1,-1), rapidcsv::SeparatorParams(';'));

		std::pair<int, int> days_cells[WORK_DAYS_COUNT];
		std::fill(days_cells, days_cells+WORK_DAYS_COUNT, std::pair<int,int>(-1, -1));

		unsigned curr_day = 0;
		int column_idx = 0;

		for (int row_idx = 0; row_idx < doc.GetRowCount() && curr_day < WORK_DAYS_COUNT; ++row_idx) {
			if (curr_day == 0) {
				for (column_idx = 0; column_idx < doc.GetColumnCount(); ++column_idx) {

					std::string cell_value = doc.GetCell<std::string>(column_idx, row_idx);
					// ##### FIX THIS (TROUBLE WITH WSTRING AFTER UPPERCASE CONVERTATION) ##### //
					//std::transform(cell_value.begin(), cell_value.end(),
								   //cell_value.begin(),
								   //[](unsigned char c) { return std::toupper(c); });
					std::wstring converted_string = converter.from_bytes(cell_value);

					if (converted_string == DAYS_NAMES[0]) {
						days_cells[curr_day] = {column_idx, row_idx};
						++curr_day;
						break;
					}
				}
			} else {
				std::string cell_value = doc.GetCell<std::string>(column_idx, row_idx);
				// ##### FIX THIS (TROUBLE WITH WSTRING AFTER UPPERCASE CONVERTATION) ##### //
				/*std::transform(cell_value.begin(), cell_value.end(),
							   cell_value.begin(),
							   [](unsigned char c) { return std::toupper(c); });*/

				if (converter.from_bytes(cell_value) == DAYS_NAMES[curr_day]) {
					days_cells[curr_day] = {column_idx, row_idx};
					++curr_day;
				}
			}
		}

		doc.Clear();

		TableWeekMarkup tw_markup{input_file_path, days_cells};
		return tw_markup;
	} else {
		throw std::exception("File doesn't exist");
	}
}

std::vector<core::TeacherLessonInfo> GetTeacherLessons(std::wstring_view teacher_name, const TableWeekMarkup & week_markup) {
	if (fs::exists(week_markup.GetTablePath())) {
		rapidcsv::Document doc;
		doc.Load(week_markup.GetTablePath().string(), rapidcsv::LabelParams(-1, -1), rapidcsv::SeparatorParams(';'));

		default_converter converter;

		std::vector<core::TeacherLessonInfo> teacher_lessons;

		for (int day_idx = 0; day_idx < WORK_DAYS_COUNT; ++day_idx) {
			for (int half_pair_idx = 0; half_pair_idx < (2 * DAY_PAIRS_COUNT); ++half_pair_idx) {
				int week_day_column = week_markup.GetDaysCells()[day_idx].first;
				int week_day_row = week_markup.GetDaysCells()[day_idx].second;

				std::wstring not_even_teacher_name = converter.from_bytes(
					doc.GetCell<std::string>(week_day_column + CELL_OFFSET_TO_NOT_EVEN_TEACHER_NAME, week_day_row + half_pair_idx)
				);

				std::wstring even_teacher_name = converter.from_bytes(
					doc.GetCell<std::string>(week_day_column + CELL_OFFSET_TO_EVEN_TEACHER_NAME, week_day_row + half_pair_idx)
				);

				if (not_even_teacher_name.find(teacher_name) != std::wstring::npos) {
//	*	*	*	*	Get not even teacher's subject info
					std::wstring subject_name = converter.from_bytes(
						doc.GetCell<std::string>(week_day_column + CELL_OFFSET_TO_NOT_EVEN_SUBJECT_NAME, week_day_row + half_pair_idx)
					);
					std::wstring subject_type = converter.from_bytes(
						doc.GetCell<std::string>(week_day_column + CELL_OFFSET_TO_NOT_EVEN_SUBJECT_TYPE, week_day_row + half_pair_idx)
					);
					unsigned day = day_idx;
					std::wstring time = converter.from_bytes(
						doc.GetCell<std::string>(week_day_column + CELL_OFFSET_TO_NOT_EVEN_TIME, week_day_row + half_pair_idx)
					);
					bool is_even_week = false;
					unsigned auditorium_number = doc.GetCell<unsigned>(week_day_column + CELL_OFFSET_TO_NOT_EVEN_AUDITORIUM, week_day_row + half_pair_idx);

					teacher_lessons.emplace_back(not_even_teacher_name, subject_name, subject_type, day, time, is_even_week, auditorium_number);
				}
				if (even_teacher_name.find(teacher_name) != std::wstring::npos) {
//	*	*	*	*	Get even teacher's subject info
					std::wstring subject_name = converter.from_bytes(
						doc.GetCell<std::string>(week_day_column + CELL_OFFSET_TO_EVEN_SUBJECT_NAME, week_day_row + half_pair_idx)
					);
					std::wstring subject_type = converter.from_bytes(
						doc.GetCell<std::string>(week_day_column + CELL_OFFSET_TO_EVEN_SUBJECT_TYPE, week_day_row + half_pair_idx)
					);
					unsigned day = day_idx;
					std::wstring time = converter.from_bytes(
						doc.GetCell<std::string>(week_day_column + CELL_OFFSET_TO_EVEN_TIME, week_day_row + half_pair_idx)
					);
					bool is_even_week = true;
					unsigned auditorium_number = doc.GetCell<unsigned>(week_day_column + CELL_OFFSET_TO_EVEN_AUDITORIUM, week_day_row + half_pair_idx);

					teacher_lessons.emplace_back(even_teacher_name, subject_name, subject_type, day, time, is_even_week, auditorium_number);
				}
			}
		}

		doc.Clear();

		return teacher_lessons;
	} else {
		throw std::exception("Couldn't find file");
	}
}

} // namespace app