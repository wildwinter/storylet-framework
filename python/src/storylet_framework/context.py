# This file is part of an MIT-licensed project: see LICENSE file or README.md for details.
# Copyright (c) 2025 Ian Thomas

import sys
import os
from typing import Any, Dict, List, Optional

sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), "../../lib/expression-parser")))

from expression_parser.parser import Parser

expression_parser = Parser()

def eval_expression(val: Any, context: Dict[str, Any], dump_eval: Optional[List[str]] = None) -> Any:
    """
    Use the ExpressionParser to evaluate an expression
    Unless it's a simple bool or number, in which case just use that.
    dump_eval will fill an array with evaluation debug steps
    """
    if isinstance(val, (bool, int, float)):
        return val
    expression = expression_parser.parse(val)
    return expression.evaluate(context, dump_eval)


def init_context(context: Dict[str, Any], properties: Dict[str, Any], dump_eval: Optional[List[str]] = None) -> None:
    """
    Evaluate each expression in 'properties' and add it to the context.
    An error is raised if a property already exists in the context.
    """
    for prop_name, expr in properties.items():
        if dump_eval is not None:
            dump_eval.append(f"InitContext: Evaluating {prop_name} = {expr}")
        result = eval_expression(expr, context, dump_eval)
        if prop_name in context:
            raise ValueError(f"Trying to initialise property '{prop_name}' in context when it already exists.")
        context[prop_name] = result


def update_context(context: Dict[str, Any], updates: Dict[str, Any], dump_eval: Optional[List[str]] = None) -> None:
    """
    Evaluate any entries in updates and apply them to the context, but they must already exist
    dump_eval will fill an array with evaluation debug steps
    """
    for prop_name, expr in updates.items():
        if dump_eval is not None:
            dump_eval.append(f"UpdateContext: Evaluating {prop_name} = {expr}")
        result = eval_expression(expr, context, dump_eval)
        #print(f"Setting {prop_name} to {result}");
        if prop_name not in context:
            raise KeyError(f"Context var: '{prop_name}' undefined.")
        context[prop_name] = result


def dump_context(context: Dict[str, Any]) -> str:
    """
    For debugging - dump out the vars in the context.
    """
    lines = []
    for prop_name, expr in context.items():
        if callable(expr):
            lines.append(f"{prop_name} = <function>")
        elif isinstance(expr, (bool, int, float, str)):
            result = eval_expression(expr, context)
            lines.append(f"{prop_name} = {result}")
            
    return "\n".join(lines)