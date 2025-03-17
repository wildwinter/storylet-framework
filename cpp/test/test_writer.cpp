// This file is part of an MIT-licensed project: see LICENSE file or README.md for details.
// Copyright (c) 2025 Ian Thomas

#include "expression_parser/parser.h"
#include "expression_parser/writer.h"
#include "catch_amalgamated.hpp"
#include "test_utils.h"
#include <fstream>

using namespace ExpressionParser;

TEST_CASE( "SimpleWriter") {
    Parser parser;
    auto expression = parser.Parse("get_name()=='fred' and counter>0 and 5/5.0!=0");

    std::string result = expression->Write();
    REQUIRE(result == "get_name() == 'fred' and counter > 0 and 5 / 5 != 0");

    Writer::setStringFormat(STRING_FORMAT::DOUBLEQUOTE);
    result = expression->Write();
    REQUIRE(result == "get_name() == \"fred\" and counter > 0 and 5 / 5 != 0");

    Writer::setStringFormat(STRING_FORMAT::ESCAPED_DOUBLEQUOTE);
    result = expression->Write();
    REQUIRE(result == "get_name() == \\\"fred\\\" and counter > 0 and 5 / 5 != 0");

    Writer::setStringFormat(STRING_FORMAT::ESCAPED_SINGLEQUOTE);
    result = expression->Write();
    REQUIRE(result == "get_name() == \\'fred\\' and counter > 0 and 5 / 5 != 0");

    Writer::setStringFormat(STRING_FORMAT::SINGLEQUOTE);
}

TEST_CASE( "MatchOutputWriter") {
    std::string source = loadTestFile("Writer.txt");

    std::vector<std::string> lines;
    std::istringstream iss(source);
    std::string line;
    while (std::getline(iss, line)) {
        lines.push_back(line);
    }

    Parser parser;
    std::vector<std::string> processedLines;
    for (const auto &l : lines) {
        if (l.rfind("//", 0) == 0) {
            processedLines.push_back(l);
            continue;
        }

        // Append the original line wrapped in quotes.
        processedLines.push_back("\"" + l + "\"");
        try {
            auto node = parser.Parse(l);
            processedLines.push_back(node->Write());
        }
        catch (const std::exception &ex) {
            processedLines.push_back(ex.what());
        }
        processedLines.push_back("");
    }

    std::string output = joinStrings(processedLines, "\n");

    /*std::ofstream outFile("output.txt");
    if (!outFile) {
        throw std::runtime_error("Failed to open output.txt for writing.");
    }
    outFile << output;
    outFile.close();*/

    std::string match = loadTestFile("Writer-Output.txt");
    REQUIRE(match == output);
}