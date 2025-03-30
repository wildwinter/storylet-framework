// This file is part of an MIT-licensed project: see LICENSE file or README.md for details.
// Copyright (c) 2025 Ian Thomas

using StoryletFramework;

namespace StoryletFramework.Tests;

public class StoryletsTest
{
    [Fact]
    public void Simple()
    {
        var dumpEval = new List<string>();

        var json = TestUtils.LoadJsonFile("Streets.jsonc");
        var deck = Deck.FromJson(json, new Dictionary<string, object>(), true, dumpEval);

        var card = deck.Draw();
        Assert.NotNull(card);

        card = deck.Draw();
        Assert.NotNull(card);

        Console.WriteLine(string.Join('\n', dumpEval));
    }
}
