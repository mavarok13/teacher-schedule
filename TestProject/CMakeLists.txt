cmake_minimum_required(VERSION 3.22)

project(TestProject CXX)

# Настройки компиляции
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Conan-пакеты
find_package(Xlnt REQUIRED CONFIG)
find_package(Boost REQUIRED)
find_package(OpenSSL REQUIRED)

# Исполняемый файл
add_executable(app
    main.cpp
    ../TeacherSchedule/src/app.hpp
    ../TeacherSchedule/src/core.hpp
    ../TeacherSchedule/src/excel_utils.hpp
    ../TeacherSchedule/src/file_loader.hpp
    ../TeacherSchedule/src/http_client.hpp
    ../TeacherSchedule/src/string_converter.hpp
)

add_executable(testgethtml
    test_get_html.cpp
    ../TeacherSchedule/src/app.hpp
    ../TeacherSchedule/src/core.hpp
    ../TeacherSchedule/src/excel_utils.hpp
    ../TeacherSchedule/src/file_loader.hpp
    ../TeacherSchedule/src/http_client.hpp
    ../TeacherSchedule/src/string_converter.hpp
)

add_executable(testconvertcsv
    testconvertcsv.cpp
    ../TeacherSchedule/src/app.hpp
    ../TeacherSchedule/src/core.hpp
    ../TeacherSchedule/src/excel_utils.hpp
    ../TeacherSchedule/src/file_loader.hpp
    ../TeacherSchedule/src/http_client.hpp
    ../TeacherSchedule/src/string_converter.hpp
)

# Линковка
target_link_libraries(app
    PRIVATE
        xlnt::xlnt
        Boost::headers
        Boost::url
        OpenSSL::SSL
        OpenSSL::Crypto
)

target_link_libraries(testgethtml
    PRIVATE
        xlnt::xlnt
        Boost::headers
        Boost::url
        OpenSSL::SSL
        OpenSSL::Crypto
)

target_link_libraries(testconvertcsv
    PRIVATE
        xlnt::xlnt
        Boost::headers
        Boost::url
        OpenSSL::SSL
        OpenSSL::Crypto
)

# Твой кастомный include
target_include_directories(app PRIVATE
    /home/libs/rapidcsv
)

target_include_directories(testconvertcsv PRIVATE
    /home/libs/rapidcsv
)
