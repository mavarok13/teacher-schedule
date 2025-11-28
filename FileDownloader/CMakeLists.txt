cmake_minimum_required(VERSION 3.22)

project(FileDownloader CXX)

# Настройки компиляции
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Conan-пакеты
find_package(Boost REQUIRED)
find_package(Boost REQUIRED COMPONENTS url)
find_package(OpenSSL REQUIRED)

# Исполняемый файл
add_executable(app
    main.cpp
    http_client.hpp
)

# Линковка
target_link_libraries(app
    PRIVATE
        Boost::headers
        Boost::url
        OpenSSL::SSL
        OpenSSL::Crypto
)
