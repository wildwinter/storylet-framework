/*
 * This file is part of an MIT-licensed project: see LICENSE file or README.md for details.
 * Copyright (c) 2025 Ian Thomas
 */

using System.Reflection;

namespace ExpressionParser
{
    // Utility functions: type conversion and formatting
    public static class Utils
    {
        public static bool MakeBool(object val)
        {
            if (val is bool b)
                return b;
            if (val is int i)
                return i != 0;
            if (val is double d)
                return d != 0;
            if (val is string s)
            {
                string lower = s.ToLower();
                return lower == "true" || lower == "1";
            }
            throw new Exception($"Type mismatch: Expecting bool, but got '{val}'");
        }

        public static double MakeNumeric(object val)
        {
            if (val is bool b)
                return b ? 1 : 0;
            if (val is int i)
                return Convert.ToDouble(i);
            if (val is double d)
                return d;
            if (val is string s)
            {
                if (double.TryParse(s, out double dResult))
                    return dResult;
            }
            throw new Exception($"Type mismatch: Expecting number but got '{val}'");
        }

        public static string MakeString(object val)
        {
            if (val is string s)
                return s;
            if (val is bool b)
                return b ? "true" : "false";
            if (val is int i)
                return i.ToString();
            if (val is double d)
                return d.ToString();
            throw new Exception($"Type mismatch: Expecting string but got '{val}'");
        }

        public static object MakeTypeMatch(object leftVal, object rightVal)
        {
            if (leftVal is bool)
                return MakeBool(rightVal);
            if (leftVal is int || leftVal is double)
                return MakeNumeric(rightVal);
            if (leftVal is string)
                return MakeString(rightVal);
            throw new Exception($"Type mismatch: unrecognised type for '{leftVal}'");
        }

        public static string FormatBoolean(bool val) => val ? "true" : "false";

        public static string FormatNumeric(double num)
        {
            // If num is whole, show without a decimal point.
            if (num % 1 == 0)
                return ((int)num).ToString();
            return num.ToString();
        }

        public static string FormatString(string val)
        {
            switch (Writer.StringFormat)
            {
                case Writer.STRING_FORMAT.SINGLEQUOTE:
                    return $"'{val}'";
                case Writer.STRING_FORMAT.ESCAPED_SINGLEQUOTE:
                    return $"\\'{val}\\'";
                case Writer.STRING_FORMAT.ESCAPED_DOUBLEQUOTE:
                    return $"\\\"{val}\\\"";
                case Writer.STRING_FORMAT.DOUBLEQUOTE:
                default:
                    return $"\"{val}\"";
            }
        }

        public static string FormatValue(object val)
        {
            if (val is bool b)
                return FormatBoolean(b);
            if (val is int i)
                return i.ToString();
            if (val is double d)
                return FormatNumeric(d);
            if (val is string s)
                return FormatString(s);
            return "";
        }
    }

    // Abstract base class for AST nodes
    public abstract class ExpressionNode
    {
        public string Name { get; set; }
        public int Precedence { get; set; }

        protected ExpressionNode(string name, int precedence)
        {
            Name = name;
            Precedence = precedence;
        }

        public abstract object Evaluate(Dictionary<string, object> context, List<string>? dumpEval = null);
        public abstract string DumpStructure(int indent = 0);
        public abstract string Write();
    }

    // Abstract base for binary operators
    public abstract class BinaryOp : ExpressionNode
    {
        protected ExpressionNode Left;
        protected ExpressionNode Right;
        protected string Op;

        protected BinaryOp(string name, ExpressionNode left, string op, ExpressionNode right, int precedence)
            : base(name, precedence)
        {
            Left = left;
            Op = op;
            Right = right;
        }

        public override object Evaluate(Dictionary<string, object> context, List<string>? dumpEval = null)
        {
            object leftVal = Left.Evaluate(context, dumpEval);
            object rightVal = Right.Evaluate(context, dumpEval);
            object result = DoEval(leftVal, rightVal);
            if (dumpEval != null)
            {
                dumpEval.Add($"Evaluated: {Utils.FormatValue(leftVal)} {Op} {Utils.FormatValue(rightVal)} = {Utils.FormatValue(result)}");
            }
            return result;
        }

        protected abstract object DoEval(object leftVal, object rightVal);

        public override string DumpStructure(int indent = 0)
        {
            string indentStr = new string(' ', indent * 2);
            string outStr = indentStr + Name + "\n" +
                            Left.DumpStructure(indent + 1) +
                            Right.DumpStructure(indent + 1);
            return outStr;
        }

        public override string Write()
        {
            string leftStr = Left.Write();
            string rightStr = Right.Write();

            if (Left.Precedence < this.Precedence)
                leftStr = "(" + leftStr + ")";
            if (Right.Precedence < this.Precedence)
                rightStr = "(" + rightStr + ")";

            return $"{leftStr} {Op} {rightStr}";
        }
    }

    // Concrete binary operator nodes
    public class OpOr : BinaryOp
    {
        public OpOr(ExpressionNode left, ExpressionNode right)
            : base("Or", left, "or", right, 40) { }

        protected override object DoEval(object leftVal, object rightVal)
        {
            return Utils.MakeBool(leftVal) || Utils.MakeBool(rightVal);
        }
    }

    public class OpAnd : BinaryOp
    {
        public OpAnd(ExpressionNode left, ExpressionNode right)
            : base("And", left, "and", right, 50) { }

        protected override object DoEval(object leftVal, object rightVal)
        {
            return Utils.MakeBool(leftVal) && Utils.MakeBool(rightVal);
        }
    }

    public class OpEquals : BinaryOp
    {
        public OpEquals(ExpressionNode left, ExpressionNode right)
            : base("Equals", left, "==", right, 60) { }

        protected override object DoEval(object leftVal, object rightVal)
        {
            rightVal = Utils.MakeTypeMatch(leftVal, rightVal);
            return leftVal.Equals(rightVal);
        }
    }

    public class OpNotEquals : BinaryOp
    {
        public OpNotEquals(ExpressionNode left, ExpressionNode right)
            : base("NotEquals", left, "!=", right, 60) { }

        protected override object DoEval(object leftVal, object rightVal)
        {
            rightVal = Utils.MakeTypeMatch(leftVal, rightVal);
            return !leftVal.Equals(rightVal);
        }
    }

    public class OpPlus : BinaryOp
    {
        public OpPlus(ExpressionNode left, ExpressionNode right)
            : base("Plus", left, "+", right, 70) { }

        protected override object DoEval(object leftVal, object rightVal)
        {
            return Utils.MakeNumeric(leftVal) + Utils.MakeNumeric(rightVal);
        }
    }

    public class OpMinus : BinaryOp
    {
        public OpMinus(ExpressionNode left, ExpressionNode right)
            : base("Minus", left, "-", right, 70) { }

        protected override object DoEval(object leftVal, object rightVal)
        {
            return Utils.MakeNumeric(leftVal) - Utils.MakeNumeric(rightVal);
        }
    }

    public class OpDivide : BinaryOp
    {
        public OpDivide(ExpressionNode left, ExpressionNode right)
            : base("Divide", left, "/", right, 85) { }

        protected override object DoEval(object leftVal, object rightVal)
        {
            double numRight = Utils.MakeNumeric(rightVal);
            if (numRight == 0)
                throw new DivideByZeroException("Division by zero.");
            return Utils.MakeNumeric(leftVal) / numRight;
        }
    }

    public class OpMultiply : BinaryOp
    {
        public OpMultiply(ExpressionNode left, ExpressionNode right)
            : base("Multiply", left, "*", right, 80) { }

        protected override object DoEval(object leftVal, object rightVal)
        {
            return Utils.MakeNumeric(leftVal) * Utils.MakeNumeric(rightVal);
        }
    }

    public class OpGreaterThan : BinaryOp
    {
        public OpGreaterThan(ExpressionNode left, ExpressionNode right)
            : base("GreaterThan", left, ">", right, 60) { }

        protected override object DoEval(object leftVal, object rightVal)
        {
            return Utils.MakeNumeric(leftVal) > Utils.MakeNumeric(rightVal);
        }
    }

    public class OpLessThan : BinaryOp
    {
        public OpLessThan(ExpressionNode left, ExpressionNode right)
            : base("LessThan", left, "<", right, 60) { }

        protected override object DoEval(object leftVal, object rightVal)
        {
            return Utils.MakeNumeric(leftVal) < Utils.MakeNumeric(rightVal);
        }
    }

    public class OpGreaterThanEquals : BinaryOp
    {
        public OpGreaterThanEquals(ExpressionNode left, ExpressionNode right)
            : base("GreaterThanEquals", left, ">=", right, 60) { }

        protected override object DoEval(object leftVal, object rightVal)
        {
            return Utils.MakeNumeric(leftVal) >= Utils.MakeNumeric(rightVal);
        }
    }

    public class OpLessThanEquals : BinaryOp
    {
        public OpLessThanEquals(ExpressionNode left, ExpressionNode right)
            : base("LessThanEquals", left, "<=", right, 60) { }

        protected override object DoEval(object leftVal, object rightVal)
        {
            return Utils.MakeNumeric(leftVal) <= Utils.MakeNumeric(rightVal);
        }
    }

    // Abstract base for unary operators
    public abstract class UnaryOp : ExpressionNode
    {
        protected ExpressionNode Operand;
        protected string Op;

        protected UnaryOp(string name, string op, ExpressionNode operand, int precedence)
            : base(name, precedence)
        {
            Operand = operand;
            Op = op;
        }

        public override object Evaluate(Dictionary<string, object> context, List<string>? dumpEval = null)
        {
            object val = Operand.Evaluate(context, dumpEval);
            object result = DoEval(val);
            if (dumpEval != null)
            {
                dumpEval.Add($"Evaluated: {Op} {Utils.FormatValue(val)} = {Utils.FormatValue(result)}");
            }
            return result;
        }

        protected abstract object DoEval(object val);

        public override string DumpStructure(int indent = 0)
        {
            string indentStr = new string(' ', indent * 2);
            return indentStr + Name + "\n" + Operand.DumpStructure(indent + 1);
        }

        public override string Write()
        {
            string operandStr = Operand.Write();
            if (Operand.Precedence < this.Precedence)
                operandStr = "(" + operandStr + ")";
            return $"{Op} {operandStr}";
        }
    }

    public class OpNegative : UnaryOp
    {
        public OpNegative(ExpressionNode operand)
            : base("Negative", "-", operand, 90) { }

        protected override object DoEval(object val)
        {
            double num = Utils.MakeNumeric(val);
            return -num;
        }
    }

    public class OpNot : UnaryOp
    {
        public OpNot(ExpressionNode operand)
            : base("Not", "not", operand, 90) { }

        protected override object DoEval(object val)
        {
            bool b = Utils.MakeBool(val);
            return !b;
        }
    }

    // Literal nodes
    public class LiteralBoolean : ExpressionNode
    {
        private bool _value;
        public LiteralBoolean(bool value) : base("Boolean", 100)
        {
            _value = value;
        }

        public override object Evaluate(Dictionary<string, object> context, List<string>? dumpEval = null)
        {
            if (dumpEval != null)
                dumpEval.Add($"Boolean: {Utils.FormatBoolean(_value)}");
            return _value;
        }

        public override string DumpStructure(int indent = 0)
        {
            string indentStr = new string(' ', indent * 2);
            return indentStr + $"Boolean({Utils.FormatBoolean(_value)})\n";
        }

        public override string Write() => Utils.FormatBoolean(_value);
    }

    public class LiteralNumber : ExpressionNode
    {
        private double _value;
        public LiteralNumber(string value) : base("Number", 100)
        {
            _value = double.Parse(value);
        }

        public override object Evaluate(Dictionary<string, object> context, List<string>? dumpEval = null)
        {
            if (dumpEval != null)
                dumpEval.Add($"Number: {Utils.FormatNumeric(_value)}");
            return _value;
        }

        public override string DumpStructure(int indent = 0)
        {
            string indentStr = new string(' ', indent * 2);
            return indentStr + $"Number({Utils.FormatNumeric(_value)})\n";
        }

        public override string Write() => Utils.FormatNumeric(_value);
    }

    public class LiteralString : ExpressionNode
    {
        private string _value;
        public LiteralString(string value) : base("String", 100)
        {
            _value = value;
        }

        public override object Evaluate(Dictionary<string, object> context, List<string>? dumpEval = null)
        {
            if (dumpEval != null)
                dumpEval.Add($"String: {Utils.FormatString(_value)}");
            return _value;
        }

        public override string DumpStructure(int indent = 0)
        {
            string indentStr = new string(' ', indent * 2);
            return indentStr + $"String({Utils.FormatString(_value)})\n";
        }

        public override string Write() => Utils.FormatString(_value);
    }

    public class Variable : ExpressionNode
    {
        private string _name;
        public Variable(string name) : base("Variable", 100)
        {
            _name = name;
        }

        public override object Evaluate(Dictionary<string, object> context, List<string>? dumpEval = null)
        {
            if (!context.ContainsKey(_name))
                throw new Exception($"Variable '{_name}' not found in context.");

            object value = context[_name];
            if (!(value is int || value is double || value is bool || value is string))
                throw new Exception($"Variable '{_name}' must return bool, string, or numeric.");

            if (dumpEval != null)
                dumpEval.Add($"Fetching variable: {_name} -> {Utils.FormatValue(value)}");
            return value;
        }

        public override string DumpStructure(int indent = 0)
        {
            string indentStr = new string(' ', indent * 2);
            return indentStr + $"Variable({_name})\n";
        }

        public override string Write() => _name;
    }

    public class FunctionCall : ExpressionNode
    {
        private string _funcName;
        private List<ExpressionNode> _args;

        public FunctionCall(string funcName, List<ExpressionNode> args) : base("FunctionCall", 100)
        {
            _funcName = funcName;
            _args = args ?? new List<ExpressionNode>();
        }

        public override object Evaluate(Dictionary<string, object> context, List<string>? dumpEval = null)
        {
            if (!context.ContainsKey(_funcName))
                throw new Exception($"Function '{_funcName}' not found in context.");

            object funcObj = context[_funcName];
            if (!(funcObj is Delegate))
                throw new Exception($"Context entry for '{_funcName}' is not a function.");

            Delegate func = (Delegate)funcObj;
            List<object> argValues = new List<object>();
            foreach (var arg in _args)
            {
                argValues.Add(arg.Evaluate(context, dumpEval));
            }

            // Strict arity check using reflection on the delegate's method signature.
            ParameterInfo[] expectedParams = func.Method.GetParameters();
            if (argValues.Count != expectedParams.Length)
            {
                string formattedArgs = string.Join(", ", argValues.Select(v => Utils.FormatValue(v)));
                throw new Exception($"Function '{_funcName}' does not support the provided arguments ({formattedArgs}).");
            }

            object? result = func.DynamicInvoke(argValues.ToArray());

            if (!(result is int || result is double || result is bool || result is string))
                throw new Exception($"Function '{_funcName}' must return bool, string, or numeric.");

            if (dumpEval != null)
            {
                string formattedArgs = string.Join(", ", argValues.Select(v => Utils.FormatValue(v)));
                dumpEval.Add($"Called function: {_funcName}({formattedArgs}) = {Utils.FormatValue(result)}");
            }

            return result;
        }

        public override string DumpStructure(int indent = 0)
        {
            string indentStr = new string(' ', indent * 2);
            string outStr = indentStr + $"FunctionCall({_funcName})\n";
            foreach (var arg in _args)
            {
                outStr += arg.DumpStructure(indent + 1);
            }
            return outStr;
        }

        public override string Write()
        {
            string argsStr = string.Join(", ", _args.Select(arg => arg.Write()));
            return $"{_funcName}({argsStr})";
        }
    }
}