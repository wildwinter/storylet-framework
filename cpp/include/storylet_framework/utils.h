/*
 * This file is part of an MIT-licensed project: see LICENSE file or README.md for details.
 * Copyright (c) 2025 Ian Thomas
 */

#ifndef UTILS_H
#define UTILS_H

#include <vector>
#include <unordered_map>
#include <string>
#include <algorithm>
#include <random>
#include <any>

namespace StoryletFramework
{
    class Utils
    {
    public:
        // Shuffle a vector in place
        template <typename T>
        static void ShuffleArray(std::vector<T>& array)
        {
            std::random_device rd;
            std::mt19937 generator(rd());
            for (size_t i = array.size() - 1; i > 0; --i)
            {
                std::uniform_int_distribution<size_t> distribution(0, i);
                size_t j = distribution(generator);
                std::swap(array[i], array[j]);
            }
        }

        // Copy an unordered_map
        static std::unordered_map<std::string, std::any> CopyObject(const std::unordered_map<std::string, std::any>& original)
        {
            return std::unordered_map<std::string, std::any>(original);
        }

        // Update an unordered_map with another unordered_map
        static void UpdateObject(std::unordered_map<std::string, std::any>& original, const std::unordered_map<std::string, std::any>& additions)
        {
            for (const auto& kvp : additions)
            {
                original[kvp.first] = kvp.second;
            }
        }
    };
}

#endif // UTILS_H