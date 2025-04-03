#ifndef SF_CONTEXT_H
#define SF_CONTEXT_H

#include <any>
#include <unordered_map>
#include <vector>
#include "expression_parser/parser.h" 

namespace StoryletFramework
{
    using Context = ExpressionParser::Context;
    using DumpEval = std::vector<std::string>;
    using KeyedMap = std::unordered_map<std::string, std::any>;

    class ContextUtils
    {
    public:

        // Evaluate an expression
        static std::any EvalExpression(const std::any& val, Context& context, DumpEval* dumpEval = nullptr);

        // Initialize context with properties
        static void InitContext(Context& context, const KeyedMap& properties, DumpEval* dumpEval = nullptr);

        // Update context with updates
        static void UpdateContext(Context& context, const KeyedMap& updates, DumpEval* dumpEval = nullptr);

        // Dump the context as a string for debugging
        static std::string DumpContext(const Context& context);
    };
}

#endif