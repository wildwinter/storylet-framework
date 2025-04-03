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
    };
}

#endif // UTILS_H