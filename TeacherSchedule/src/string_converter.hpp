#pragma once

#include <locale>
#include <codecvt>

using default_converter = std::wstring_convert<std::codecvt_utf8<wchar_t>, wchar_t>;

//class StringConverter {
//public:
//	StringConverter& operator=(const StringConverter& converter) = delete;
//
//	template <typename T>
//	void Init(std::wstring_convert<T> converter) {
//		static StringConverter string_converter;
//	}
//
//	const StringConverter & Get() {
//		static StringConverter converter;
//	}
//private:
//	explicit StringConverter() {}
//
//
//};