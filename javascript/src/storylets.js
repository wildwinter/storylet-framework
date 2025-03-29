// This file is part of an MIT-licensed project: see LICENSE file or README.md for details.
// Copyright (c) 2025 Ian Thomas

import { ExpressionParser } from '../lib/expression-parser/expressionParser.js';
import { shuffleArray } from "./utils.js";

const expressionParser = new ExpressionParser();
const USE_SPECIFICITY = true;

export function evalExpression(val, context) {

  if (typeof val==="boolean"||typeof val==="number")
    return val;

  const expression = expressionParser.parse(val);
  return expression.evaluate(context);
}

export function initContext(context, vars) {
  for (const [varName, value] of Object.entries(vars)) {
    const result = evalExpression(value, context);
    if (!(varName in context)) {
      context[varName] = result;
    }
  }
}

export function updateContext(context, vars) {
  for (const [varName, value] of Object.entries(vars)) {
    const result = evalExpression(value, context);
    console.log(`Setting ${varName} to ${result}`);
    if (!(varName in context)) {
      throw new Error(`Context var: '${varName}' undefined.`);
    }
    context[varName] = result;
  }
}

const REDRAW_ALWAYS = 0;
const REDRAW_NEVER = -1;

export class Storylet {

  constructor(id) {
    this.id = id;
    this.condition = "";
    this.content = {};
    this.redraw = REDRAW_ALWAYS;
    this.updateOnDrawn = null;

    this._compiledCondition = null;
    this._priority = 0;
    this._nextDraw = 0;
  }

  compileCondition() {
    this._compiledCondition = null;
    if (this.condition!="") {
      this._compiledCondition = expressionParser.parse(this.condition);
    }
  }

  checkCondition(context) {
    if (!this._compiledCondition)
      return true;
    return this._compiledCondition.evaluate(context);
  }

  set priority(numOrExpression) { this._priority = numOrExpression;}

  calcCurrentPriority(context) {
    let workingPriority = + evalExpression(this._priority, context);
    if (USE_SPECIFICITY) {
      workingPriority = workingPriority*100;
      if (this._compiledCondition!=null) {
        workingPriority += this._compiledCondition.specificity;
      }
    }
    return workingPriority;
  }

  canDraw(currentDraw) {
    if (this.redraw == REDRAW_NEVER && this._nextDraw<0)
      return false;
    if (this.redraw == REDRAW_ALWAYS)
      return true;
    return currentDraw>=this._nextDraw;
  }

  drawn(currentDraw) {
    if (this.redraw == REDRAW_NEVER) {
      this._nextDraw = -1;
      return;
    }
    this._nextDraw = currentDraw + this.redraw;
  }

  static fromJson(json) {

    if (!("id" in json))
      throw new Error("No 'id' property in the storylet JSON.", json);

    const storylet = new Storylet(json.id);

    if ("redraw" in json) {
      let val = json.redraw;
      if (val=="always")
        storylet.redraw = REDRAW_ALWAYS;
      else if (val=="never")
        storylet.redraw = REDRAW_NEVER;
      else
        storylet.redraw = parseInt(val);
    }

    if ("condition" in json) {
      storylet.condition = json.condition;
      storylet.compileCondition();
    }
    if ("priority" in json) {
      storylet.priority = json.priority;
    }
    if ("updateOnDrawn" in json) {
      storylet.updateOnDrawn = json.updateOnDrawn;
    }
    if ("content" in json) {
      storylet.content = json.content;
    }
    return storylet;

  }

}

export class Deck {

  constructor(context = {}) {
    this._all = new Map();
    this._drawPile = [];
    this._currentDraw = 0;
    this._context = context;
  }

  static fromJson(json, context = {}) {
    const deck = new Deck(context);
    deck.loadJson(json);
    return deck
  }

  loadJson(json) {
    for (const item of json) {

      // Initialiser
      if ("context" in item) {
        initContext(this._context, item.context);
        continue;
      }

      const storylet = Storylet.fromJson(item);
      if (this._all.has(storylet.id)) {
        throw new Error(`Duplicate storylet id: '${storylet.id}'.`, item);
      }
      this._all.set(storylet.id, storylet);
    }
  }

  reset() {
    this.currentDraw = 0;
  }

  refresh(filter=null) {
    this._drawPile = [];
    const priorityAvailable = new Map();
    const toProcess = [...this._all.values()];

    while (toProcess.length>0) {
      
      const storylet = toProcess.shift();

      if (!storylet.canDraw(this._currentDraw))
        continue;
      
      // Apply filter, if available
      if (filter!=null) {
        if (!filter(storylet)) {
          continue;
        }
      }

      if (!storylet.checkCondition(this._context)) {
        //console.log(`Storylet '${storylet.id}': condition '${storylet.condition}' didn't pass.`);
        continue;
      }

      let priority = storylet.calcCurrentPriority(this._context);
      if (!priorityAvailable.has(priority))
        priorityAvailable.set(priority, []);
      priorityAvailable.get(priority).push(storylet);
    }

    const sortedPriorities = [...priorityAvailable.keys()].sort((a, b) => b - a);
    for (const priority of sortedPriorities) {
      const bucket = priorityAvailable.get(priority);
      const shuffledBucket = shuffleArray(bucket);
      this._drawPile.push(...shuffledBucket);
    }

  }

  dumpDrawPile() {
    let ids = [];
    for (const storylet of this._drawPile) {
      ids.push(storylet.id);
    }
    return ids.join(',');
  }

  draw() {
    if (this._drawPile.length==0)
      return null;

    const storylet = this._drawPile.shift();
    if (storylet.updateOnDrawn)
      updateContext(this._context, storylet.updateOnDrawn);
    storylet.drawn(this._currentDraw);
    this._currentDraw++;
    return storylet;
  }
}
