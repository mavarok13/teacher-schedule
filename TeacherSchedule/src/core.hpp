#pragma once

#include <string>

namespace core {

class TeacherLessonInfo {
public:
	TeacherLessonInfo(std::wstring_view teacher_name, std::wstring_view subject_name, std::wstring_view subject_type, unsigned day, std::wstring_view time, bool is_even_week, unsigned auditorium_number) :
		teacher_name{ teacher_name }, subject_name{ subject_name }, subject_type{ subject_type }, day{ day }, time{ time }, is_even_week{ is_even_week }, auditorium_number{ auditorium_number } {
	}

	std::wstring GetTeacherName() const {
		return teacher_name;
	}

	std::wstring GetSubjectName() const {
		return subject_name;
	}

	std::wstring GetSubjectType() const {
		return subject_type;
	}

	unsigned GetDay() const {
		return day;
	}

	std::wstring GetTime() const {
		return time;
	}

	bool IsEvenWeek() const {
		return is_even_week;
	}

	unsigned GetAuditorium() const {
		return auditorium_number;
	}
private:
	std::wstring teacher_name;
	std::wstring subject_name;
	std::wstring subject_type;
	unsigned day;
	std::wstring time;
	bool is_even_week = false;
	unsigned auditorium_number;
};

} // namespace core