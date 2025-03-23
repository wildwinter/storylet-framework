// This file is part of an MIT-licensed project: see LICENSE file or README.md for details.
// Copyright (c) 2025 Ian Thomas

import { ExpressionParser } from '../lib/expression-parser/expressionParser.js';
import { shuffleArray, strToNumeric} from "./utils.js";

const expressionParser = new ExpressionParser();
const USE_SPECIFICITY = true;

export function evalExpression(str, context) {
  const expression = expressionParser.parse(str);
  return expression.evaluate(context);
}

export class Storylet {

  constructor(id) {
    this.id = id;
    this.condition = "";
    this.content = {};
    this._compiledCondition = null;
    this._priority = 0;
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
    let workingPriority = 0;
    if (typeof this._priority === "number") {
      workingPriority = this._priority;
    } else {
      workingPriority = + evalExpression(this._priority, context);
    }
    if (USE_SPECIFICITY && this._compiledCondition!=null) {
      workingPriority = workingPriority*100;
      workingPriority += this._compiledCondition.specificity;
    }
    return workingPriority;
  }

  static fromJson(json) {

    if (!("id" in json))
      throw new Error("No 'id' property in the storylet JSON.", json);

    const storylet = new Storylet(json.id);

    if ("condition" in json) {
      storylet.condition = json.condition;
      storylet.compileCondition();
    }
    if ("priority" in json) {
        storylet.priority = json.priority;
    }
    if ("content" in json)
      storylet.content = json.content;
    return storylet;

  }

}

export class Deck {

  constructor() {
    this._all = new Map();
    this._drawPile = [];
    this._lastDrawn = new Map();
    this._currentDraw = 0;
  }

  static fromJson(json) {
    const deck = new Deck();
    deck.loadJson(json);
    return deck
  }

  loadJson(json) {
    for (const item of json) {
      const storylet = Storylet.fromJson(item);
      if (this._all.has(storylet.id)) {
        throw new Error(`Duplicate storylet id: '${storylet.id}'.`, item);
      }
      this._all.set(storylet.id, storylet);
    }
  }

  refresh(context, filter=null) {

    this._drawPile = [];
    const priorityAvailable = new Map();
    const toProcess = [...this._all.values()];

    while (toProcess.length>0) {
      
      const storylet = toProcess.shift();
      
      // Apply filter, if available
      if (filter!=null) {
        if (!filter(storylet)) {
          continue;
        }
      }

      if (!storylet.checkCondition(context)) {
        //console.log(`Storylet '${storylet.id}': condition '${storylet.condition}' didn't pass.`);
        continue;
      }

      let priority = storylet.calcCurrentPriority(context);
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

  draw() {
    if (this._drawPile.length==0)
      return null;

    const storylet = this._drawPile.shift();
    this._lastDrawn.set(storylet.id, this._currentDraw++);
    return storylet;
  }
}
