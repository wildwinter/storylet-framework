// This file is part of an MIT-licensed project: see LICENSE file or README.md for details.
// Copyright (c) 2025 Ian Thomas

#ifndef TEST_UTILS_H
#define TEST_UTILS_H

#include <string>

std::string loadTestFile(const std::string& filepath);
std::string joinStrings(std::vector<std::string>& strList, std::string join);
#endif