// This file is part of an MIT-licensed project: see LICENSE file or README.md for details.
// Copyright (c) 2025 Ian Thomas

#include "storylet_framework/storylets.h"
#include "catch_amalgamated.hpp"
#include "test_utils.h"
#include <fstream>
#include <iostream>

using namespace StoryletFramework;

TEST_CASE( "Simple") {

    DumpEval dumpEval;

    nlohmann::json json = loadJsonFile("Streets.jsonc");
    StoryletFramework::Context context;
    std::shared_ptr<Deck> deck = Deck::FromJson(json, &context, true, &dumpEval);
    
    /*auto card = deck->Draw();
    REQUIRE(card != nullptr);
    
    card = deck->Draw();
    REQUIRE(card != nullptr);
    
    for (const auto& line : dumpEval) {
        std::cout << line << '\n';
    }*/
}
