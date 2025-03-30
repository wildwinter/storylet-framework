/*
 * This file is part of an MIT-licensed project: see LICENSE file or README.md for details.
 * Copyright (c) 2025 Ian Thomas
 */

using System;
using System.Collections.Generic;
using System.Text.Json;

namespace StoryletFramework;

public static class JsonHelpers
{
    public static Dictionary<string, object> TextToJson(string text)
    {
        var jsonElement = JsonSerializer.Deserialize<JsonElement>(text);
        if (jsonElement.ValueKind != JsonValueKind.Object)
            throw new InvalidDataException("Top-level JSON must be an object.");

        return ConvertJsonElementToDictionary(jsonElement);
    }

    private static object ConvertJsonElement(JsonElement element)
    {
        switch (element.ValueKind)
        {
            case JsonValueKind.Array:
                var list = new List<object>();
                foreach (var item in element.EnumerateArray())
                {
                    list.Add(ConvertJsonElement(item));
                }
                return list;

            case JsonValueKind.String:
                return element.GetString()!;

            case JsonValueKind.Number:
                if (element.TryGetInt32(out int intValue))
                    return intValue;
                if (element.TryGetDouble(out double doubleValue))
                    return doubleValue;
                throw new InvalidDataException("Unsupported number format.");

            case JsonValueKind.True:
            case JsonValueKind.False:
                return element.GetBoolean();

            case JsonValueKind.Null:
                return null!;

            case JsonValueKind.Object:
                return ConvertJsonElementToDictionary(element);

            default:
                throw new InvalidDataException($"Unsupported JSON value kind: {element.ValueKind}");
        }
    }

    private static Dictionary<string, object> ConvertJsonElementToDictionary(JsonElement element)
    {
        var dictionary = new Dictionary<string, object>();
        foreach (var property in element.EnumerateObject())
        {
            dictionary[property.Name] = ConvertJsonElement(property.Value);
        }
        return dictionary;
    }
}
