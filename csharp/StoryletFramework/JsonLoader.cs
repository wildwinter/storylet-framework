/*
 * This file is part of an MIT-licensed project: see LICENSE file or README.md for details.
 * Copyright (c) 2025 Ian Thomas
 */

using System.Text.Json;

namespace StoryletFramework;

using ParsedJson = Dictionary<string, object>;
using KeyMap = Dictionary<string, object>;

public static class JsonLoader
{
 // Parse a Storylet from JSON-like data
    public static Storylet StoryletFromJson(ParsedJson json, KeyMap defaults)
    {
        if (!json.ContainsKey("id"))
            throw new ArgumentException("No 'id' property in the storylet JSON.");

        var config = new KeyMap(defaults);
        Utils.UpdateObject(config, json);

        var storylet = new Storylet(config["id"].ToString()!);

        if (config.ContainsKey("redraw"))
        {
            var val = config["redraw"].ToString();
            if (val == "always")
                storylet.Redraw = Storylet.REDRAW_ALWAYS;
            else if (val == "never")
                storylet.Redraw = Storylet.REDRAW_NEVER;
            else
                storylet.Redraw = int.Parse(val!);
        }

        if (config.ContainsKey("condition"))
        {
            storylet.SetCondition(config["condition"].ToString());
        }

        if (config.ContainsKey("priority"))
        {                    
            storylet.SetPriority(config["priority"]);
        }

        if (config.ContainsKey("updateOnPlayed"))
        {
            storylet.UpdateOnPlayed = (KeyMap)config["updateOnPlayed"];
        }

        if (config.ContainsKey("content"))
        {
            storylet.Content = config["content"];
        }

        return storylet;
    }

    // Parse from JSON-like data
    public static Deck DeckFromJson(ParsedJson json, KeyMap? context = null, List<string>? dumpEval = null)
    {
        var deck = new Deck(context);
        ReadPacketFromJson(deck, json, new KeyMap(), dumpEval);
        return deck;
    }

    // Read a packet of storylets, inheriting the given defaults
    private static void ReadPacketFromJson(Deck deck, ParsedJson jsonObj, KeyMap defaults, List<string>? dumpEval = null)
    {
        if (jsonObj.ContainsKey("context"))
        {
            ContextUtils.InitContext(deck.Context, (KeyMap)jsonObj["context"], dumpEval);
        }

        if (jsonObj.ContainsKey("defaults"))
        {
            foreach (var kvp in (KeyMap)jsonObj["defaults"])
            {
                defaults[kvp.Key] = kvp.Value;
            }
        }

        if (jsonObj.ContainsKey("storylets"))
        {
            var storyletList = jsonObj["storylets"] as List<ParsedJson>;
            if (storyletList == null)
                throw new ArgumentException("Storylets property is not a list.");
            ReadStoryletsFromJson(deck, storyletList, Utils.CopyObject(defaults), dumpEval);
        }
    }

    // Read an array of storylets, inheriting the given defaults
    private static void ReadStoryletsFromJson(Deck deck, List<ParsedJson> jsonList, Dictionary<string, object> defaults, List<string>? dumpEval = null)
    {
        foreach (var item in jsonList)
        {
            // Is this a storylet or a packet?
            if (item.ContainsKey("storylets") || item.ContainsKey("defaults") || item.ContainsKey("context"))
            {
                ReadPacketFromJson(deck, item, defaults, dumpEval);
                continue;
            }

            // Read as a storylet
            if (!item.ContainsKey("id"))
                throw new ArgumentException("JSON item is not a storylet or packet.");

            var storylet = StoryletFromJson(item, defaults);
            deck.AddStorylet(storylet);
            dumpEval?.Add($"Added storylet '{storylet.Id}'");
        }
    }

    public static ParsedJson TextToJson(string text)
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
                return ConvertJsonArray(element);

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

    private static object ConvertJsonArray(JsonElement element)
    {
        var items = new List<object>();
        foreach (var item in element.EnumerateArray())
        {
            items.Add(ConvertJsonElement(item));
        }

        // Determine the type of the first element to return a more specific list type
        if (items.Count > 0)
        {
            var firstItem = items[0];
            if (firstItem is string)
                return items.Cast<string>().ToList();
            if (firstItem is int)
                return items.Cast<int>().ToList();
            if (firstItem is double)
                return items.Cast<double>().ToList();
            if (firstItem is ParsedJson)
                return items.Cast<ParsedJson>().ToList();
        }

        // Default to List<object> if no specific type can be determined
        return items;
    }

    private static ParsedJson ConvertJsonElementToDictionary(JsonElement element)
    {
        var dictionary = new ParsedJson();
        foreach (var property in element.EnumerateObject())
        {
            dictionary[property.Name] = ConvertJsonElement(property.Value);
        }
        return dictionary;
    }
}
