// This file is part of an MIT-licensed project: see LICENSE file or README.md for details.
// Copyright (c) 2025 Ian Thomas

namespace FountainTools.Tests;
using System.IO;
using ExpressionParser;

public class ParserTest
{
    private string loadTestFile(string fileName) {
        return File.ReadAllText("../../../../../tests/"+fileName);
    }

    [Fact]
    public void Simple()
    {
        var parser = new Parser();
        var expression = parser.Parse("get_name()=='fred' and counter>0 and 5/5.0!=0");

        var context = new Dictionary<string, object>
        {
            { "get_name", new Func<string>(() => "fred") },
            { "counter", 1 }
        };

        var result = expression.Evaluate(context);

        Assert.Equal(true, result);
    }

    [Fact]
    public void MatchOutput()
    {
        string source = loadTestFile("Parse.txt");

        string[] lines = source.Split(new[] { "\r\n", "\n", "\r" }, StringSplitOptions.None);

        var context = new Dictionary<string, object>
        {
            { "C", 15 },
            { "D", false },
            { "get_name", new Func<string>(() => "fred") },
            { "end_func", new Func<bool>(() => true) },
            { "whisky", new Func<string, double, string>((id, n) => ((int)n).ToString() + "whisky_" + id) },
            { "counter", 1 }
        };

        var parser = new Parser();

        var processedLines = new List<string>();
        foreach (var line in lines)
        {
            if (line.StartsWith("//"))
            {
                processedLines.Add(line);
                continue;
            }

            processedLines.Add($"\"{line}\"");
            try
            {
                var node = parser.Parse(line);
                processedLines.Add(node.DumpStructure());

                var dumpEval = new List<string>();
                node.Evaluate(context, dumpEval);
                processedLines.Add(string.Join("\n", dumpEval));
            }
            catch (Exception e)
            {
                processedLines.Add(e.Message);
            }
            processedLines.Add("");
        }

        string output = string.Join("\n", processedLines);
                
        //Console.WriteLine(output);

        string match = loadTestFile("Parse-Output.txt");
        Assert.Equal(match, output);
    }
}
