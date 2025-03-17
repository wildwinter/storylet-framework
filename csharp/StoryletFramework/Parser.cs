/*
 * This file is part of an MIT-licensed project: see LICENSE file or README.md for details.
 * Copyright (c) 2025 Ian Thomas
 */
 
using System.Text.RegularExpressions;

namespace ExpressionParser
{
    // Custom exception to mirror Python's SyntaxError.
    public class SyntaxErrorException : Exception
    {
        public SyntaxErrorException(string message) : base(message) { }
    }

    public class Parser
    {
        private List<string> _tokens;
        private int _pos;

        // This regex corresponds to the Python TOKEN_REGEX with VERBOSE mode.
        // The regex uses a verbatim string literal (@) so backslashes don't need double escaping.
        private static readonly Regex TOKEN_REGEX = new Regex(@"
            \s*(
                >=|<=|==|=|!=|>|<|\(|\)|,|and|&&|or|\|\||not|!      # Operators & keywords
                | \+|\-|\/|\*                                      # Maths operators
                | [A-Za-z_][A-Za-z0-9_]*                           # Identifiers (Variables & Functions)
                | -?\d+\.\d+(?![A-Za-z_])                          # Floating-point numbers (supports negative)
                | -?\d+(?![A-Za-z_])                               # Integers (supports negative)
                | ""[^""]*""                                      # Strings in double quotes
                | '[^']*'                                        # Strings in single quotes
                | true|false|True|False                           # Booleans
            )\s*
        ", RegexOptions.IgnorePatternWhitespace);

        public Parser()
        {
            _tokens = new List<string>();
            _pos = 0;
        }

        public ExpressionNode Parse(string expression)
        {
            _tokens = Tokenize(expression);
            _pos = 0;
            ExpressionNode node = ParseOr();

            if (_pos < _tokens.Count)
                throw new SyntaxErrorException($"Unexpected token '{_tokens[_pos]}' at position {_pos}");

            return node;
        }

        public List<string> Tokenize(string expression)
        {
            List<string> tokens = new List<string>();
            int pos = 0;

            while (pos < expression.Length)
            {
                Match match = TOKEN_REGEX.Match(expression, pos);
                if (!match.Success)
                {
                    throw new SyntaxErrorException($"Unrecognized token at position {pos}: '{expression.Substring(pos)}'");
                }

                string token = match.Groups[1].Value.Trim();
                if (!string.IsNullOrEmpty(token))
                    tokens.Add(token);

                pos = match.Index + match.Length;
            }

            return tokens;
        }

        private ExpressionNode ParseOr()
        {
            ExpressionNode node = ParseAnd();
            while (_Match("or") || _Match("||"))
            {
                node = new OpOr(node, ParseAnd());
            }
            return node;
        }

        private ExpressionNode ParseAnd()
        {
            ExpressionNode node = ParseBinaryOp();
            while (_Match("and") || _Match("&&"))
            {
                node = new OpAnd(node, ParseBinaryOp());
            }
            return node;
        }

        private ExpressionNode ParseMathAddSub()
        {
            ExpressionNode node = ParseMathMulDiv();
            while (_Match("+", "-"))
            {
                string? op = _Previous();
                if (op == "+")
                    node = new OpPlus(node, ParseMathMulDiv());
                else
                    node = new OpMinus(node, ParseMathMulDiv());
            }
            return node;
        }

        private ExpressionNode ParseMathMulDiv()
        {
            ExpressionNode node = ParseUnaryOp();
            while (_Match("*", "/"))
            {
                string? op = _Previous();
                if (op == "*")
                    node = new OpMultiply(node, ParseUnaryOp());
                else
                    node = new OpDivide(node, ParseUnaryOp());
            }
            return node;
        }

        private ExpressionNode ParseBinaryOp()
        {
            ExpressionNode node = ParseMathAddSub();
            while (_Match("==", "!=", ">", "<", ">=", "<=", "="))
            {
                string? op = _Previous();
                if (op == "=" || op == "==")
                    node = new OpEquals(node, ParseMathAddSub());
                else if (op == "!=")
                    node = new OpNotEquals(node, ParseMathAddSub());
                else if (op == ">")
                    node = new OpGreaterThan(node, ParseMathAddSub());
                else if (op == "<")
                    node = new OpLessThan(node, ParseMathAddSub());
                else if (op == ">=")
                    node = new OpGreaterThanEquals(node, ParseMathAddSub());
                else if (op == "<=")
                    node = new OpLessThanEquals(node, ParseMathAddSub());
            }
            return node;
        }

        private ExpressionNode ParseUnaryOp()
        {
            if (_Match("not") || _Match("!"))
            {
                return new OpNot(ParseUnaryOp());
            }
            else if (_Match("-"))
            {
                return new OpNegative(ParseUnaryOp());
            }
            return ParseTerm();
        }

        // Returns a LiteralString node if the current token is a string literal, or null otherwise.
        private LiteralString? ParseStringLiteral()
        {
            string? stringVal = _Peek();
            if (!string.IsNullOrEmpty(stringVal) &&
                ((stringVal.StartsWith("\"") && stringVal.EndsWith("\"")) ||
                 (stringVal.StartsWith("'") && stringVal.EndsWith("'"))))
            {
                _Advance();
                // Remove the first and last character (the quotes)
                return new LiteralString(stringVal.Substring(1, stringVal.Length - 2));
            }
            return null;
        }

        private ExpressionNode ParseTerm()
        {
            if (_Match("("))
            {
                ExpressionNode node = ParseOr();
                _Consume(")");
                return node;
            }
            else if (_Match("true") || _Match("True"))
            {
                return new LiteralBoolean(true);
            }
            else if (_Match("false") || _Match("False"))
            {
                return new LiteralBoolean(false);
            }
            // Check if the token matches a number pattern.
            else if (Regex.IsMatch(_Peek() ?? "", @"^-?\d+(\.\d+)?$"))
            {
                return new LiteralNumber(_Advance()!);
            }

            LiteralString? stringLiteral = ParseStringLiteral();
            if (stringLiteral != null)
                return stringLiteral;

            string? identifier = _MatchIdentifier();
            if (!string.IsNullOrEmpty(identifier))
            {
                if (_Match("("))
                {
                    List<ExpressionNode> args = new List<ExpressionNode>();
                    if (!_Match(")"))
                    {
                        args.Add(ParseOr());
                        while (_Match(","))
                        {
                            args.Add(ParseOr());
                        }
                        _Consume(")");
                    }
                    return new FunctionCall(identifier, args);
                }
                return new Variable(identifier);
            }

            throw new SyntaxErrorException($"Unexpected token: {_Peek()}");
        }

        private bool _Match(params string[] expectedTokens)
        {
            if (_pos < _tokens.Count && Array.Exists(expectedTokens, t => t == _tokens[_pos]))
            {
                _pos++;
                return true;
            }
            return false;
        }

        private void _Consume(string expectedToken)
        {
            if (!_Match(expectedToken))
            {
                if (_pos >= _tokens.Count)
                    throw new SyntaxErrorException($"Expected '{expectedToken}' but expression ended.");
                throw new SyntaxErrorException($"Expected '{expectedToken}' but found '{_Peek()}'");
            }
        }

        private string? _Peek()
        {
            return _pos < _tokens.Count ? _tokens[_pos] : null;
        }

        private string? _Previous()
        {
            return _pos > 0 ? _tokens[_pos - 1] : null;
        }

        private string? _Advance()
        {
            if (_pos < _tokens.Count)
            {
                _pos++;
                return _tokens[_pos - 1];
            }
            return null;
        }

        private string _Expect(string expectedToken)
        {
            string? token = _Advance();
            if (token != expectedToken)
            {
                throw new SyntaxErrorException($"Expected '{expectedToken}', but found '{token}'");
            }
            return token;
        }

        private string? _MatchIdentifier()
        {
            if (_pos < _tokens.Count && Regex.IsMatch(_tokens[_pos], @"^[A-Za-z_][A-Za-z0-9_]*$"))
            {
                string token = _tokens[_pos];
                _pos++;
                return token;
            }
            return null;
        }
    }
}