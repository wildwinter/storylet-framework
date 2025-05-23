#ifndef JSON_UTILS_H
#define JSON_UTILS_H

#include "storylet_framework/storylets.h"
#include <json.hpp>
#include <any>
#include <string>
#include <stdexcept>
#include <typeinfo>

namespace nlohmann
{
    template <>
    struct adl_serializer<std::any>
    {
        // Serialize std::any to JSON
        static void to_json(json& j, const std::any& value)
        {
            if (value.type() == typeid(int))
            {
                j = std::any_cast<int>(value);
            }
            else if (value.type() == typeid(double))
            {
                j = std::any_cast<double>(value);
            }
            else if (value.type() == typeid(bool))
            {
                j = std::any_cast<bool>(value);
            }
            else if (value.type() == typeid(std::string))
            {
                j = std::any_cast<std::string>(value);
            }
            else
            {
                throw std::runtime_error("Unsupported type for std::any serialization");
            }
        }

        // Deserialize JSON to std::any
        static void from_json(const json& j, std::any& value)
        {
            if (j.is_number_integer())
            {
                value = j.get<int>();
            }
            else if (j.is_number_float())
            {
                value = j.get<double>();
            }
            else if (j.is_boolean())
            {
                value = j.get<bool>();
            }
            else if (j.is_string())
            {
                value = j.get<std::string>();
            }
            else
            {
                throw std::runtime_error("Unsupported type for std::any deserialization");
            }
        }
    };
}

namespace StoryletFramework
{
    using KeyedMap = std::unordered_map<std::string, std::any>;

    std::shared_ptr<Storylet> StoryletFromJson(const nlohmann::json& json, const nlohmann::json& defaults);
    std::shared_ptr<Deck> DeckFromJson(const nlohmann::json& json, Context* context = nullptr, DumpEval* dumpEval = nullptr);
    void _readPacketFromJson(Deck& deck, const nlohmann::json& json, nlohmann::json defaults, DumpEval* dumpEval = nullptr);
    void _readStoryletsFromJson(Deck& deck, const nlohmann::json& json, nlohmann::json defaults, DumpEval* dumpEval = nullptr);

    // Utility to extract Json stored in a std::any
    nlohmann::json ExtractJsonFromAny(const std::any& value);

    // Utility function to convert nlohmann::json to KeyedMap
    KeyedMap JsonToKeyedMap(const nlohmann::json& json);
}

#endif // JSON_UTILS_H