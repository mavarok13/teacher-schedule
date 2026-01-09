cmake_minimum_required(VERSION 3.22)

project(TeacherScheduleExamples CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Xlnt CONFIG REQUIRED)
find_package(OpenSSL REQUIRED)
find_package(Boost REQUIRED)
find_package(Boost REQUIRED COMPONENTS url)

### TeacherSchedule library (header-only interface)
add_library(TeacherScheduleLib INTERFACE)

target_include_directories(TeacherScheduleLib
    INTERFACE
        ${CMAKE_CURRENT_SOURCE_DIR}/../TeacherSchedule/src
        ${CMAKE_CURRENT_SOURCE_DIR}/../libs
)

target_link_libraries(TeacherScheduleLib
    INTERFACE
        xlnt::xlnt
        Boost::headers
        Boost::url
        OpenSSL::Crypto
        OpenSSL::SSL
)

add_executable (xls_convertation main.cpp)
add_executable (links_finder links_finder.cpp)

target_link_libraries(xls_convertation PRIVATE TeacherScheduleLib)
target_link_libraries(links_finder PRIVATE TeacherScheduleLib)