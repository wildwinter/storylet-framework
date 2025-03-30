// This file is part of an MIT-licensed project: see LICENSE file or README.md for details.
// Copyright (c) 2025 Ian Thomas

import { ExpressionParser } from '../lib/expression-parser/expressionParser.js';

const expressionParser = new ExpressionParser();

// Use the ExpressionParser to evaluate an expression
// Unless it's a simple bool or number, in which case just use that.
// dump_eval will fill an array with evaluation debug steps
export function evalExpression(val, context, dump_eval = null) {
  if (typeof val==="boolean"||typeof val==="number")
    return val;

  const expression = expressionParser.parse(val);
  return expression.evaluate(context, dump_eval);
}

// Evaluate and copy any new entries in 'properties' into the context
// dump_eval will fill an array with evaluation debug steps
export function initContext(context, properties, dump_eval = null) {
  for (const [propName, expression] of Object.entries(properties)) {
    if (dump_eval)
      dump_eval.push(`InitContext: Evaluating ${propName} = ${expression}`);
    const result = evalExpression(expression, context, dump_eval);
    if (propName in context)
      throw new Error(`Trying to initialise propert '${propName}' in context when it already exists.`)
    context[propName] = result;
  }
}

// Evaluate any entries in updates and apply them to the context, but they must already exist
// dump_eval will fill an array with evaluation debug steps
export function updateContext(context, updates, dump_eval = null) {
  for (const [propName, expression] of Object.entries(updates)) {
    if (dump_eval)
      dump_eval.push(`UpdateContext: Evaluating ${propName} = ${expression}`);
    const result = evalExpression(expression, context, dump_eval);
    //console.log(`Setting ${varName} to ${result}`);
    if (!(propName in context)) {
      throw new Error(`Context var: '${propName}' undefined.`);
    }
    context[propName] = result;
  }
}

// For debugging
export function dumpContext(context) {
  let out = [];
  for (const [propName, expression] of Object.entries(context)) {
    if (typeof expression === 'function') {
      out.push(`${propName} = <function>`);
    }
    else if (typeof expression==="boolean"||typeof expression==="number"||typeof expression==="string") {
      const result = evalExpression(expression, context);
      out.push(`${propName} = ${result}`);
    }
  }
  return out.join('\n');
}