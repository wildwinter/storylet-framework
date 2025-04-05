/*
 * This file is part of an MIT-licensed project: see LICENSE file or README.md for details.
 * Copyright (c) 2025 Ian Thomas
 */

 #include "storylet_framework/context.h"
 #include "storylet_framework/storylets.h"
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
         _nextDraw = 0;
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
         if (redraw == REDRAW_NEVER && _nextDraw < 0)
             return false;
         if (redraw == REDRAW_ALWAYS)
             return true;
         return currentDraw >= _nextDraw;
     }
 
     // Call when actually drawn - updates the redraw counter
     void Storylet::Drawn(int currentDraw)
     {
         if (redraw == REDRAW_NEVER)
         {
             _nextDraw = -1;
         }
         else
         {
             _nextDraw = currentDraw + redraw;
         }
     }
 
     // Parse a Storylet from JSON-like data
     std::shared_ptr<Storylet> Storylet::FromJson(const nlohmann::json& json, const nlohmann::json& defaults)
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

    Deck::Deck() {
        _context = std::make_shared<Context>();
    }

    Deck::Deck(Context& context) {
        _context = std::shared_ptr<Context>(&context, [](Context*) {
            // Do nothing, we don't own the context
        });
    }

    std::shared_ptr<Deck> Deck::FromJson(const nlohmann::json& json, Context* context, bool reshuffle, DumpEval* dumpEval)
    {
        std::shared_ptr<Deck> deck = std::make_shared<Deck>(*context);
        deck->LoadJson(json, dumpEval);
        if (reshuffle)
            deck->Reshuffle(nullptr, dumpEval);
        return deck;
    }

    void Deck::LoadJson(const nlohmann::json& json, DumpEval* dumpEval)
    {
        _readPacketFromJson(json, nlohmann::json::object(), dumpEval);
    }

    void Deck::Reset()
    {
        _currentDraw = 0;
        for (auto& [id, storylet] : _all)
        {
            storylet->Reset();
        }
    }

    void Deck::Reshuffle(std::function<bool(const Storylet&)> filter, DumpEval* dumpEval)
    {
        if (AsyncReshuffleInProgress())
            throw std::runtime_error("Async reshuffle in progress, can't call Reshuffle()");

        _reshufflePrep(filter, dumpEval);
        _reshuffleDoChunk(_reshuffleState.toProcess.size());
        _reshuffleFinalise();
    }

    void Deck::ReshuffleAsync(std::function<void()> callback, std::function<bool(const Storylet&)> filter, DumpEval* dumpEval)
    {
        if (AsyncReshuffleInProgress())
            throw std::runtime_error("Async reshuffle in progress, can't call ReshuffleAsync()");

        _reshuffleState.callback = callback;
        _reshufflePrep(filter, dumpEval);
    }

    bool Deck::AsyncReshuffleInProgress() const
    {
        return _reshuffleState.callback != nullptr;
    }

    void Deck::Update()
    {
        if (AsyncReshuffleInProgress())
        {
            _reshuffleDoChunk(asyncReshuffleCount);
            if (_reshuffleState.toProcess.empty())
                _reshuffleFinalise();
        }
    }

    std::string Deck::DumpDrawPile() const
    {
        if (AsyncReshuffleInProgress())
            throw std::runtime_error("Async reshuffle in progress, can't call DumpDrawPile()");

        std::string result;
        for (const auto& storylet : _drawPile)
        {
            result += storylet->id + ",";
        }
        if (!result.empty())
            result.pop_back(); // Remove trailing comma
        return result;
    }

    std::shared_ptr<Storylet> Deck::Draw()
    {
        if (AsyncReshuffleInProgress())
            throw std::runtime_error("Async reshuffle in progress, can't call Draw()");

        _currentDraw++;

        if (_drawPile.empty())
            return nullptr;

        std::shared_ptr<Storylet> storylet = _drawPile.front();
        _drawPile.erase(_drawPile.begin());

        if (!storylet->updateOnDrawn.empty())
        {
            ContextUtils::UpdateContext(*_context, storylet->updateOnDrawn);
        }

        storylet->Drawn(_currentDraw);
        return storylet;
    }

    std::vector<std::shared_ptr<Storylet>> Deck::DrawHand(int count, bool reshuffleIfNeeded) {
        std::vector<std::shared_ptr<Storylet>> storylets;
    
        for (int i = 0; i < count; i++) {
            if (_drawPile.empty()) {
                if (reshuffleIfNeeded) {
                    Reshuffle(nullptr, nullptr);
                } else {
                    break;
                }
            }
    
            auto storylet = Draw();
            if (!storylet) {
                break;
            }
    
            storylets.push_back(storylet);
        }
    
        return storylets;
    }

    void Deck::_readPacketFromJson(const nlohmann::json& json, nlohmann::json defaults, DumpEval* dumpEval)
    {
        if (json.contains("context"))
        {
            ContextUtils::InitContext(*_context, json["context"], dumpEval);
        }

        if (json.contains("defaults"))
        {
            defaults.update(json["defaults"]);
        }

        if (json.contains("storylets"))
        {
            _readStoryletsFromJson(json["storylets"], defaults, dumpEval);
        }
    }

    void Deck::_readStoryletsFromJson(const nlohmann::json& json, nlohmann::json defaults, DumpEval* dumpEval)
    {
        for (const auto& item : json)
        {
            if (item.contains("storylets") || item.contains("defaults") || item.contains("context"))
            {
                _readPacketFromJson(item, defaults, dumpEval);
                continue;
            }

            if (!item.contains("id"))
                throw std::invalid_argument("Json item is not a storylet or packet");

            std::shared_ptr<Storylet> storylet = Storylet::FromJson(item, defaults);
            if (_all.find(storylet->id) != _all.end())
                throw std::invalid_argument("Duplicate storylet id: " + storylet->id);

            _all[storylet->id] = storylet;
            if (dumpEval)
                dumpEval->push_back("Added storylet '" + storylet->id + "'");
            
        }
    }

    void Deck::_reshufflePrep(std::function<bool(const Storylet&)> filter, DumpEval* dumpEval)
    {
        _drawPile.clear();
        _reshuffleState.dumpEval = dumpEval;
        _reshuffleState.filter = filter;
        _reshuffleState.priorityMap.clear();
        _reshuffleState.toProcess.clear();

        for (auto& [id, storylet] : _all)
        {
            _reshuffleState.toProcess.push_back(storylet);
        }
    }

    void Deck::_reshuffleDoChunk(size_t count)
    {
        size_t numberToDo = std::min(count, _reshuffleState.toProcess.size());

        while (numberToDo > 0)
        {
            numberToDo--;

            std::shared_ptr<Storylet> storylet = _reshuffleState.toProcess.front();
            _reshuffleState.toProcess.erase(_reshuffleState.toProcess.begin());

            if (!storylet->CanDraw(_currentDraw))
                continue;

            if (_reshuffleState.filter && !_reshuffleState.filter(*storylet))
                continue;

            if (!storylet->CheckCondition(*_context, _reshuffleState.dumpEval))
                continue;

            int priority = storylet->CalcCurrentPriority(*_context, useSpecificity, _reshuffleState.dumpEval);

            _reshuffleState.priorityMap[priority].push_back(storylet);
        }
    }

    void Deck::_reshuffleFinalise()
    {
        std::vector<int> sortedPriorities;
        for (const auto& [priority, _] : _reshuffleState.priorityMap)
        {
            sortedPriorities.push_back(priority);
        }
        std::sort(sortedPriorities.begin(), sortedPriorities.end(), std::greater<int>());

        for (int priority : sortedPriorities)
        {
            auto& bucket = _reshuffleState.priorityMap[priority];
            std::shuffle(bucket.begin(), bucket.end(), std::mt19937{std::random_device{}()});
            for (std::shared_ptr<Storylet> storylet : bucket)
            {
                _drawPile.push_back(storylet);
            }
        }

        _reshuffleState.priorityMap.clear();
        _reshuffleState.toProcess.clear();
        _reshuffleState.filter = nullptr;

        if (_reshuffleState.callback)
        {
            _reshuffleState.callback();
            _reshuffleState.callback = nullptr;
        }
    }
 }