/*
 * This file is part of an MIT-licensed project: see LICENSE file or README.md for details.
 * Copyright (c) 2025 Ian Thomas
 */

#ifndef STORYLETS_H
#define STORYLETS_H

#include <string>
#include <unordered_map>
#include <any>
#include <vector>
#include "expression_parser/parser.h"
#include "utils.h"
#include "context.h"

namespace StoryletFramework {

    const int REDRAW_ALWAYS = 0;
    const int REDRAW_NEVER = -1;

    class Storylet
    {
    public:
        std::string id; // Unique ID of the storylet
        std::any content; // Application-defined content
        int redraw = REDRAW_ALWAYS; // Redraw setting
        KeyedMap updateOnDrawn; // Updates to context

    private:
        std::shared_ptr<ExpressionParser::ExpressionNode> _condition; // Precompiled condition
        std::any _priority = 0; // Priority (absolute value or expression)
        int _nextDraw = 0; // The next draw this should be available

    public:
        // Constructor
        explicit Storylet(const std::string& id);

        // Reset the redraw counter
        void Reset();

        // Set condition as a string, which will be precompiled.
        void SetCondition(const std::string& text);

        // Evaluate condition using the current context. Returns true if no condition is set.
        bool CheckCondition(const Context& context, DumpEval* dumpEval = nullptr) const;

        // Set priority to a fixed number or a precompiled expression
        void SetPriority(int num);
        void SetPriority(std::string expression);

        // Evaluate priority using the current context
        int CalcCurrentPriority(const Context& context, bool useSpecificity = true, DumpEval* dumpEval = nullptr) const;

        // Check if the storylet is available to draw according to its redraw rules
        bool CanDraw(int currentDraw) const;

        // Call when actually drawn - updates the redraw counter
        void Drawn(int currentDraw);
    };

    class Deck
    {
    private:
        std::unordered_map<std::string, std::shared_ptr<Storylet>> _all; // Store shared_ptr to Storylet
        std::vector<std::shared_ptr<Storylet>> _drawPile; // Store shared_ptr to Storylet
        int _currentDraw = 0;
    
        struct ReshuffleState
        {
            std::function<void()> callback = nullptr;
            std::vector<std::shared_ptr<Storylet>> toProcess; // Store shared_ptr to Storylet
            std::unordered_map<int, std::vector<std::shared_ptr<Storylet>>> priorityMap; // Store shared_ptr to Storylet
            std::function<bool(const Storylet&)> filter = nullptr;
            DumpEval* dumpEval = nullptr;
        } _reshuffleState;
    
        void _reshufflePrep(std::function<bool(const Storylet&)> filter, DumpEval* dumpEval = nullptr);
        void _reshuffleDoChunk(size_t count);
        void _reshuffleFinalise();
    
    public:
        explicit Deck();
        explicit Deck(Context& context);
        void Reset();
        void Reshuffle(std::function<bool(const Storylet&)> filter = nullptr, DumpEval* dumpEval = nullptr);
        void ReshuffleAsync(std::function<void()> callback, std::function<bool(const Storylet&)> filter = nullptr, DumpEval* dumpEval = nullptr);
        bool AsyncReshuffleInProgress() const;
        void Update();
        std::shared_ptr<Storylet> GetStorylet(const std::string& id) const;
        void AddStorylet(std::shared_ptr<Storylet> storylet);
        std::string DumpDrawPile() const;
        std::shared_ptr<Storylet> Draw();
        std::vector<std::shared_ptr<Storylet>> DrawHand(int count, bool reshuffleIfNeeded = false);

        std::shared_ptr<Context> context;
        bool useSpecificity = false;
        int asyncReshuffleCount = 10;

    };

} // namespace StoryletFramework

#endif // STORYLETS_H