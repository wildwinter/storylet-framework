/*
 * This file is part of an MIT-licensed project: see LICENSE file or README.md for details.
 * Copyright (c) 2025 Ian Thomas
 */

 #include "storylet_framework/context.h"
 
 #include <iostream>
 #include <string>
 #include <stdexcept>
 #include <sstream>

namespace StoryletFramework
{

    ExpressionParser::Parser expressionParser;

    // Evaluate an expression
    std::any ContextUtils::EvalExpression(const std::any& val, const Context& context, DumpEval* dumpEval)
    {
        if (val.type() == typeid(bool) || val.type() == typeid(double) || val.type() == typeid(int))
        {
            return val;
        }

        if (val.type() == typeid(std::string))
        {
            const auto& str = std::any_cast<std::string>(val);
            auto expression = expressionParser.Parse(str);
            if (!expression)
            {
                throw std::invalid_argument("Expression result should never be null.");
            }
            return expression->Evaluate(context, dumpEval);
        }

        throw std::invalid_argument("Expression text cannot be null or empty.");
    }

    // Initialize context with properties
    void ContextUtils::InitContext(Context& context, const KeyedMap& properties, DumpEval* dumpEval)
    {
        for (const auto& kvp : properties)
        {
            const auto& propName = kvp.first;
            const auto& expression = kvp.second;

            if (context.find(propName) != context.end())
            {
                throw std::invalid_argument("Trying to initialize property '" + propName + "' in context when it already exists.");
            }

            if (dumpEval)
            {
                dumpEval->push_back("InitContext: Evaluating " + propName + " = " + std::any_cast<std::string>(expression));
            }

            auto result = EvalExpression(expression, context, dumpEval);
            context[propName] = result;
        }
    }

    // Update context with updates
    void ContextUtils::UpdateContext(Context& context, const KeyedMap& updates, DumpEval* dumpEval)
    {
        for (const auto& kvp : updates)
        {
            const auto& propName = kvp.first;
            const auto& expression = kvp.second;

            if (context.find(propName) == context.end())
            {
                throw std::out_of_range("Context variable '" + propName + "' is undefined.");
            }

            if (dumpEval)
            {
                dumpEval->push_back("UpdateContext: Evaluating " + propName + " = " + std::any_cast<std::string>(expression));
            }

            auto result = EvalExpression(expression, context, dumpEval);

            if (dumpEval)
            {
                dumpEval->push_back("Setting " + propName + " to " + std::any_cast<std::string>(expression));
            }

            context[propName] = result;
        }
    }

    // Dump the context as a string for debugging
    std::string ContextUtils::DumpContext(const Context& context)
    {
        std::ostringstream output;

        for (const auto& kvp : context)
        {
            const auto& propName = kvp.first;
            const auto& expression = kvp.second;

            if (expression.type() == typeid(ExpressionParser::FunctionWrapper))
            {
                output << propName << " = <function>\n";
            }
            else if (expression.type() == typeid(bool))
            {
                output << propName << " = " << (std::any_cast<bool>(expression) ? "true" : "false") << "\n";
            }
            else if (expression.type() == typeid(int))
            {
                output << propName << " = " << std::any_cast<int>(expression) << "\n";
            } 
            else if (expression.type() == typeid(std::string))
            {
                output << propName << " = \"" << std::any_cast<std::string>(expression) << "\"\n";
            } 
            else if (expression.type() == typeid(double))
            {
                output << propName << " = " << std::any_cast<double>(expression) << "\n";
            } 
            else
            {
                output << propName << " = <unknown type>\n";
            }
        }

        return output.str();
    }
}