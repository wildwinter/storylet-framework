/*
 * This file is part of an MIT-licensed project: see LICENSE file or README.md for details.
 * Copyright (c) 2025 Ian Thomas
 */

#ifndef PARSER_H
#define PARSER_H

#include <vector>
#include <string>
#include <regex>
#include <memory>

#include "expression.h"

namespace ExpressionParser {

    class Parser {
    private:
        std::vector<std::string> _tokens;
        int _pos;
        static const std::regex TOKEN_REGEX;
    public:
        Parser();
        std::shared_ptr<ExpressionNode> Parse(const std::string &expression);
    private:
        std::shared_ptr<ExpressionNode> ParseOr();
        std::shared_ptr<ExpressionNode> ParseAnd();
        std::shared_ptr<ExpressionNode> ParseMathAddSub();
        std::shared_ptr<ExpressionNode> ParseMathMulDiv();
        std::shared_ptr<ExpressionNode> ParseBinaryOp();
        std::shared_ptr<ExpressionNode> ParseUnaryOp();
        std::shared_ptr<ExpressionNode> ParseTerm();
        std::shared_ptr<LiteralString> ParseStringLiteral();

        std::vector<std::string> Tokenize(const std::string &expression);
        bool _Match(std::initializer_list<const char*> tokens);
        bool _Match(const char* token);
        std::string _MatchIdentifier();
        void _Consume(const std::string &expectedToken);
        std::string _Peek();
        std::string _Previous();
        std::string _Advance();
        std::string _Expect(const std::string &expectedToken);
    };

} // namespace ExpressionParser

#endif // PARSER_H