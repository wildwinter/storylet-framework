// This file is part of an MIT-licensed project: see LICENSE file or README.md for details.
// Copyright (c) 2025 Ian Thomas

#include "storylet_framework/json_loader.h"
#include "catch_amalgamated.hpp"
#include "test_utils.h"
#include <fstream>
#include <iostream>

using namespace StoryletFramework;
using namespace StoryletFrameworkTest;

TEST_CASE( "Simple") {

    DumpEval dumpEval;

    nlohmann::json json = loadJsonFile("Streets.jsonc");
    StoryletFramework::Context context;
    std::shared_ptr<Deck> deck = DeckFromJson(json, &context, &dumpEval);
    
    auto cards = deck->DrawAndPlay(2);
    REQUIRE(cards[0] != nullptr);
    REQUIRE(cards[1] != nullptr);
    
    for (const auto& line : dumpEval) {
        std::cout << line << '\n';
    }
}

TEST_CASE("Barks") {
    // Define the context
    StoryletFramework::Context context;
    context["street_id"] = "";
    context["street_wealth"] = 1;
    context["encounter_tag"] = ExpressionParser::make_function_wrapper([](const std::string& tag) { return false; });

    // Load the JSON file and create a Deck
    nlohmann::json json = loadJsonFile("Barks.jsonc");
    std::shared_ptr<Deck> barks = DeckFromJson(json, &context);

    // Draw a card and assert it's not null
    auto card = barks->DrawAndPlaySingle();
    REQUIRE(card != nullptr);
}

TEST_CASE("StreetSystem") {
    // Define the context
    StoryletFramework::Context context;
    context["street_id"] = "";
    context["street_wealth"] = 0;
    context["street_tag"] = ExpressionParser::make_function_wrapper([](const std::string& tag) { return false; });
    context["encounter_tag"] = ExpressionParser::make_function_wrapper([](const std::string& tag) { return false; });

    // Load the JSON files and create Decks
    nlohmann::json streetsJson = loadJsonFile("Streets.jsonc");
    nlohmann::json encountersJson = loadJsonFile("Encounters.jsonc");
    nlohmann::json barksJson = loadJsonFile("Barks.jsonc");

    std::shared_ptr<Deck> streets = DeckFromJson(streetsJson, &context);
    std::shared_ptr<Deck> encounters = DeckFromJson(encountersJson, &context);
    std::shared_ptr<Deck> barks = DeckFromJson(barksJson, &context);

    // Define a method to set the current street
    auto SetStreet = [&](const Storylet& street) {
        nlohmann::json content = ExtractJsonFromAny(street.content);
        context["street_id"] = street.id;
        context["street_wealth"] = content["wealth"].get<int>();
        context["street_tag"] = ExpressionParser::make_function_wrapper([&](const std::string& tag) {
            if (content.contains("tags")) {
                const auto& tags = content["tags"];
                if (tags.is_array()) {
                    return std::find(tags.begin(), tags.end(), tag) != tags.end();
                }
            }
            return false;
        });
        std::cout << "Location: \"" << content["title"] << "\"" << std::endl;
    };

    // Define a method to handle an encounter
    auto DoEncounter = [&](const Storylet& street) {
        SetStreet(street);

        auto encounter = encounters->DrawAndPlaySingle();
        nlohmann::json content = ExtractJsonFromAny(encounter->content);

        context["encounter_tag"] = ExpressionParser::make_function_wrapper([&](const std::string& tag) {    

            if (!encounter || !content.contains("tags")) {
                return false;
            }
            const auto& tags = content["tags"];
            return std::find(tags.begin(), tags.end(), tag) != tags.end();
        });

        std::cout << "  Encounter: \"" << (encounter ? content["title"] : "None") << "\"" << std::endl;

        auto bark = barks->DrawAndPlaySingle();
        if (bark) {
            nlohmann::json content = ExtractJsonFromAny(bark->content);
            std::cout << "  Comment: \"" << content["comment"] << "\"" << std::endl;
        }
    };

    // First encounter - this should pull out a "start" location
    auto street = streets->DrawAndPlaySingle([](const Storylet& street) {
        nlohmann::json content = ExtractJsonFromAny(street.content);
        if (content.contains("tags")) {
            const auto& tags = content["tags"];
            if (tags.is_array()) {
                return std::find(tags.begin(), tags.end(), "start") != tags.end();
            }
        }
        return false;
    });

    REQUIRE(street != nullptr);
    DoEncounter(*street);

    REQUIRE((street->id == "docks" || street->id == "market" || street->id == "bridge"));

    // Reshuffle the deck so that all streets are fair game
    auto streetsDrawn = streets->Draw();

    std::vector<std::string> path;

    // Walk through the street deck and pull an encounter for each location
    for (int i = 0; i < streetsDrawn.size(); i++) {
        street = streetsDrawn[i];
        REQUIRE(street != nullptr);
        street->Play();
        path.push_back(street->id);
        DoEncounter(*street);
    }

    REQUIRE((std::find(path.begin(), path.end(), "market") != path.end() ||
            std::find(path.begin(), path.end(), "slums") != path.end() ||
            std::find(path.begin(), path.end(), "bridge") != path.end()));
}