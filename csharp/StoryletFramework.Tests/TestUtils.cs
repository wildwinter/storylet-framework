// This file is part of an MIT-licensed project: see LICENSE file or README.md for details.
// Copyright (c) 2025 Ian Thomas

using System.IO;
using System.Text.RegularExpressions;
using StoryletFramework;

namespace StoryletFramework.Tests;

public static class TestUtils
{
    public static string LoadTestFile(string fileName)
    {
        return File.ReadAllText("../../../../../tests/" + fileName);
    }

    // Remove comments from JSON text
    public static string StripJsonComments(string jsonText)
    {
        // Remove block comments (/* ... */)
        string withoutBlockComments = Regex.Replace(jsonText, @"/\*[\s\S]*?\*/", string.Empty);
        // Remove line comments (// ...)
        string withoutComments = Regex.Replace(withoutBlockComments, @"//.*$", string.Empty, RegexOptions.Multiline);
        return withoutComments;
    }

    // Load and parse a JSON file, stripping comments
    public static Dictionary<string, object> LoadJsonFile(string fileName)
    {
        string text = LoadTestFile(fileName);
        text = StripJsonComments(text);

        return JsonHelpers.TextToJson(text);
    }
}