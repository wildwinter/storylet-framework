# This file is part of an MIT-licensed project: see LICENSE file or README.md for details.
# Copyright (c) 2025 Ian Thomas

import re
from typing import List, Optional

from .expression import (
    ExpressionNode,
    LiteralBoolean,
    LiteralNumber,
    LiteralString,
    OpAnd,
    OpDivide,
    OpEquals,
    OpGreaterThan,
    OpGreaterThanEquals,
    OpLessThan,
    OpLessThanEquals,
    OpMinus,
    OpMultiply,
    OpNegative,
    OpNot,
    OpNotEquals,
    OpOr,
    OpPlus,
    Variable,
    FunctionCall,
)

TOKEN_REGEX = re.compile(r'''
    \s*(
        >=|<=|==|=|!=|>|<|\(|\)|,|and|&&|or|\|\||not|!  # Operators & keywords
        | \+|\-|\/|\*                                   # Maths operators
        | [A-Za-z_][A-Za-z0-9_]*                        # Identifiers (Variables & Functions)
        | -?\d+\.\d+(?![A-Za-z_])                       # Floating-point numbers (supports negative)
        | -?\d+(?![A-Za-z_])                            # Integers (supports negative)
        | "[^"]*"                                       # Strings in double quotes
        | '[^']*'                                       # Strings in single quotes
        | true|false|True|False                         # Booleans
    )\s*
''', re.VERBOSE)


class Parser:
    def __init__(self) -> None:
        self._tokens: List[str] = []
        self._pos: int = 0

    def parse(self, expression: str) -> ExpressionNode:
        self._tokens = self.tokenize(expression)
        self._pos = 0
        node: ExpressionNode = self._parse_or()

        if self._pos < len(self._tokens):
            raise SyntaxError(f"Unexpected token '{self._tokens[self._pos]}' at position {self._pos}")
    
        return node

    def tokenize(self, expression: str) -> List[str]:
        tokens: List[str] = []
        pos: int = 0

        while pos < len(expression):
            match = TOKEN_REGEX.match(expression, pos)
            if not match:
                raise SyntaxError(f"Unrecognized token at position {pos}: '{expression[pos:]}'")
            
            token: str = match.group(0).strip()
            if token:
                tokens.append(token)

            pos = match.end()

        return tokens

    def _parse_or(self) -> ExpressionNode:
        node: ExpressionNode = self._parse_and()
        while self._match("or") or self._match("||"):
            node = OpOr(node, self._parse_and())
        return node

    def _parse_and(self) -> ExpressionNode:
        node: ExpressionNode = self._parse_binary_op()
        while self._match("and") or self._match("&&"):
            node = OpAnd(node, self._parse_binary_op())
        return node

    def _parse_math_add_sub(self) -> ExpressionNode:
        node: ExpressionNode = self._parse_math_mul_div()
        while self._match("+", "-"):
            op: str = self._previous() or ""
            if op=="+":
                node = OpPlus(node, self._parse_math_mul_div())
            else:
                node = OpMinus(node, self._parse_math_mul_div())
        return node

    def _parse_math_mul_div(self) -> ExpressionNode:
        node: ExpressionNode = self._parse_unary_op()
        while self._match("*", "/"):
            op: str = self._previous() or ""
            if op=="*":
                node = OpMultiply(node, self._parse_unary_op())
            else:
                node = OpDivide(node, self._parse_unary_op())          
        return node

    def _parse_binary_op(self) -> ExpressionNode:
        node: ExpressionNode = self._parse_math_add_sub()
        while self._match("==", "!=", ">", "<", ">=", "<=", "="):
            op: str = self._previous() or ""
            if op == "=" or op == "==":
                node = OpEquals(node, self._parse_math_add_sub())
            elif op == "!=":
                node = OpNotEquals(node, self._parse_math_add_sub())
            elif op == ">":
                node = OpGreaterThan(node, self._parse_math_add_sub())
            elif op == "<":
                node = OpLessThan(node, self._parse_math_add_sub())
            elif op == ">=":
                node = OpGreaterThanEquals(node, self._parse_math_add_sub())
            elif op == "<=":
                node = OpLessThanEquals(node, self._parse_math_add_sub())          
        return node

    def _parse_unary_op(self) -> ExpressionNode:
        if self._match("not") or self._match("!"):
            return OpNot(self._parse_unary_op())
        elif self._match("-"):
            return OpNegative(self._parse_unary_op())
        return self._parse_term()
    
    def _parse_string_literal(self) -> Optional[LiteralString]:
        string_val = self._peek()
        if string_val and ((string_val.startswith('"') and string_val.endswith('"')) or (string_val.startswith("'") and string_val.endswith("'"))):
            self._advance()
            return LiteralString(string_val[1:-1])
        return None
    
    def _parse_term(self) -> ExpressionNode:
        if self._match("("):
            node: ExpressionNode = self._parse_or()
            self._consume(")")
            return node
        elif self._match("true") or self._match("True"):
            return LiteralBoolean(True)
        elif self._match("false") or self._match("False"):
            return LiteralBoolean(False)
        elif re.match(r'^-?\d+(\.\d+)?$', self._peek() or ""):
            return LiteralNumber(self._advance() or "")
        
        string_val = self._parse_string_literal()
        if string_val is not None:
            return string_val
        
        identifier = self._match_identifier()
        if identifier:
            if self._match("("):
                args = []
                if not self._match(")"):
                    args.append(self._parse_or())
                    while self._match(","):
                        args.append(self._parse_or())
                    self._consume(")")
                return FunctionCall(identifier, args)
            return Variable(identifier)
        raise SyntaxError(f"Unexpected token: {self._peek()}")

    def _match(self, *expected_tokens: str) -> bool:
        if self._pos < len(self._tokens) and self._tokens[self._pos] in expected_tokens:
            self._pos += 1
            return True
        return False

    def _consume(self, expected_token: str) -> None:
        if self._match(expected_token):
            return
        if self._pos>=len(self._tokens):
            raise SyntaxError(f"Expected '{expected_token}' but expression ended.")
        raise SyntaxError(f"Expected '{expected_token}' but found '{self._peek()}'")

    def _peek(self) -> Optional[str]:
        return self._tokens[self._pos] if self._pos < len(self._tokens) else None

    def _previous(self) -> Optional[str]:
        return self._tokens[self._pos - 1] if self._pos > 0 else None

    def _advance(self) -> Optional[str]:
        if self._pos < len(self._tokens):
            self._pos += 1
            return self._tokens[self._pos - 1]
        return None

    def _expect(self, expected_token: str) -> str:
        token: Optional[str] = self._advance()
        if token != expected_token:
            raise SyntaxError(f"Expected '{expected_token}', but found '{token}'")
        return token

    def _match_identifier(self) -> Optional[str]:
        if self._pos < len(self._tokens) and re.match(r'^[A-Za-z_][A-Za-z0-9_]*$', self._tokens[self._pos]):
            token: str = self._tokens[self._pos]
            self._pos += 1
            return token
        return None