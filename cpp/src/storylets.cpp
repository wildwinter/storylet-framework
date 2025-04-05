/*
 * This file is part of an MIT-licensed project: see LICENSE file or README.md for details.
 * Copyright (c) 2025 Ian Thomas
 */

 #include "storylet_framework/context.h"
 #include "storylet_framework/storylets.h"
  #include "storylet_framework/utils.h"
 #include <stdexcept>
 #include <iostream>
 
 namespace StoryletFramework
 {
    static ExpressionParser::Parser expressionParser;

     // Constructor
     Storylet::Storylet(const std::string& id) : id(id) {}
 
     // Reset the redraw counter
     void Storylet::Reset()
     {
         _nextPlay = 0;
     }
 
     // Set condition as a precompiled expression
     void Storylet::SetCondition(const std::string& text)
     {
         _condition = nullptr;
         if (!text.empty())
         {
             // Assuming ExpressionParser::Parse is implemented elsewhere
             _condition = expressionParser.Parse(text);
         }
     }
 
     // Evaluate condition using the current context
     bool Storylet::CheckCondition(const Context& context, DumpEval* dumpEval) const
     {
         if (!_condition)
             return true;
 
         if (dumpEval)
         {
             dumpEval->push_back("Evaluating condition for " + id);
         }
 
         std::any result = _condition->Evaluate(context, dumpEval);
         return ExpressionParser::Utils::MakeBool(result);
     }
 
     // Set priority to a fixed number
     void Storylet::SetPriority(int num)
     {
        _priority = num;
     }

    // Set priority to a precompiled expression
    void Storylet::SetPriority(std::string expression)
    {
        _priority = expression;
    }
 
     // Evaluate priority using the current context
     int Storylet::CalcCurrentPriority(const Context& context, bool useSpecificity, std::vector<std::string>* dumpEval) const
     {
         int workingPriority = 0;
 
         if (_priority.type() == typeid(int))
         {
             workingPriority = std::any_cast<int>(_priority);
         }
         else if (_priority.type() == typeid(std::shared_ptr<ExpressionParser::ExpressionNode>))
         {
             if (dumpEval)
             {
                 dumpEval->push_back("Evaluating priority for " + id);
             }
             std::any result = std::any_cast<std::shared_ptr<ExpressionParser::ExpressionNode>>(_priority)->Evaluate(context, dumpEval);
             workingPriority = ExpressionParser::Utils::MakeNumeric(result);
         }
 
         if (useSpecificity)
         {
             workingPriority *= 100;
             if (_condition)
             {
                 workingPriority += _condition->GetSpecificity();
             }
         }
 
         return workingPriority;
     }
 
     // Check if the storylet is available to draw
     bool Storylet::CanDraw(int currentDraw) const
     {
         if (redraw == REDRAW_NEVER && _nextPlay < 0)
             return false;
         if (redraw == REDRAW_ALWAYS)
             return true;
         return currentDraw >= _nextPlay;
     }
 
     // Call when actually drawn - updates the redraw counter
    void Storylet::OnPlayed(int currentDraw, Context& context)
     {
        if (redraw == REDRAW_NEVER)
        {
            _nextPlay = -1;
        }
        else
        {
            _nextPlay = currentDraw + redraw;
        }
        if (!updateOnPlayed.empty())
        {
            ContextUtils::UpdateContext(context, updateOnPlayed);
        }
    }

    void Storylet::Play() {
        if (!_deck)
        {
            throw std::runtime_error("Storylet not part of a deck");
            
        }
        _deck->Play(*this);
    }

    Deck::Deck() {
        this->context = std::make_shared<Context>();
    }

    Deck::Deck(Context& context) {
        this->context = std::shared_ptr<Context>(&context, [](Context*) {
            // Do nothing, we don't own the context
        });
    }

    void Deck::Reset()
    {
        _currentDraw = 0;
        for (auto& [id, storylet] : _all)
        {
            storylet->Reset();
        }
    }

    std::vector<std::shared_ptr<Storylet>> Deck::Draw(int count, std::function<bool(const Storylet&)> filter, DumpEval* dumpEval)
    {
        std::unordered_map<int, std::vector<std::shared_ptr<Storylet>>> priorityMap;
        std::vector<std::shared_ptr<Storylet>> toProcess;

        for (auto& [id, storylet] : _all)
        {
            toProcess.push_back(storylet);
        }

        while (toProcess.size() > 0)
        {
            std::shared_ptr<Storylet> storylet = toProcess.front();
            toProcess.erase(toProcess.begin());

            if (!storylet->CanDraw(_currentDraw))
                continue;

            if (filter && !filter(*storylet))
                continue;

            if (!storylet->CheckCondition(*context, dumpEval))
                continue;

            int priority = storylet->CalcCurrentPriority(*context, useSpecificity, dumpEval);

            if (!priorityMap.contains(priority))
            {
                priorityMap[priority] = std::vector<std::shared_ptr<Storylet>>();
            }

            priorityMap[priority].push_back(storylet);
        }
        
        std::vector<int> sortedPriorities;
        for (const auto& [priority, _] : priorityMap)
        {
            sortedPriorities.push_back(priority);
        }
        std::sort(sortedPriorities.begin(), sortedPriorities.end(), std::greater<int>());
        std::vector<std::shared_ptr<Storylet>> drawPile;

        for (int priority : sortedPriorities)
        {
            auto& bucket = priorityMap[priority];
            Utils::ShuffleArray(bucket);
            for (std::shared_ptr<Storylet> storylet : bucket)
            {
                drawPile.push_back(storylet);
                if (count>-1 && drawPile.size() >= count)
                {
                    break;
                }
            }
        }
        return drawPile;
    }

    std::vector<std::shared_ptr<Storylet>> Deck::DrawAndPlay(int count, std::function<bool(const Storylet&)> filter, DumpEval* dumpEval) {
        std::vector<std::shared_ptr<Storylet>> drawPile = Draw(count, filter, dumpEval);
        for (auto& storylet : drawPile)
        {
            Play(*storylet);
        }
        return drawPile;
    }

    std::shared_ptr<Storylet> Deck::DrawSingle(std::function<bool(const Storylet&)> filter, DumpEval* dumpEval) {
        std::vector<std::shared_ptr<Storylet>> drawPile = Draw(1, filter, dumpEval);
        if (drawPile.size() > 0)
        {
            return drawPile[0];
        }
        return nullptr;
    }

    std::shared_ptr<Storylet> Deck::DrawAndPlaySingle(std::function<bool(const Storylet&)> filter, DumpEval* dumpEval) {
        std::vector<std::shared_ptr<Storylet>> drawPile = Draw(1, filter, dumpEval);
        if (drawPile.size() > 0)
        {
            Play(*drawPile[0]);
            return drawPile[0];
        }
        return nullptr;
    }

    std::shared_ptr<Storylet> Deck::GetStorylet(const std::string& id) const {
        auto it = _all.find(id);
        if (it != _all.end())
            return it->second;
        return nullptr;
    }

    void Deck::AddStorylet(std::shared_ptr<Storylet> storylet)
    {
        if (_all.find(storylet->id) != _all.end())
            throw std::invalid_argument("Duplicate storylet id: " + storylet->id);
        _all[storylet->id] = storylet;
        storylet->_deck = this;
    }

    void Deck::Play(Storylet& storylet) {
        _currentDraw++;
        storylet.OnPlayed(_currentDraw, *context);
    }
 }