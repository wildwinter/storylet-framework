#ifndef SF_CONTEXT_H
#define SF_CONTEXT_H

#include <iostream>
#include <string>
#include <unordered_map>
#include <functional>
#include <stdexcept>
#include <sstream>
#include <vector>
#include "expression_parser/parser.h" 

namespace StoryletFramework
{
    class ContextUtils
    {
    public:
        using Context = ExpressionParser::Context;
        using DumpEval = std::vector<std::string>;

        static ExpressionParser::Parser parser;

        // Evaluate an expression
        static std::any EvalExpression(const std::any& val, Context& context, DumpEval* dumpEval = nullptr)
        {
            if (val.type() == typeid(bool) || val.type() == typeid(double) || val.type() == typeid(int))
            {
                return val;
            }

            if (val.type() == typeid(std::string))
            {
                const auto& str = std::any_cast<std::string>(val);
                auto expression = parser.Parse(str);
                if (!expression)
                {
                    throw std::invalid_argument("Expression result should never be null.");
                }
                return expression->Evaluate(context, dumpEval);
            }

            throw std::invalid_argument("Expression text cannot be null or empty.");
        }

        // Initialize context with properties
        static void InitContext(Context& context, const Context& properties, DumpEval* dumpEval = nullptr)
        {
            for (const auto& kvp : properties)
            {
                const auto& propName = kvp.first;
                const auto& expression = kvp.second;

                if (dumpEval)
                {
                    dumpEval->push_back("InitContext: Evaluating " + propName + " = " + std::any_cast<std::string>(expression));
                }

                auto result = EvalExpression(expression, context, dumpEval);

                if (context.find(propName) != context.end())
                {
                    throw std::invalid_argument("Trying to initialize property '" + propName + "' in context when it already exists.");
                }

                context[propName] = result;
            }
        }

        // Update context with updates
        static void UpdateContext(Context& context, const Context& updates, DumpEval* dumpEval = nullptr)
        {
            for (const auto& kvp : updates)
            {
                const auto& propName = kvp.first;
                const auto& expression = kvp.second;

                if (dumpEval)
                {
                    dumpEval->push_back("UpdateContext: Evaluating " + propName + " = " + std::any_cast<std::string>(expression));
                }

                auto result = EvalExpression(expression, context, dumpEval);

                if (context.find(propName) == context.end())
                {
                    throw std::out_of_range("Context variable '" + propName + "' is undefined.");
                }

                context[propName] = result;
            }
        }

        // Dump the context as a string for debugging
        static std::string DumpContext(const Context& context)
        {
            std::ostringstream output;

            for (const auto& kvp : context)
            {
                const auto& propName = kvp.first;
                const auto& expression = kvp.second;

                if (expression.type() == typeid(std::function<void()>))
                {
                    output << propName << " = <function>\n";
                }
                else if (expression.type() == typeid(bool) || expression.type() == typeid(double) || expression.type() == typeid(int) || expression.type() == typeid(std::string))
                {
                    auto result = EvalExpression(expression, const_cast<Context&>(context));
                    output << propName << " = " << std::any_cast<std::string>(result) << "\n";
                }
            }

            return output.str();
        }
    };
}

#endif