// This file is part of an MIT-licensed project: see LICENSE file or README.md for details.
// Copyright (c) 2025 Ian Thomas

namespace StoryletFramework.Tests;
using System.IO;

public static class TestUtils
{
    public static string LoadTestFile(string fileName)
    {
        return File.ReadAllText("../../../../../tests/" + fileName);
    }
}