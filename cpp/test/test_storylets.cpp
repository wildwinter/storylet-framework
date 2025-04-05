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
    std::shared_ptr<Deck> deck = DeckFromJson(json, &context, true, &dumpEval);
    
    auto card = deck->Draw();
    REQUIRE(card != nullptr);
    
    card = deck->Draw();
    REQUIRE(card != nullptr);
    
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

    // Uncomment the following line to debug the draw pile
    std::cout << barks->DumpDrawPile() << std::endl;

    // Draw a card and assert it's not null
    auto card = barks->Draw();
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

        // Shuffle the encounters deck to only include relevant cards
        encounters->Reshuffle();
        // Uncomment the following line to debug the draw pile
        // std::cout << encounters->DumpDrawPile() << std::endl;

        auto encounter = encounters->Draw();
        nlohmann::json content = ExtractJsonFromAny(encounter->content);

        context["encounter_tag"] = ExpressionParser::make_function_wrapper([&](const std::string& tag) {    

            if (!encounter || !content.contains("tags")) {
                return false;
            }
            const auto& tags = content["tags"];
            return std::find(tags.begin(), tags.end(), tag) != tags.end();
        });

        std::cout << "  Encounter: \"" << (encounter ? content["title"] : "None") << "\"" << std::endl;

        barks->Reshuffle();
        // Uncomment the following line to debug the draw pile
        // std::cout << barks->DumpDrawPile() << std::endl;

        auto bark = barks->Draw();
        if (bark) {
            nlohmann::json content = ExtractJsonFromAny(bark->content);
            std::cout << "  Comment: \"" << content["comment"] << "\"" << std::endl;
        }
    };

    // First encounter - this should pull out a "start" location
    streets->Reshuffle([](const Storylet& street) {
        nlohmann::json content = ExtractJsonFromAny(street.content);
        if (content.contains("tags")) {
            const auto& tags = content["tags"];
            if (tags.is_array()) {
                return std::find(tags.begin(), tags.end(), "start") != tags.end();
            }
        }
        return false;
    });

    auto street = streets->Draw();
    REQUIRE(street != nullptr);
    DoEncounter(*street);

    REQUIRE((street->id == "docks" || street->id == "market" || street->id == "bridge"));

    // Reshuffle the deck so that all streets are fair game
    streets->Reshuffle();

    std::vector<std::string> path;

    // Walk through the street deck and pull an encounter for each location
    for (int i = 0; i < 11; i++) {
        street = streets->Draw();
        REQUIRE(street != nullptr);
        path.push_back(street->id);
        DoEncounter(*street);
    }

    REQUIRE((std::find(path.begin(), path.end(), "market") != path.end() ||
            std::find(path.begin(), path.end(), "slums") != path.end() ||
            std::find(path.begin(), path.end(), "bridge") != path.end()));
}

TEST_CASE("AsyncReshuffleTest") {
    // Define the context
    StoryletFramework::Context context;
    context["street_id"] = "";
    context["street_wealth"] = 1;
    context["encounter_tag"] = ExpressionParser::make_function_wrapper([](const std::string& tag) { return false; });

    // Load the JSON file and create a Deck without reshuffling
    nlohmann::json json = loadJsonFile("Barks.jsonc");
    std::shared_ptr<Deck> barks = DeckFromJson(json, &context, false);

    // Perform an asynchronous reshuffle
    DumpEval dumpEval;
    barks->ReshuffleAsync(
        []() { std::cout << "Async reshuffle complete." << std::endl; },
        nullptr,
        &dumpEval
    );

    // Wait for the reshuffle to complete
    while (barks->AsyncReshuffleInProgress()) {
        barks->Update();
    }

    // Uncomment the following line to debug the draw pile
    // std::cout << barks->DumpDrawPile() << std::endl;

    // Draw the first card and assert its ID
    auto card = barks->Draw();
    REQUIRE(card != nullptr);
    REQUIRE(card->id == "welcome");

    // Draw another card and assert it is not null
    card = barks->Draw();
    REQUIRE(card != nullptr);

    // Uncomment the following line to debug the evaluation steps
    // for (const auto& line : dumpEval) {
    //     std::cout << line << '\n';
    // }
}

TEST_CASE("DrawHandTest") {
    // Define the context
    StoryletFramework::Context context;
    context["street_id"] = "";
    context["street_wealth"] = 1;
    context["encounter_tag"] = ExpressionParser::make_function_wrapper([](const std::string& tag) { return true; });

    // Load the JSON file and create a Deck
    nlohmann::json json = loadJsonFile("Barks.jsonc");
    std::shared_ptr<Deck> deck = DeckFromJson(json, &context, true);

    // Draw a hand of 10 cards and assert the length is not 10
    auto drawn = deck->DrawHand(10);
    REQUIRE(drawn.size() != 10);

    // Reset the deck and draw a hand of 10 cards with reshuffling
    deck->Reset();
    drawn = deck->DrawHand(10, true);
    REQUIRE(drawn.size() == 10);
    REQUIRE(drawn[0]->id == "welcome");

    // Uncomment the following lines for debugging
    // for (size_t i = 0; i < drawn.size(); i++) {
    //     std::cout << "Card " << i << ": " << drawn[i]->id << std::endl;
    // }

    // Uncomment to debug the evaluation steps
    // for (const auto& line : dumpEval) {
    //     std::cout << line << '\n';
    // }
}