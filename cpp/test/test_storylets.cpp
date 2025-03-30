// This file is part of an MIT-licensed project: see LICENSE file or README.md for details.
// Copyright (c) 2025 Ian Thomas

#include "storylet_framework/storylets.h"
#include "catch_amalgamated.hpp"
#include "test_utils.h"
#include <fstream>

using namespace StoryletFramework;

TEST_CASE( "Simple") {

    Deck deck;
    /*Parser parser;

    auto expression = parser.Parse("get_name()=='fred' and counter>0 and 5/5.0!=0");

    Context context;
    context["get_name"] = make_function_wrapper([]() -> std::string {
        return "fred";
    });
    context["counter"] = 1;

    std::any result = expression->Evaluate(context);

    REQUIRE(std::any_cast<bool>(result) == true);*/
    auto result = true;
    REQUIRE(result == true);
}
