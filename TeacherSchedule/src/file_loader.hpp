#pragma once

#include <fstream>

namespace file_loader {

std::string LoadFileContent(const std::string & file_path) {
    std::ifstream file_stream(file_path, std::ios::in | std::ios::binary);
    if (!file_stream) {
        throw std::runtime_error("Could not open file: " + file_path);
    }

    std::string content;
    file_stream.seekg(0, std::ios::end);
    content.resize(file_stream.tellg());
    file_stream.seekg(0, std::ios::beg);
    file_stream.read(&content[0], content.size());
    file_stream.close();

    return content;
}

} // namespace file_loader