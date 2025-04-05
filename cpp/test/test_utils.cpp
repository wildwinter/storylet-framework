// This file is part of an MIT-licensed project: see LICENSE file or README.md for details.
// Copyright (c) 2025 Ian Thomas

#include "test_utils.h"
#include <filesystem>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <iostream>
#include <locale>
#include <regex>

namespace StoryletFrameworkTest {

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

// Remove comments from a JSON string
std::string stripJsonComments(const std::string& jsonText) {
    // Remove block comments (/* ... */)
    std::regex blockCommentRegex(R"(/\*[\s\S]*?\*/)");
    std::string withoutBlockComments = std::regex_replace(jsonText, blockCommentRegex, "");

    // Remove line comments (// ...) - ensure it handles inline comments properly
    std::regex lineCommentRegex(R"(//[^\n\r]*)");
    std::string withoutComments = std::regex_replace(withoutBlockComments, lineCommentRegex, "");

    // Trim any trailing whitespace or newlines
    withoutComments.erase(std::remove(withoutComments.begin(), withoutComments.end(), '\r'), withoutComments.end());
    withoutComments.erase(std::remove(withoutComments.begin(), withoutComments.end(), '\n'), withoutComments.end());

    return withoutComments;
}

// Load and parse a JSON file, stripping comments
nlohmann::json loadJsonFile(const std::string& fileName) {
    std::string text = loadTestFile(fileName);
    text = stripJsonComments(text);

    return nlohmann::json::parse(text);
}

}