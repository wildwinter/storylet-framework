/*
 * This file is part of an MIT-licensed project: see LICENSE file or README.md for details.
 * Copyright (c) 2025 Ian Thomas
 */

using ExpressionParser;

namespace StoryletFramework;

using Context = Dictionary<string, object>;
using KeyMap = Dictionary<string, object>;

public static class ContextUtils
{
    private static readonly Parser expressionParser = new Parser();

    // Use the ExpressionParser to evaluate an expression
    // Unless it's a simple bool or number, in which case just use that.
    // dumpEval will fill a list with evaluation debug steps
    public static object EvalExpression(object val, Context context, List<string>? dumpEval = null)
    {
        if (val is bool || val is double || val is int)
            return val;

        if (val is string str) {
                var expression = expressionParser.Parse(str);
            return expression?.Evaluate(context, dumpEval) ?? throw new InvalidOperationException("Expression result should never be null.");
        }
        
        throw new ArgumentNullException(nameof(val), "Expression text cannot be null or empty.");
    }

    // Evaluate and copy any new entries in 'properties' into the context
    // dumpEval will fill a list with evaluation debug steps
    public static void InitContext(Context context, KeyMap properties, List<string>? dumpEval = null)
    {
        foreach (var kvp in properties)
        {
            var propName = kvp.Key;
            var expression = kvp.Value;

            if (dumpEval != null)
                dumpEval.Add($"InitContext: Evaluating {propName} = {expression}");

            var result = EvalExpression(expression, context, dumpEval);

            if (context.ContainsKey(propName))
                throw new InvalidOperationException($"Trying to initialize property '{propName}' in context when it already exists.");

            context[propName] = result;
        }
    }

    // Evaluate any entries in updates and apply them to the context, but they must already exist
    // dumpEval will fill a list with evaluation debug steps
    public static void UpdateContext(Context context, KeyMap updates, List<string>? dumpEval = null)
    {
        foreach (var kvp in updates)
        {
            var propName = kvp.Key;
            var expression = kvp.Value;

            if (dumpEval != null)
                dumpEval.Add($"UpdateContext: Evaluating {propName} = {expression}");

            var result = EvalExpression(expression, context, dumpEval);

            if (!context.ContainsKey(propName))
                throw new KeyNotFoundException($"Context variable '{propName}' is undefined.");

            context[propName] = result;
        }
    }

    // For debugging: Dump the context as a string
    public static string DumpContext(Context context)
    {
        var output = new List<string>();

        foreach (var kvp in context)
        {
            var propName = kvp.Key;
            var expression = kvp.Value;

            if (expression is Delegate)
            {
                output.Add($"{propName} = <function>");
            }
            else if (expression is bool || expression is double || expression is int || expression is string)
            {
                var result = EvalExpression(expression, context);
                output.Add($"{propName} = {result}");
            }
        }

        return string.Join("\n", output);
    }
}