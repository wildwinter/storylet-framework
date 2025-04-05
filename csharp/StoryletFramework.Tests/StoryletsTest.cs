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
        var deck = JsonLoader.DeckFromJson(json, new Dictionary<string, object>(), dumpEval);

        var cards = deck.DrawAndPlay(2);
        Assert.NotNull(cards[0]);
        Assert.NotNull(cards[1]);

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
    
        var card = barks.DrawAndPlaySingle();
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


            var encounter = encounters.DrawAndPlaySingle();
            context["encounter_tag"] = new Func<string, bool>(tag =>
            {
                if ( encounter==null || !encounter.Content?.ContainsKey("tags"))
                    return false;
                return encounter?.Content?["tags"] is List<string> tags && tags.Contains(tag);
            });

            Console.WriteLine($"  Encounter: \"{encounter?.Content?["title"]}\"");

            var bark = barks.DrawAndPlaySingle();
            if (bark != null)
            {
                Console.WriteLine($"  Comment: \"{bark.Content?["comment"]}\"");
            }
        }

        // First encounter - this should pull out a "start" location
        var street = streets.DrawAndPlaySingle(street => { 
            if (street.Content?.ContainsKey("tags")) {
                var tags = street.Content?["tags"];
                if (tags is List<string>) {
                    return ((List<string>)tags).Contains("start");
                }
            }
            return false;
        });
        Assert.NotNull(street);
        DoEncounter(street);

        Assert.True(street.Id == "docks" || street.Id == "market" || street.Id == "bridge");

        // Reshuffle the deck so that all streets are fair game
        var streetsDrawn = streets.Draw();

        var path = new List<string>();

        // Walk through the street deck and pull an encounter for each location
        for (int i=0;i<streetsDrawn.Count;i++)
        {
            street = streetsDrawn[i];
            street.Play();
            Assert.NotNull(street);
            path.Add(street.Id);
            DoEncounter(street);
        }

        Assert.True(path.Contains("market") || path.Contains("slums") || path.Contains("bridge"));
    }
}
