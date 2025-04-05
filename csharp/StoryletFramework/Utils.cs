/*
 * This file is part of an MIT-licensed project: see LICENSE file or README.md for details.
 * Copyright (c) 2025 Ian Thomas
 */

namespace StoryletFramework;

public static class Utils
{
    // Shuffle an array in place
    public static void ShuffleArray<T>(List<T> array)
    {
        Random random = new Random();
        for (int i = array.Count - 1; i > 0; i--)
        {
            int j = random.Next(i + 1);
            (array[i], array[j]) = (array[j], array[i]);
        }
    }

    public static Dictionary<string, object> CopyObject(Dictionary<string, object> original)
    {
        return new Dictionary<string, object>(original);
    }

    public static void UpdateObject(Dictionary<string, object> original, Dictionary<string, object> additions)
    {
        foreach (var kvp in additions)
        {
            original[kvp.Key] = kvp.Value;
        }
    }
}
