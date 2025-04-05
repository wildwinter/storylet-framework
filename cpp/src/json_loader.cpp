#include "storylet_framework/json_loader.h"

namespace StoryletFramework
{
    // Parse a Storylet from JSON-like data
    std::shared_ptr<Storylet> StoryletFromJson(const nlohmann::json& json, const nlohmann::json& defaults)
    {
        if (!json.contains("id"))
        {
            throw std::invalid_argument("No 'id' property in the storylet JSON.");
        }

        nlohmann::json config = nlohmann::json::object();
        config.update(defaults);
        config.update(json);

        std::shared_ptr<Storylet> storylet = std::make_shared<Storylet>(config["id"].get<std::string>());

        if (config.contains("redraw"))
        {
            auto val = config["redraw"];
            if (val == "always")
                storylet->redraw = REDRAW_ALWAYS;
            else if (val == "never")
                storylet->redraw = REDRAW_NEVER;
            else
                storylet->redraw = val.get<int>();
        }

        if (config.contains("condition"))
        {
            storylet->SetCondition(config["condition"].get<std::string>());
        }

        if (config.contains("priority"))
        {
        auto val = config["priority"];
        if (val.is_number_integer())
            storylet->SetPriority(val.get<int>());
        else if (val.is_number_float())
            storylet->SetPriority(static_cast<int>(val.get<double>()));
        else if (val.is_string())
            storylet->SetPriority(val.get<std::string>());
        }

        if (config.contains("updateOnDrawn"))
        {
            storylet->updateOnDrawn = JsonToKeyedMap(config["updateOnDrawn"]);
        }
        if (config.contains("content"))
        {
            storylet->content = config["content"];
        }
        return storylet;
    }

    std::shared_ptr<Deck> DeckFromJson(const nlohmann::json& json, Context* context, bool reshuffle, DumpEval* dumpEval)
    {
        std::shared_ptr<Deck> deck = std::make_shared<Deck>(*context);
        _readPacketFromJson(*deck, json, nlohmann::json::object(), dumpEval);
        if (reshuffle)
            deck->Reshuffle(nullptr, dumpEval);
        return deck;
    }

    void _readPacketFromJson(Deck& deck, const nlohmann::json& json, nlohmann::json defaults, DumpEval* dumpEval)
    {
        if (json.contains("context"))
        {
            ContextUtils::InitContext(*deck.context, json["context"], dumpEval);
        }

        if (json.contains("defaults"))
        {
            defaults.update(json["defaults"]);
        }

        if (json.contains("storylets"))
        {
            _readStoryletsFromJson(deck, json["storylets"], defaults, dumpEval);
        }
    }

    void _readStoryletsFromJson(Deck& deck, const nlohmann::json& json, nlohmann::json defaults, DumpEval* dumpEval)
    {
        for (const auto& item : json)
        {
            if (item.contains("storylets") || item.contains("defaults") || item.contains("context"))
            {
                _readPacketFromJson(deck, item, defaults, dumpEval);
                continue;
            }

            if (!item.contains("id"))
                throw std::invalid_argument("Json item is not a storylet or packet");

            std::shared_ptr<Storylet> storylet = StoryletFromJson(item, defaults);
            deck.AddStorylet(storylet);

            if (dumpEval)
                dumpEval->push_back("Added storylet '" + storylet->id + "'");
            
        }
    }

    nlohmann::json ExtractJsonFromAny(const std::any& value)
    {
        if (value.type() == typeid(nlohmann::json))
        {
            return std::any_cast<nlohmann::json>(value);
        }
        else
        {
            throw std::runtime_error("Value is not of type nlohmann::json");
        }
    }

    // Utility function to convert nlohmann::json to KeyedMap
    KeyedMap JsonToKeyedMap(const nlohmann::json& json)
    {
        KeyedMap map;

        if (!json.is_object())
        {
            throw std::invalid_argument("JsonToKeyedMap: Input JSON is not an object");
        }

        for (auto it = json.begin(); it != json.end(); ++it)
        {
            const std::string& key = it.key();
            const nlohmann::json& value = it.value();

            if (value.is_string())
            {
                map[key] = value.get<std::string>();
            }
            else if (value.is_number_integer())
            {
                map[key] = value.get<int>();
            }
            else if (value.is_number_float())
            {
                map[key] = value.get<double>();
            }
            else if (value.is_boolean())
            {
                map[key] = value.get<bool>();
            }
            else if (value.is_object())
            {
                // Recursively convert nested JSON objects
                map[key] = JsonToKeyedMap(value);
            }
            else if (value.is_array())
            {
                // Convert JSON array to std::vector<std::any>
                std::vector<std::any> array;
                for (const auto& element : value)
                {
                    if (element.is_string())
                    {
                        array.push_back(element.get<std::string>());
                    }
                    else if (element.is_number_integer())
                    {
                        array.push_back(element.get<int>());
                    }
                    else if (element.is_number_float())
                    {
                        array.push_back(element.get<double>());
                    }
                    else if (element.is_boolean())
                    {
                        array.push_back(element.get<bool>());
                    }
                    else if (element.is_object())
                    {
                        array.push_back(JsonToKeyedMap(element)); // Recursively convert nested objects
                    }
                    else if (element.is_array())
                    {
                        array.push_back(JsonToKeyedMap(element)); // Recursively handle nested arrays
                    }
                    else if (element.is_null())
                    {
                        array.push_back(nullptr); // Store null as nullptr
                    }
                    else
                    {
                        throw std::runtime_error("JsonToKeyedMap: Unsupported JSON value type in array");
                    }
                }
                map[key] = array;
            }
            else if (value.is_null())
            {
                map[key] = nullptr; // Store null as nullptr
            }
            else
            {
                throw std::runtime_error("JsonToKeyedMap: Unsupported JSON value type");
            }
        }
        return map;
    }
}