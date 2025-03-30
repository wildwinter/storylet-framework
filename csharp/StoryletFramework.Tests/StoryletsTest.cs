// This file is part of an MIT-licensed project: see LICENSE file or README.md for details.
// Copyright (c) 2025 Ian Thomas

namespace StoryletFramework.Tests;
using StoryletFramework;

public class StoryletsTest
{
    [Fact]
    public void Simple()
    {
        var deck = new Deck();

        string testFileContent = TestUtils.LoadTestFile("Barks.jsonc");

        var context = new Dictionary<string, object>
        {
            { "get_name", new Func<string>(() => "fred") },
            { "counter", 1 }
        };

        //var result = expression.Evaluate(context);

//        Assert.Equal(true, result);
    }
}
