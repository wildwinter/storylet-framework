// This file is part of an MIT-licensed project: see LICENSE file or README.md for details.
// Copyright (c) 2025 Ian Thomas

namespace StoryletFramework.Tests;

public class StoryletsTest
{
    [Fact]
    public void Simple()
    {
        var dumpEval = new List<string>();

        var json = TestUtils.LoadJsonFile("Streets.jsonc");
        var deck = JsonLoader.DeckFromJson(json, new Dictionary<string, object>(), true, dumpEval);

        var card = deck.Draw();
        Assert.NotNull(card);

        card = deck.Draw();
        Assert.NotNull(card);

        Console.WriteLine(string.Join('\n', dumpEval));
    }

    [Fact]
    public void Barks()
    {
        // Define the context as a Dictionary<string, object>
        var context = new Dictionary<string, object>
        {
            { "street_id", "" },
            { "street_wealth", 1 },
            { "encounter_tag", new Func<string, bool>(tag => false) }
        };
    
        // Load the JSON file and create a Deck
        var json = TestUtils.LoadJsonFile("Barks.jsonc");
        var barks = JsonLoader.DeckFromJson(json, context);
    
        // Uncomment the following line to debug the draw pile
        // Console.WriteLine(barks.DumpDrawPile());
    
        var card = barks.Draw();
        Assert.NotNull(card);
    }

    [Fact]
    public void StreetSystem()
    {
        // Define the context as a Dictionary<string, object>
        var context = new Dictionary<string, object>
        {
            { "street_id", "" },
            { "street_wealth", 0 },
            { "street_tag", new Func<string, bool>(tag => false) },
            { "encounter_tag", new Func<string, bool>(tag => false) }
        };

        // Load the JSON files and create Decks
        var streets = JsonLoader.DeckFromJson(TestUtils.LoadJsonFile("Streets.jsonc"), context);
        var encounters = JsonLoader.DeckFromJson(TestUtils.LoadJsonFile("Encounters.jsonc"), context);
        var barks = JsonLoader.DeckFromJson(TestUtils.LoadJsonFile("Barks.jsonc"), context);

        // Define a method to set the current street
        void SetStreet(Storylet street)
        {
            context["street_id"] = street.Id;
            context["street_wealth"] = street.Content?["wealth"] ?? 0;
            context["street_tag"] = new Func<string, bool>(tag => {
                if (street.Content?.ContainsKey("tags")) {
                var tags = street.Content?["tags"];
                    if (tags is List<string>) {
                        return ((List<string>)tags).Contains(tag);
                    }
                }
                return false;
                });
            Console.WriteLine($"Location: \"{street.Content?["title"]}\"");
        }

        // Define a method to handle an encounter
        void DoEncounter(Storylet street)
        {
            SetStreet(street);

            // Shuffle the encounters deck to only include relevant cards
            encounters.Reshuffle();
            // Uncomment the following line to debug the draw pile
            // Console.WriteLine(encounters.DumpDrawPile());

            var encounter = encounters.Draw();
            context["encounter_tag"] = new Func<string, bool>(tag =>
            {
                if ( encounter==null || !encounter.Content?.ContainsKey("tags"))
                    return false;
                return encounter?.Content?["tags"] is List<string> tags && tags.Contains(tag);
            });

            Console.WriteLine($"  Encounter: \"{encounter?.Content?["title"]}\"");

            barks.Reshuffle();
            // Uncomment the following line to debug the draw pile
            // Console.WriteLine(barks.DumpDrawPile());

            var bark = barks.Draw();
            if (bark != null)
            {
                Console.WriteLine($"  Comment: \"{bark.Content?["comment"]}\"");
            }
        }

        // First encounter - this should pull out a "start" location
        streets.Reshuffle(street => { 
            if (street.Content?.ContainsKey("tags")) {
                var tags = street.Content?["tags"];
                if (tags is List<string>) {
                    return ((List<string>)tags).Contains("start");
                }
            }
            return false;
        });
        var street = streets.Draw();
        Assert.NotNull(street);
        DoEncounter(street);

        Assert.True(street.Id == "docks" || street.Id == "market" || street.Id == "bridge");

        // Reshuffle the deck so that all streets are fair game
        streets.Reshuffle();

        var path = new List<string>();

        // Walk through the street deck and pull an encounter for each location
        for (int i = 0; i < 11; i++)
        {
            street = streets.Draw();
            Assert.NotNull(street);
            path.Add(street.Id);
            DoEncounter(street);
        }

        Assert.True(path.Contains("market") || path.Contains("slums") || path.Contains("bridge"));
    }

    [Fact]
    public void AsyncReshuffleTest()
    {
        // Define the context as a Dictionary<string, object>
        var context = new Dictionary<string, object>
        {
            { "street_id", "" },
            { "street_wealth", 1 },
            { "encounter_tag", new Func<string, bool>(tag => false) }
        };
    
        // Load the JSON file and create a Deck without reshuffling
        var json = TestUtils.LoadJsonFile("Barks.jsonc");
        var barks = JsonLoader.DeckFromJson(json, context, reshuffle: false);
    
        // Perform an asynchronous reshuffle
        var dumpEval = new List<string>();
        
        barks.ReshuffleAsync(() => Console.WriteLine("Async reshuffle complete."), null, dumpEval);

        // Wait for the reshuffle to complete
        while (barks.AsyncReshuffleInProgress())
        {
            barks.Update();
        }

        // Uncomment the following line to debug the draw pile
        //Console.WriteLine(barks.DumpDrawPile());
    
        // Draw the first card and assert its ID
        var card = barks.Draw();
        Assert.NotNull(card);
        Assert.Equal("welcome", card.Id);
    
        // Draw another card and assert it is not null
        card = barks.Draw();
        Assert.NotNull(card);
    
        // Uncomment the following line to debug the evaluation steps
        // Console.WriteLine(string.Join('\n', dumpEval));
    }

        [Fact]
        public void DrawHandTest()
        {
            // Define the context as a Dictionary<string, object>
            var context = new Dictionary<string, object>
            {
                { "street_id", "" },
                { "street_wealth", 1 },
                { "encounter_tag", new Func<string, bool>(tag => true) }
            };
        
            // Load the JSON file and create a Deck
            var json = TestUtils.LoadJsonFile("Barks.jsonc");
            var deck = JsonLoader.DeckFromJson(json, context, reshuffle: true);
        
            // Draw a hand of 10 cards and assert the length is not 10
            var drawn = deck.DrawHand(10);
            Assert.NotEqual(10, drawn.Count);
        
            // Reset the deck and draw a hand of 10 cards with reshuffling
            deck.Reset();
            drawn = deck.DrawHand(10, reshuffleIfNeeded: true);
            Assert.Equal(10, drawn.Count);
            Assert.Equal("welcome", drawn[0].Id);
        
            // Uncomment the following lines for debugging
            // for (int i = 0; i < drawn.Count; i++)
            // {
            //     Console.WriteLine($"Card {i}: {drawn[i].Id}");
            // }
        
            // Uncomment to debug the evaluation steps
            // Console.WriteLine(string.Join('\n', dumpEval));
        }
}
