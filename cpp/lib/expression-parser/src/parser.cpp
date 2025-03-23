/*
 * This file is part of an MIT-licensed project: see LICENSE file or README.md for details.
 * Copyright (c) 2025 Ian Thomas
 */

#include "expression_parser/parser.h"
#include <stdexcept>
#include <sstream>
#include <algorithm>
#include <iostream>

namespace ExpressionParser {

    // Define the static TOKEN_REGEX.
    const std::regex Parser::TOKEN_REGEX = std::regex(R"(\s*(>=|<=|==|=|!=|>|<|\(|\)|,|and|&&|or|\|\||not|!|\+|\-|\/|\*|[A-Za-z_][A-Za-z0-9_]*|-?\d+\.\d+(?![A-Za-z_])|-?\d+(?![A-Za-z_])|"[^"]*"|'[^']*'|true|false|True|False)\s*)", std::regex::ECMAScript);

    Parser::Parser() : _pos(0) { }

    std::shared_ptr<ExpressionNode> Parser::Parse(const std::string &expression) {
        _tokens = Tokenize(expression);
        _pos = 0;
        std::shared_ptr<ExpressionNode> node = ParseOr();
        if (_pos < static_cast<int>(_tokens.size()))
            throw std::runtime_error("Unexpected token '" + _tokens[_pos] +
                                     "' at position " + std::to_string(_pos));
        return node;
    }

    std::vector<std::string> Parser::Tokenize(const std::string &expression) {
        std::vector<std::string> tokens;
        int pos = 0;
        while (pos < static_cast<int>(expression.size())) {
            std::smatch match;
            std::string sub = expression.substr(pos);
            if (!std::regex_search(sub, match, TOKEN_REGEX))
                throw std::runtime_error("Unrecognized token at position " + std::to_string(pos) +
                                         ": '" + expression.substr(pos) + "'");
            std::string token = match[1].str();

            token.erase(0, token.find_first_not_of(" \t\r\n"));
            token.erase(token.find_last_not_of(" \t\r\n") + 1);

            if (!token.empty()) {
                tokens.push_back(token);
                std::cerr << "Matched token: '" << token << "' at pos " << pos << std::endl;
            }

            pos += static_cast<int>(match.position() + match.length());
        }
        return tokens;
    }

    std::shared_ptr<ExpressionNode> Parser::ParseOr() {
        std::shared_ptr<ExpressionNode> node = ParseAnd();
        while (_Match({"or", "||"})) {
            node = std::make_shared<OpOr>(node, ParseAnd());
        }
        return node;
    }

    std::shared_ptr<ExpressionNode> Parser::ParseAnd() {
        std::shared_ptr<ExpressionNode> node = ParseBinaryOp();
        while (_Match({"and", "&&"})) {
            node = std::make_shared<OpAnd>(node, ParseBinaryOp());
        }
        return node;
    }

    std::shared_ptr<ExpressionNode> Parser::ParseMathAddSub() {
        std::shared_ptr<ExpressionNode> node = ParseMathMulDiv();
        while (_Match({"+", "-"})) {
            std::string op = _Previous();
            if (op == "+")
                node = std::make_shared<OpPlus>(node, ParseMathMulDiv());
            else
                node = std::make_shared<OpMinus>(node, ParseMathMulDiv());
        }
        return node;
    }

    std::shared_ptr<ExpressionNode> Parser::ParseMathMulDiv() {
        std::shared_ptr<ExpressionNode> node = ParseUnaryOp();
        while (_Match({"*", "/"})) {
            std::string op = _Previous();
            if (op == "*")
                node = std::make_shared<OpMultiply>(node, ParseUnaryOp());
            else
                node = std::make_shared<OpDivide>(node, ParseUnaryOp());
        }
        return node;
    }

    std::shared_ptr<ExpressionNode> Parser::ParseBinaryOp() {
        std::shared_ptr<ExpressionNode> node = ParseMathAddSub();
        while (_Match({"==", "!=", ">", "<", ">=", "<=", "="})) {
            std::string op = _Previous();
            if (op == "=" || op == "==")
                node = std::make_shared<OpEquals>(node, ParseMathAddSub());
            else if (op == "!=")
                node = std::make_shared<OpNotEquals>(node, ParseMathAddSub());
            else if (op == ">")
                node = std::make_shared<OpGreaterThan>(node, ParseMathAddSub());
            else if (op == "<")
                node = std::make_shared<OpLessThan>(node, ParseMathAddSub());
            else if (op == ">=")
                node = std::make_shared<OpGreaterThanEquals>(node, ParseMathAddSub());
            else if (op == "<=")
                node = std::make_shared<OpLessThanEquals>(node, ParseMathAddSub());
        }
        return node;
    }

    std::shared_ptr<ExpressionNode> Parser::ParseUnaryOp() {
        if (_Match("not") || _Match("!"))
            return std::make_shared<OpNot>(ParseUnaryOp());
        else if (_Match("-"))
            return std::make_shared<OpNegative>(ParseUnaryOp());
        return ParseTerm();
    }

    std::shared_ptr<LiteralString> Parser::ParseStringLiteral() {
        std::string s = _Peek();
        if (!s.empty() &&
            ((s.front() == '"' && s.back() == '"') || (s.front() == '\'' && s.back() == '\''))) {
            _Advance();
            return std::make_shared<LiteralString>(s.substr(1, s.size() - 2));
        }
        return nullptr;
    }

    std::shared_ptr<ExpressionNode> Parser::ParseTerm() {
        if (_Match("(")) {
            std::shared_ptr<ExpressionNode> node = ParseOr();
            _Consume(")");
            return node;
        }
        else if (_Match("true") || _Match("True"))
            return std::make_shared<LiteralBoolean>(true);
        else if (_Match("false") || _Match("False"))
            return std::make_shared<LiteralBoolean>(false);
        else if (std::regex_match(_Peek(), std::regex("^-?\\d+(\\.\\d+)?$"))) {
            return std::make_shared<LiteralNumber>(_Advance());
        }

        std::shared_ptr<LiteralString> stringLiteral = ParseStringLiteral();
        if (stringLiteral != nullptr)
            return stringLiteral;

        std::string identifier = _MatchIdentifier();
        if (!identifier.empty()) {
            if (_Match("(")) {
                std::vector<std::shared_ptr<ExpressionNode>> args;
                if (!_Match(")")) {
                    args.push_back(ParseOr());
                    while (_Match(","))
                        args.push_back(ParseOr());
                    _Consume(")");
                }
                return std::make_shared<FunctionCall>(identifier, args);
            }
            return std::make_shared<Variable>(identifier);
        }

        throw std::runtime_error("Unexpected token: " + _Peek());
    }

    // Helper functions:

    bool Parser::_Match(std::initializer_list<const char*> tokens) {
        if (_pos < static_cast<int>(_tokens.size())) {
            for (auto token : tokens) {
                if (_tokens[_pos] == token) {
                    _pos++;
                    return true;
                }
            }
        }
        return false;
    }

    bool Parser::_Match(const char* token) {
        return _Match({ token });
    }

    std::string Parser::_MatchIdentifier() {
        if (_pos < static_cast<int>(_tokens.size()) &&
            std::regex_match(_tokens[_pos], std::regex("^[A-Za-z_][A-Za-z0-9_]*$"))) {
            std::string token = _tokens[_pos];
            _pos++;
            return token;
        }
        return "";
    }

    void Parser::_Consume(const std::string &expectedToken) {
        if (!_Match(expectedToken.c_str())) {
            if (_pos >= static_cast<int>(_tokens.size()))
                throw std::runtime_error("Expected '" + expectedToken + "' but expression ended.");
            throw std::runtime_error("Expected '" + expectedToken + "' but found '" + _Peek() + "'");
        }
    }

    std::string Parser::_Peek() {
        return (_pos < static_cast<int>(_tokens.size())) ? _tokens[_pos] : "";
    }

    std::string Parser::_Previous() {
        return (_pos > 0) ? _tokens[_pos - 1] : "";
    }

    std::string Parser::_Advance() {
        if (_pos < static_cast<int>(_tokens.size())) {
            _pos++;
            return _tokens[_pos - 1];
        }
        return "";
    }

    std::string Parser::_Expect(const std::string &expectedToken) {
        std::string token = _Advance();
        if (token != expectedToken)
            throw std::runtime_error("Expected '" + expectedToken + "', but found '" + token + "'");
        return token;
    }

} // namespace ExpressionParser