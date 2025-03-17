// This file is part of an MIT-licensed project: see LICENSE file or README.md for details.
// Copyright (c) 2025 Ian Thomas

namespace FountainTools.Tests;
using System.IO;
using ExpressionParser;

public class WriterTest
{
    private string loadTestFile(string fileName) {
        return File.ReadAllText("../../../../../tests/"+fileName);
    }

    [Fact]
    public void Simple()
    {
        var parser = new Parser();
        var expression = parser.Parse("get_name()=='fred' and counter>0 and 5/5.0!=0");
        
        var result = expression.Write();
        Assert.Equal("get_name() == 'fred' and counter > 0 and 5 / 5 != 0", result);
        Writer.StringFormat = Writer.STRING_FORMAT.DOUBLEQUOTE;
        result = expression.Write();
        Assert.Equal("get_name() == \"fred\" and counter > 0 and 5 / 5 != 0", result);
        Writer.StringFormat = Writer.STRING_FORMAT.ESCAPED_DOUBLEQUOTE;
        result = expression.Write();
        Assert.Equal("get_name() == \\\"fred\\\" and counter > 0 and 5 / 5 != 0", result);
        Writer.StringFormat = Writer.STRING_FORMAT.ESCAPED_SINGLEQUOTE;
        result = expression.Write();
        Assert.Equal("get_name() == \\'fred\\' and counter > 0 and 5 / 5 != 0", result);
        Writer.StringFormat = Writer.STRING_FORMAT.SINGLEQUOTE;
    }

    [Fact]
    public void MatchOutput()
    {
        string source = loadTestFile("Writer.txt");

        string[] lines = source.Split(new[] { "\r\n", "\n", "\r" }, StringSplitOptions.None);

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
                var expression = parser.Parse(line);
                processedLines.Add(expression.Write());
            }
            catch (Exception e)
            {
                processedLines.Add(e.Message);
            }
            processedLines.Add("");
        }

        string output = string.Join("\n", processedLines);
                
        //Console.WriteLine(output);

        string match = loadTestFile("Writer-Output.txt");
        Assert.Equal(match, output);
    }
}
