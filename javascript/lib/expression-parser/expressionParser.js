// This file is part of an MIT-licensed project: see LICENSE file or README.md for details.
// Copyright (c) 2025 Ian Thomas

const STRING_FORMAT = Object.freeze({
    SINGLEQUOTE: 0,
    ESCAPED_SINGLEQUOTE: 1,
    DOUBLEQUOTE: 2,
    ESCAPED_DOUBLEQUOTE: 3,
  });
  
let _stringFormat = STRING_FORMAT.SINGLEQUOTE;

const Writer = {
    get StringFormat() {
        return _stringFormat;
    },
    set StringFormat(value) {
        _stringFormat = value;
    }
};

/*
 * This file is part of an MIT-licensed project: see LICENSE file or README.md for details.
 * Copyright (c) 2025 Ian Thomas
 */


class ExpressionNode {
  constructor(name, precedence) {
    if (new.target === ExpressionNode) {
      throw new TypeError("Cannot instantiate abstract class ExpressionNode directly");
    }
    this._name = name;
    this._precedence = precedence;
    this._specificity = 0;
  }

  get specificity() {return this._specificity;}

  evaluate(context, dump_eval) {
    throw new Error("Abstract method 'evaluate' not implemented");
  }

  dump_structure(indent = 0) {
    throw new Error("Abstract method 'dump_structure' not implemented");
  }

  write() {
    throw new Error("Abstract method 'write' not implemented");
  }
}

class BinaryOp extends ExpressionNode {
  constructor(name, left, op, right, precedence) {
    super(name, precedence);
    this._left = left;
    this._op = op;
    this._right = right;
    this._specificity = left.specificity + right.specificity;
  }

  evaluate(context, dump_eval) {
    const left_val = this._left.evaluate(context, dump_eval);

    const [shortCircuit, shortCircuitResult] = this._short_circuit(left_val);
    if (shortCircuit) {
      if (dump_eval) {
        dump_eval.push(`Evaluated: ${_format_value(left_val)} ${this._op} (ignore) = ${_format_value(shortCircuitResult)}`);
      }
      return shortCircuitResult;
    }

    const right_val = this._right.evaluate(context, dump_eval);
    const result = this._do_eval(left_val, right_val);

    if (dump_eval) {
      dump_eval.push(`Evaluated: ${_format_value(left_val)} ${this._op} ${_format_value(right_val)} = ${_format_value(result)}`);
    }
    return result;
  }

  _do_eval(left_val, right_val) {
    throw new Error("Abstract method '_do_eval' not implemented");
  }

  _short_circuit(left_val) {
    // By default, do not short-circuit.
    return [false, null];
  }

  dump_structure(indent = 0) {
    let out = "  ".repeat(indent) + `${this._name}\n`;
    out += this._left.dump_structure(indent + 1);
    out += this._right.dump_structure(indent + 1);
    return out;
  }

  write() {
    let left_str = this._left.write();
    let right_str = this._right.write();

    if (this._left._precedence < this._precedence) {
      left_str = `(${left_str})`;
    }
    if (this._right._precedence < this._precedence) {
      right_str = `(${right_str})`;
    }
    return `${left_str} ${this._op} ${right_str}`;
  }
}

class OpOr extends BinaryOp {
  constructor(left, right) {
    super("Or", left, "or", right, 40);
    this._specificity+=1;
  }
  _short_circuit(left_val) {
    const result = _make_bool(left_val);
    if (result)
      return [true, true];
    return [false, null];
  }
  _do_eval(left_val, right_val) {
    return _make_bool(left_val) || _make_bool(right_val);
  }
}

class OpAnd extends BinaryOp {
  constructor(left, right) {
    super("And", left, "and", right, 50);
    this._specificity+=1;
  }
  _short_circuit(left_val) {
    const result = _make_bool(left_val);
    if (!result)
      return [true, false];
    return [false, null];
  }
  _do_eval(left_val, right_val) {
    return _make_bool(left_val) && _make_bool(right_val);
  }
}

class OpEquals extends BinaryOp {
  constructor(left, right) {
    super("Equals", left, "==", right, 60);
  }
  _do_eval(left_val, right_val) {
    right_val = _make_type_match(left_val, right_val);
    return left_val === right_val;
  }
}

class OpNotEquals extends BinaryOp {
  constructor(left, right) {
    super("NotEquals", left, "!=", right, 60);
  }
  _do_eval(left_val, right_val) {
    right_val = _make_type_match(left_val, right_val);
    return left_val !== right_val;
  }
}

class OpPlus extends BinaryOp {
  constructor(left, right) {
    super("Plus", left, "+", right, 70);
  }
  _do_eval(left_val, right_val) {
    return _make_numeric(left_val) + _make_numeric(right_val);
  }
}

class OpMinus extends BinaryOp {
  constructor(left, right) {
    super("Minus", left, "-", right, 70);
  }
  _do_eval(left_val, right_val) {
    return _make_numeric(left_val) - _make_numeric(right_val);
  }
}

class OpDivide extends BinaryOp {
  constructor(left, right) {
    super("Divide", left, "/", right, 85);
  }
  _do_eval(left_val, right_val) {
    right_val = _make_numeric(right_val);
    if (right_val === 0) {
      throw new Error("Division by zero.");
    }
    return _make_numeric(left_val) / right_val;
  }
}

class OpMultiply extends BinaryOp {
  constructor(left, right) {
    super("Multiply", left, "*", right, 80);
  }
  _short_circuit(left_val) {
    const result = _make_numeric(left_val);
    if (result==0)
      return [true, 0];
    return [false, null];
  }
  _do_eval(left_val, right_val) {
    return _make_numeric(left_val) * _make_numeric(right_val);
  }
}

class OpGreaterThan extends BinaryOp {
  constructor(left, right) {
    super("GreaterThan", left, ">", right, 60);
  }
  _do_eval(left_val, right_val) {
    return _make_numeric(left_val) > _make_numeric(right_val);
  }
}

class OpLessThan extends BinaryOp {
  constructor(left, right) {
    super("LessThan", left, "<", right, 60);
  }
  _do_eval(left_val, right_val) {
    return _make_numeric(left_val) < _make_numeric(right_val);
  }
}

class OpGreaterThanEquals extends BinaryOp {
  constructor(left, right) {
    super("GreaterThanEquals", left, ">=", right, 60);
  }
  _do_eval(left_val, right_val) {
    return _make_numeric(left_val) >= _make_numeric(right_val);
  }
}

class OpLessThanEquals extends BinaryOp {
  constructor(left, right) {
    super("LessThanEquals", left, "<=", right, 60);
  }
  _do_eval(left_val, right_val) {
    return _make_numeric(left_val) <= _make_numeric(right_val);
  }
}

class UnaryOp extends ExpressionNode {
  constructor(name, op, operand, precedence) {
    super(name, precedence);
    this._operand = operand;
    this._op = op;
    this._specificity = operand.specificity;
  }

  evaluate(context, dump_eval) {
    const val = this._operand.evaluate(context, dump_eval);
    const result = this._do_eval(val);
    if (dump_eval) {
      dump_eval.push(`Evaluated: ${this._op} ${_format_value(val)} = ${_format_value(result)}`);
    }
    return result;
  }

  _do_eval(val) {
    throw new Error("Abstract method '_do_eval' not implemented");
  }

  dump_structure(indent = 0) {
    let out = "  ".repeat(indent) + `${this._name}\n`;
    out += this._operand.dump_structure(indent + 1);
    return out;
  }

  write() {
    let operand_str = this._operand.write();
    if (this._operand._precedence < this._precedence) {
      operand_str = `(${operand_str})`;
    }
    return `${this._op} ${operand_str}`;
  }
}

class OpNegative extends UnaryOp {
  constructor(operand) {
    super("Negative", "-", operand, 90);
  }
  _do_eval(val) {
    return -_make_numeric(val);
  }
}

class OpNot extends UnaryOp {
  constructor(operand) {
    super("Not", "not", operand, 90);
  }
  _do_eval(val) {
    return !_make_bool(val);
  }
}

class LiteralBoolean extends ExpressionNode {
  constructor(value) {
    super("Boolean", 100);
    this._value = value;
  }
  evaluate(context, dump_eval) {
    if (dump_eval) {
      dump_eval.push(`Boolean: ${_format_value(this._value)}`);
    }
    return this._value;
  }
  dump_structure(indent = 0) {
    return "  ".repeat(indent) + `Boolean(${_format_value(this._value)})\n`;
  }
  write() {
    return _format_value(this._value);
  }
}

class LiteralNumber extends ExpressionNode {
  constructor(value) {
    super("Number", 100);
    // Assuming value is provided as a string
    this._value = parseFloat(value);
  }
  evaluate(context, dump_eval) {
    if (dump_eval) {
      dump_eval.push(`Number: ${this._value}`);
    }
    return this._value;
  }
  dump_structure(indent = 0) {
    return "  ".repeat(indent) + `Number(${this._value})\n`;
  }
  write() {
    return `${this._value}`;
  }
}

class LiteralString extends ExpressionNode {
  constructor(value) {
    super("String", 100);
    this._value = value;
  }
  evaluate(context, dump_eval) {
    if (dump_eval) {
      dump_eval.push(`String: ${_format_string(this._value)}`);
    }
    return this._value;
  }
  dump_structure(indent = 0) {
    return "  ".repeat(indent) + `String(${_format_string(this._value)})\n`;
  }
  write() {
    return _format_string(this._value);
  }
}

class Variable extends ExpressionNode {
  constructor(name) {
    super("Variable", 100);
    this._name = name;
  }
  evaluate(context, dump_eval) {
    const value = context[this._name];
    if (value === undefined) {
      throw new Error(`Variable '${this._name}' not found in context.`);
    }
    if (typeof value !== "number" && typeof value !== "boolean" && typeof value !== "string") {
      throw new TypeError(`Variable '${this._name}' must return bool, string, or numeric.`);
    }
    if (dump_eval) {
      dump_eval.push(`Fetching variable: ${this._name} -> ${_format_value(value)}`);
    }
    return value;
  }
  dump_structure(indent = 0) {
    return "  ".repeat(indent) + `Variable(${this._name})\n`;
  }
  write() {
    return this._name;
  }
}

class FunctionCall extends ExpressionNode {
  constructor(func_name, args = []) {
    super("FunctionCall", 100);
    this._func_name = func_name;
    this._args = args;
  }
  evaluate(context, dump_eval) {
    const func = context[this._func_name];
    if (func === undefined) {
      throw new Error(`Function '${this._func_name}' not found in context.`);
    }
    const arg_values = this._args.map(arg => arg.evaluate(context, dump_eval));

    // Check function arity using func.length
    if (arg_values.length !== func.length) {
      const formattedArgs = arg_values.map(val => _format_value(val)).join(", ");
      throw new Error(`Function '${this._func_name}' does not support the provided arguments (${formattedArgs}).`);
    }

    const result = func(...arg_values);

    if (typeof result !== "number" && typeof result !== "boolean" && typeof result !== "string") {
      throw new TypeError(`Function '${this._func_name}' must return bool, string, or numeric.`);
    }
    if (dump_eval) {
      const formattedArgs = arg_values.map(val => _format_value(val)).join(", ");
      dump_eval.push(`Called function: ${this._func_name}(${formattedArgs}) = ${_format_value(result)}`);
    }
    return result;
  }
  dump_structure(indent = 0) {
    let out = "  ".repeat(indent) + `FunctionCall(${this._func_name})\n`;
    for (const arg of this._args) {
      out += arg.dump_structure(indent + 1);
    }
    return out;
  }
  write() {
    const written_args = this._args.map(arg => arg.write());
    return `${this._func_name}(${written_args.join(", ")})`;
  }
}

function _make_bool(val) {
  if (typeof val === "boolean") {
    return val;
  }
  if (typeof val === "number") {
    return val !== 0;
  }
  if (typeof val === "string") {
    return val.toLowerCase() === "true" || val === "1";
  }
  throw new TypeError(`Type mismatch: Expecting bool, but got '${val}'`);
}

function _make_str(val) {
  if (typeof val === "string") {
    return val;
  }
  if (typeof val === "boolean") {
    return val ? "true" : "false";
  }
  if (typeof val === "number") {
    return val.toString();
  }
  throw new TypeError(`Type mismatch: Expecting string but got '${val}'`);
}

function _make_numeric(val) {
  if (typeof val === "boolean") {
    return val ? 1 : 0;
  }
  if (typeof val === "number") {
    return val;
  }
  if (typeof val === "string") {
    const numericRegex = /^-?\d+(\.\d+)?$/;
    if (numericRegex.test(val)) {
      return parseFloat(val);
    }
  }
  throw new TypeError(`Type mismatch: Expecting number but got '${val}'`);
}

function _make_type_match(left_val, right_val) {
  if (typeof left_val === "boolean") {
    return _make_bool(right_val);
  }
  if (typeof left_val === "number") {
    return _make_numeric(right_val);
  }
  if (typeof left_val === "string") {
    return _make_str(right_val);
  }
  throw new TypeError(`Type mismatch: unrecognised type for '${left_val}'`);
}

function _format_string(val) {
  switch (Writer.StringFormat) {
    case STRING_FORMAT.SINGLEQUOTE:
      return `'${val}'`;
    case STRING_FORMAT.ESCAPED_SINGLEQUOTE:
      return `\\'${val}\\'`;
    case STRING_FORMAT.ESCAPED_DOUBLEQUOTE:
      return `\\"${val}\\"`;
    case STRING_FORMAT.DOUBLEQUOTE:
    default:
      return `"${val}"`;
  }
}

function _format_value(val) {
  if (typeof val === "string") {
    return _format_string(val);
  }
  return val.toString();
}

// This file is part of an MIT-licensed project: see LICENSE file or README.md for details.
// Copyright (c) 2025 Ian Thomas

  
  // A JavaScript regular expression for tokenizing the expression.
  // The pattern below mirrors the Python TOKEN_REGEX from your parser.
  // Note: The "g" flag (global) is used for repeated matching.
  const TOKEN_REGEX = /\s*(>=|<=|==|=|!=|>|<|\(|\)|,|and|&&|or|\|\||not|!|\+|\-|\/|\*|[A-Za-z_][A-Za-z0-9_]*|-?\d+\.\d+|-?\d+|"[^"]*"|'[^']*'|true|false|True|False)\s*/g;
  
  class ExpressionParser {
    constructor() {
      this._tokens = [];
      this._pos = 0;
    }
  
    /**
     * Parses the given expression string into an AST (ExpressionNode).
     * @param {string} expression 
     * @returns {ExpressionNode}
     */
    parse(expression) {
      this._tokens = this.tokenize(expression);
      this._pos = 0;
      const node = this._parse_or();
  
      if (this._pos < this._tokens.length) {
        throw new SyntaxError(
          `Unexpected token '${this._tokens[this._pos]}' at position ${this._pos}`
        );
      }
      return node;
    }
  
    /**
     * Tokenizes the input expression string.
     * @param {string} expression 
     * @returns {string[]}
     */
    tokenize(expression) {
        const matches = expression.matchAll(TOKEN_REGEX);
        const tokens = [];
        for (const match of matches) {
          // match[1] contains the captured token (without surrounding whitespace)
          if (match[1]) {
            tokens.push(match[1]);
          }
        }

        if (tokens.length === 0) {
          throw new SyntaxError(`No tokens were recognized in expression: '${expression}'`);
        }
        return tokens;
      }

    _parse_or() {
      let node = this._parse_and();
      while (this._match("or") || this._match("||")) {
        node = new OpOr(node, this._parse_and());
      }
      return node;
    }
  
    _parse_and() {
      let node = this._parse_binary_op();
      while (this._match("and") || this._match("&&")) {
        node = new OpAnd(node, this._parse_binary_op());
      }
      return node;
    }
  
    _parse_math_add_sub() {
      let node = this._parse_math_mul_div();
      while (this._match("+", "-")) {
        const op = this._previous() || "";
        if (op === "+") {
          node = new OpPlus(node, this._parse_math_mul_div());
        } else {
          node = new OpMinus(node, this._parse_math_mul_div());
        }
      }
      return node;
    }
  
    _parse_math_mul_div() {
      let node = this._parse_unary_op();
      while (this._match("*", "/")) {
        const op = this._previous() || "";
        if (op === "*") {
          node = new OpMultiply(node, this._parse_unary_op());
        } else {
          node = new OpDivide(node, this._parse_unary_op());
        }
      }
      return node;
    }
  
    _parse_binary_op() {
      let node = this._parse_math_add_sub();
      while (this._match("==", "!=", ">", "<", ">=", "<=", "=")) {
        const op = this._previous() || "";
        if (op === "=" || op === "==") {
          node = new OpEquals(node, this._parse_math_add_sub());
        } else if (op === "!=") {
          node = new OpNotEquals(node, this._parse_math_add_sub());
        } else if (op === ">") {
          node = new OpGreaterThan(node, this._parse_math_add_sub());
        } else if (op === "<") {
          node = new OpLessThan(node, this._parse_math_add_sub());
        } else if (op === ">=") {
          node = new OpGreaterThanEquals(node, this._parse_math_add_sub());
        } else if (op === "<=") {
          node = new OpLessThanEquals(node, this._parse_math_add_sub());
        }
      }
      return node;
    }
  
    _parse_unary_op() {
      if (this._match("not") || this._match("!")) {
        return new OpNot(this._parse_unary_op());
      } else if (this._match("-")) {
        return new OpNegative(this._parse_unary_op());
      }
      return this._parse_term();
    }
  
    _parse_string_literal() {
      const stringVal = this._peek();
      if (
        stringVal &&
        ((stringVal.startsWith('"') && stringVal.endsWith('"')) ||
          (stringVal.startsWith("'") && stringVal.endsWith("'")))
      ) {
        this._advance();
        return new LiteralString(stringVal.slice(1, -1));
      }
      return null;
    }
  
    _parse_term() {
      if (this._match("(")) {
        const node = this._parse_or();
        this._consume(")");
        return node;
      } else if (this._match("true") || this._match("True")) {
        return new LiteralBoolean(true);
      } else if (this._match("false") || this._match("False")) {
        return new LiteralBoolean(false);
      } else if (/^-?\d+(\.\d+)?$/.test(this._peek() || "")) {
        return new LiteralNumber(this._advance() || "");
      }
  
      const stringLiteral = this._parse_string_literal();
      if (stringLiteral !== null) {
        return stringLiteral;
      }
  
      const identifier = this._match_identifier();
      if (identifier) {
        if (this._match("(")) {
          const args = [];
          if (!this._match(")")) {
            args.push(this._parse_or());
            while (this._match(",")) {
              args.push(this._parse_or());
            }
            this._consume(")");
          }
          return new FunctionCall(identifier, args);
        }
        return new Variable(identifier);
      }
      throw new SyntaxError(`Unexpected token: ${this._peek()}`);
    }
  
    _match(...expectedTokens) {
      if (this._pos < this._tokens.length && expectedTokens.includes(this._tokens[this._pos])) {
        this._pos++;
        return true;
      }
      return false;
    }
  
    _consume(expectedToken) {
      if (this._match(expectedToken)) {
        return;
      }
      if (this._pos >= this._tokens.length) {
        throw new SyntaxError(`Expected '${expectedToken}' but expression ended.`);
      }
      throw new SyntaxError(`Expected '${expectedToken}' but found '${this._peek()}'`);
    }
  
    _peek() {
      return this._pos < this._tokens.length ? this._tokens[this._pos] : null;
    }
  
    _previous() {
      return this._pos > 0 ? this._tokens[this._pos - 1] : null;
    }
  
    _advance() {
      if (this._pos < this._tokens.length) {
        this._pos++;
        return this._tokens[this._pos - 1];
      }
      return null;
    }
  
    _expect(expectedToken) {
      const token = this._advance();
      if (token !== expectedToken) {
        throw new SyntaxError(`Expected '${expectedToken}', but found '${token}'`);
      }
      return token;
    }
  
    _match_identifier() {
      if (this._pos < this._tokens.length && /^[A-Za-z_][A-Za-z0-9_]*$/.test(this._tokens[this._pos])) {
        const token = this._tokens[this._pos];
        this._pos++;
        return token;
      }
      return null;
    }
  }

export { BinaryOp, ExpressionNode, ExpressionParser, FunctionCall, LiteralBoolean, LiteralNumber, LiteralString, OpAnd, OpDivide, OpEquals, OpGreaterThan, OpGreaterThanEquals, OpLessThan, OpLessThanEquals, OpMinus, OpMultiply, OpNegative, OpNot, OpNotEquals, OpOr, OpPlus, STRING_FORMAT, UnaryOp, Variable, Writer };
//# sourceMappingURL=expressionParser.js.map
