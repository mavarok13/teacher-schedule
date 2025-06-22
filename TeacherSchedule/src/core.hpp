#pragma once

#include <string>

namespace core {

class Teacher {
public:
	const std::wstring GetName() const {
		return name;
	}
private:
	std::wstring name;
};

} // namespace core