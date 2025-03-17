// This file is part of an MIT-licensed project: see LICENSE file or README.md for details.
// Copyright (c) 2025 Ian Thomas

#include "expression_parser/parser.h"
#include "catch_amalgamated.hpp"
#include "test_utils.h"
#include <fstream>

using namespace ExpressionParser;

TEST_CASE( "Simple") {

    Parser parser;

    auto expression = parser.Parse("get_name()=='fred' and counter>0 and 5/5.0!=0");

    Context context;
    context["get_name"] = make_function_wrapper([]() -> std::string {
        return "fred";
    });
    context["counter"] = 1;

    std::any result = expression->Evaluate(context);

    REQUIRE(std::any_cast<bool>(result) == true);
}

TEST_CASE( "MatchOutput") {
    std::string source = loadTestFile("Parse.txt");

    std::vector<std::string> lines;
    std::istringstream iss(source);
    std::string line;
    while (std::getline(iss, line)) {
        lines.push_back(line);
    }

    Context context;
    context["C"] = 15;
    context["D"] = false;
    context["get_name"] = make_function_wrapper([]() -> std::string {
        return "fred";
    });
    context["end_func"] = make_function_wrapper([]() -> bool {
        return true;
    });
    context["whisky"] = make_function_wrapper([](const std::string &id, double n) -> std::string {
        int inum = static_cast<int>(n);
        return std::to_string(inum) + "whisky_" + id;
    });
    context["counter"] = 1;

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
            processedLines.push_back(node->DumpStructure());

            std::vector<std::string> dumpEval;
            node->Evaluate(context, &dumpEval);

            std::string evalOutput = joinStrings(dumpEval, "\n");
            processedLines.push_back(evalOutput);
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

    std::string match = loadTestFile("Parse-Output.txt");
    REQUIRE(match == output);
}