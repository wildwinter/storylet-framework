# This file is part of an MIT-licensed project: see LICENSE file or README.md for details.
# Copyright (c) 2025 Ian Thomas

import inspect
from abc import abstractmethod
from typing import Any, Dict, List, Optional, Union
from .writer import Writer, STRING_FORMAT_SINGLEQUOTE, STRING_FORMAT_ESCAPED_SINGLEQUOTE, STRING_FORMAT_DOUBLEQUOTE, STRING_FORMAT_ESCAPED_DOUBLEQUOTE

class ExpressionNode:
    @abstractmethod
    def __init__(self, name: str, precedence: int) -> None:
        self._name = name
        self._precedence = precedence

    @abstractmethod
    def evaluate(self, context: Dict[str, Any], dump_eval: Optional[List[str]] = None) -> Any:
        raise NotImplementedError()

    @abstractmethod
    def dump_structure(self, indent: int = 0) -> str:
        raise NotImplementedError()
    
    @abstractmethod
    def write(self) -> str:
        raise NotImplementedError()
    

class BinaryOp(ExpressionNode):
    @abstractmethod
    def __init__(self, name: str, left: ExpressionNode, op: str, right: ExpressionNode, precedence: int) -> None:
        super().__init__(name, precedence)
        self._left = left
        self._op = op
        self._right = right
    
    def evaluate(self, context: Dict[str, Any], dump_eval: Optional[List[str]] = None) -> Any:
        left_val = self._left.evaluate(context, dump_eval)
        
        short_circuit, short_circuit_result = self._short_circuit(left_val)
        if short_circuit:
            if dump_eval is not None:
                dump_eval.append(f"Evaluated: {_format_value(left_val)} {self._op} (ignore) = {_format_value(short_circuit_result)}")

            return short_circuit_result
        
        right_val = self._right.evaluate(context, dump_eval)
        result: Any = self._do_eval(left_val, right_val)
        
        if dump_eval is not None:
            dump_eval.append(f"Evaluated: {_format_value(left_val)} {self._op} {_format_value(right_val)} = {_format_value(result)}")

        return result
    
    @abstractmethod
    def _do_eval(self, left_val: Any, right_val: Any) -> Any:
        raise NotImplementedError()
    
    def _short_circuit(self, left_val: Any) -> tuple[bool, Any]:
        return [False, None]

    def dump_structure(self, indent: int = 0) -> str:
        out = ("  " * indent + f"{self._name}") + "\n"
        out += self._left.dump_structure(indent + 1)
        out += self._right.dump_structure(indent + 1)
        return out
    
    def write(self) -> str:
        left_str = self._left.write()
        right_str = self._right.write()

        if self._left._precedence < self._precedence:
            left_str = f"({left_str})"

        if self._right._precedence < self._precedence:
            right_str = f"({right_str})"

        return f"{left_str} {self._op} {right_str}"
    

class OpOr(BinaryOp):
    def __init__(self, left: ExpressionNode, right: ExpressionNode) -> None:
        super().__init__("Or", left, "or", right, 40)

    def _short_circuit(self, left_val: Any) -> tuple[bool, Any]:
        result = _make_bool(left_val)
        if result:
            return [True, True]
        return [False, None]

    def _do_eval(self, left_val: Any, right_val: Any) -> Any: 
        return _make_bool(left_val) or _make_bool(right_val)
    

class OpAnd(BinaryOp):
    def __init__(self, left: ExpressionNode, right: ExpressionNode) -> None:
        super().__init__("And", left, "and", right, 50)

    def _short_circuit(self, left_val: Any) -> tuple[bool, Any]:
        result = _make_bool(left_val)
        if not result:
            return [True, False]
        return [False, None]
    
    def _do_eval(self, left_val: Any, right_val: Any) -> Any: 
        return _make_bool(left_val) and _make_bool(right_val)
    

class OpEquals(BinaryOp):
    def __init__(self, left: ExpressionNode, right: ExpressionNode) -> None:
        super().__init__("Equals", left, "==", right, 60)

    def _do_eval(self, left_val: Any, right_val: Any) -> Any: 
        right_val = _make_type_match(left_val, right_val)
        return left_val == right_val


class OpNotEquals(BinaryOp):
    def __init__(self, left: ExpressionNode, right: ExpressionNode) -> None:
        super().__init__("NotEquals", left, "!=", right, 60)

    def _do_eval(self, left_val: Any, right_val: Any) -> Any: 
        right_val = _make_type_match(left_val, right_val)
        return left_val != right_val


class OpPlus(BinaryOp):
    def __init__(self, left: ExpressionNode, right: ExpressionNode) -> None:
        super().__init__("Plus", left, "+", right, 70)

    def _do_eval(self, left_val: Any, right_val: Any) -> Any: 
        return _make_numeric(left_val) + _make_numeric(right_val)


class OpMinus(BinaryOp):
    def __init__(self, left: ExpressionNode, right: ExpressionNode) -> None:
        super().__init__("Minus", left, "-", right, 70)

    def _do_eval(self, left_val: Any, right_val: Any) -> Any: 
        return _make_numeric(left_val) - _make_numeric(right_val)
    

class OpDivide(BinaryOp):
    def __init__(self, left: ExpressionNode, right: ExpressionNode) -> None:
        super().__init__("Divide", left, "/", right, 85)

    def _do_eval(self, left_val: Any, right_val: Any) -> Any: 
        right_val = _make_numeric(right_val)
        if right_val == 0:
            raise ZeroDivisionError(f"Division by zero.")
        return _make_numeric(left_val) / right_val


class OpMultiply(BinaryOp):
    def __init__(self, left: ExpressionNode, right: ExpressionNode) -> None:
        super().__init__("Multiply", left, "*", right, 80)

    def _short_circuit(self, left_val: Any) -> tuple[bool, Any]:
        result = _make_numeric(left_val)
        if result==0:
            return [True, 0]
        return [False, None]
    
    def _do_eval(self, left_val: Any, right_val: Any) -> Any: 
        return _make_numeric(left_val) * _make_numeric(right_val)
    

class OpGreaterThan(BinaryOp):
    def __init__(self, left: ExpressionNode, right: ExpressionNode) -> None:
        super().__init__("GreaterThan", left, ">", right, 60)

    def _do_eval(self, left_val: Any, right_val: Any) -> Any: 
        return _make_numeric(left_val) > _make_numeric(right_val)


class OpLessThan(BinaryOp):
    def __init__(self, left: ExpressionNode, right: ExpressionNode) -> None:
        super().__init__("LessThan", left, "<", right, 60)

    def _do_eval(self, left_val: Any, right_val: Any) -> Any: 
        return _make_numeric(left_val) < _make_numeric(right_val)


class OpGreaterThanEquals(BinaryOp):
    def __init__(self, left: ExpressionNode, right: ExpressionNode) -> None:
        super().__init__("GreaterThanEquals", left, ">=", right, 60)
    
    def _do_eval(self, left_val: Any, right_val: Any) -> Any: 
        return _make_numeric(left_val) >= _make_numeric(right_val)
    
    
class OpLessThanEquals(BinaryOp):
    def __init__(self, left: ExpressionNode, right: ExpressionNode) -> None:
        super().__init__("LessThanEquals", left, "<=", right, 60)

    def _do_eval(self, left_val: Any, right_val: Any) -> Any: 
        return _make_numeric(left_val) <= _make_numeric(right_val)
    

class UnaryOp(ExpressionNode):
    @abstractmethod
    def __init__(self, name: str, op: str, operand: ExpressionNode, precedence: int) -> None:
        super().__init__(name, precedence)
        self._operand = operand
        self._op = op

    def evaluate(self, context: Dict[str, Any], dump_eval: Optional[List[str]] = None) -> Any:
        val = self._operand.evaluate(context, dump_eval)
        result: Any = self._do_eval(val)
        
        if dump_eval is not None:
            dump_eval.append(f"Evaluated: {self._op} {_format_value(val)} = {_format_value(result)}")
        
        return result
    
    @abstractmethod
    def _do_eval(self, val: Any) -> Any:
        raise NotImplementedError()

    def dump_structure(self, indent: int = 0) -> str:
        out = ("  " * indent + f"{self._name}") + "\n"
        out += self._operand.dump_structure(indent + 1)
        return out
    
    def write(self) -> str:
        operand_str = self._operand.write()

        if self._operand._precedence < self._precedence:
            operand_str = f"({operand_str})"
    
        return f"{self._op} {operand_str}"
    

class OpNegative(UnaryOp):
    def __init__(self, operand: ExpressionNode) -> None:
        super().__init__("Negative", "-", operand, 90)

    def _do_eval(self, val: Any) -> Any:
        val = _make_numeric(val)
        return -val


class OpNot(UnaryOp):
    def __init__(self, operand: ExpressionNode) -> None:
        super().__init__("Not", "not", operand, 90)

    def _do_eval(self, val: Any) -> Any:
        val = _make_bool(val)
        return not val


class LiteralBoolean(ExpressionNode):
    def __init__(self, value: bool) -> None:
        super().__init__("Boolean", 100)
        self._value = value

    def evaluate(self, context: Dict[str, Any], dump_eval: Optional[List[str]] = None) -> bool:
        if dump_eval is not None:
            dump_eval.append(f"Boolean: {_format_boolean(self._value)}")
        return self._value

    def dump_structure(self, indent: int = 0) -> str:
        return ("  " * indent + f"Boolean({_format_boolean(self._value)})") + "\n"
    
    def write(self) -> str:
        return _format_boolean(self._value)


class LiteralNumber(ExpressionNode):
    def __init__(self, value: str) -> None:
        super().__init__("Number", 100)
        self._value = float(value)

    def evaluate(self, context: Dict[str, Any], dump_eval: Optional[List[str]] = None) -> float:
        if dump_eval is not None:
            dump_eval.append(f"Number: {_format_numeric(self._value)}")
        return self._value

    def dump_structure(self, indent: int = 0) -> str:
        return ("  " * indent + f"Number({_format_numeric(self._value)})") + "\n"

    def write(self) -> str:
        return _format_numeric(self._value)
    

class LiteralString(ExpressionNode):
    def __init__(self, value: str) -> None:
        super().__init__("String", 100)
        self._value = value

    def evaluate(self, context: Dict[str, Any], dump_eval: Optional[List[str]] = None) -> str:
        if dump_eval is not None:
            dump_eval.append(f"String: {_format_string(self._value)}")
        return self._value

    def dump_structure(self, indent: int = 0) -> str:
        return ("  " * indent + f"String({_format_string(self._value)})") + "\n"

    def write(self) -> str:
        return _format_string(self._value)
    
    
class Variable(ExpressionNode):
    def __init__(self, name: str) -> None:
        super().__init__("Variable", 100)
        self._name = name

    def evaluate(self, context: Dict[str, Any], dump_eval: Optional[List[str]] = None) -> Any:
        value = context.get(self._name)
        if value is None:
            raise RuntimeError(f"Variable '{self._name}' not found in context.")
        if not isinstance(value, (int, float, bool, str)):
            raise TypeError(f"Variable '{self._name}' must return bool, string, or numeric.")
        
        if dump_eval is not None:
            dump_eval.append(f"Fetching variable: {self._name} -> {_format_value(value)}")
        return value

    def dump_structure(self, indent: int = 0) -> str:
        return ("  " * indent + f"Variable({self._name})") + "\n"
    
    def write(self) -> str:
        return self._name
    

class FunctionCall(ExpressionNode):
    def __init__(self, func_name: str, args: Optional[List[ExpressionNode]] = None) -> None:
        super().__init__("FunctionCall", 100)
        if args is None:
            args = []
        self._func_name = func_name
        self._args = args

    def evaluate(self, context: Dict[str, Any], dump_eval: Optional[List[str]] = None) -> Any:
        func = context.get(self._func_name)
        if func is None:
            raise RuntimeError(f"Function '{self._func_name}' not found in context.")
        
        arg_values: List[Any] = [arg.evaluate(context, dump_eval) for arg in self._args]

        # Get the function signature and check if it accepts the provided arguments.
        sig = inspect.signature(func)
        try:
            sig.bind(*arg_values)
        except TypeError as e:
            formatted_args = ", ".join(_format_value(val) for val in arg_values)
            raise RuntimeError(f"Function '{self._func_name}' does not support the provided arguments ({formatted_args}).")

        result = func(*arg_values)

        if not isinstance(result, (int, float, bool, str)):
            raise TypeError(f"Function '{self._func_name}' must return bool, string, or numeric.")
        
        if dump_eval is not None:
            formatted_args = ", ".join(_format_value(val) for val in arg_values)
            dump_eval.append(f"Called function: {self._func_name}({formatted_args}) = {_format_value(result)}")

        return result

    def dump_structure(self, indent: int = 0) -> str:
        out = ("  " * indent + f"FunctionCall({self._func_name})") + "\n"
        for arg in self._args:
            out += arg.dump_structure(indent + 1)
        return out
    
    def write(self) -> str:
        out = self._func_name+"("
        written_args = []
        for arg in self._args:
            written_args.append(arg.write())
        out += ", ".join(written_args)
        out += ")"
        return out
    

def _make_bool(val: Any) -> bool:
    if isinstance(val, bool):
        return val
    if isinstance(val, (int, float)):
        return val != 0
    if isinstance(val, str):
        return val.lower() == "true" or val == "1"
    raise TypeError(f"Type mismatch: Expecting bool, but got '{val}'")


def _make_str(val: Any) -> str:
    if isinstance(val, str):
        return val
    if isinstance(val, bool):
        return "true" if val else "false"
    if isinstance(val, (int, float)):
        return str(val)
    raise TypeError(f"Type mismatch: Expecting string but got '{val}'")


def _make_numeric(val: Any) -> float:
    if isinstance(val, bool):
        return 1 if val else 0
    if isinstance(val, (float, int)):
        return float(val)
    if isinstance(val, str):
        try:
            return float(val)
        except ValueError:
            pass
    raise TypeError(f"Type mismatch: Expecting number but got '{val}'")


def _make_type_match(left_val: Any, right_val: Any) -> Any:
    if isinstance(left_val, bool):
        return _make_bool(right_val)
    if isinstance(left_val, (int, float)):
        return _make_numeric(right_val)
    if isinstance(left_val, str):
        return _make_str(right_val)
    raise TypeError(f"Type mismatch: unrecognised type for '{left_val}'")


def _format_boolean(val: bool) -> str:
    return "true" if val else "false"

def _format_numeric(num: Union[int, float]) -> str:
    if isinstance(num, float) and int(num) == num:
        return str(int(num))
    return str(num)

def _format_string(val: str) -> str:
    if Writer.string_format==STRING_FORMAT_SINGLEQUOTE:
        return f"'{val}'"
    if Writer.string_format==STRING_FORMAT_ESCAPED_SINGLEQUOTE:
        return f"\\'{val}\\'"
    if Writer.string_format==STRING_FORMAT_ESCAPED_DOUBLEQUOTE:
        return f"\\\"{val}\\\""
    return f"\"{val}\""

def _format_value(val: Any) -> str:
    if isinstance(val, bool):
        return _format_boolean(val)
    
    if isinstance(val, (int, float)):
        return _format_numeric(val)
    
    if isinstance(val, str):
        return _format_string(val)
    
    return str(val)