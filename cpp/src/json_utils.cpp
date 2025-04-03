#include "storylet_framework/json_utils.h"

namespace StoryletFramework
{
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