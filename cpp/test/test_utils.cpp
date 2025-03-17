// This file is part of an MIT-licensed project: see LICENSE file or README.md for details.
// Copyright (c) 2025 Ian Thomas

#include "test_utils.h"
#include <filesystem>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <iostream>
#include <locale>

std::string loadTestFile(const std::string& filepath) {
    std::string path = std::filesystem::absolute("../../tests/" + filepath).string();
    std::ifstream file(path, std::ios::in); // Open file for reading
    if (!file.is_open()) {
        throw std::runtime_error("Failed to open file: " + path);
    }

    file.imbue(std::locale("en_US.UTF-8"));

    std::ostringstream content;
    content << file.rdbuf(); // Read the entire file content
    return content.str();
}

std::string joinStrings(std::vector<std::string>& strList, std::string join) {
    std::ostringstream oss;
    for (size_t i = 0; i < strList.size(); i++) {
        if (i > 0)
            oss << join;
        oss << strList[i];
    }
    return oss.str();
}