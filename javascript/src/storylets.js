// This file is part of an MIT-licensed project: see LICENSE file or README.md for details.
// Copyright (c) 2025 Ian Thomas

import { ExpressionParser } from '../lib/expression-parser/expressionParser.js';

export const expressionParser = new ExpressionParser();

export class Storylet {

  constructor(id) {
    this.id = id;
    this.condition = "";
    this._compiledCondition = null;
    this._specificity = 0;
    this.priority = 0;
    this.content = {};
  }

  compileCondition() {
    this._compiledCondition = null;
    this._specificity = 0;
    if (this.condition!="") {
      this._compiledCondition = expressionParser.parse(this.condition);
      this._specificity = this._compiledCondition.specificity;
    }
  }

  checkCondition(context) {
    if (!this._compiledCondition)
      return true;
    return this._compiledCondition.evaluate(context);
  }

  get specificity() {return this._specificity;}

  static fromJson(json) {

    if (!("id" in json))
      throw new Error("No 'id' property in the storylet JSON.", json);

    const storylet = new Storylet(json.id);

    if ("condition" in json) {
      storylet.condition = json.condition;
      storylet.compileCondition();
    }
    if ("priority" in json)
      storylet.priority = json.priority;
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

      let workingPriority = storylet.priority*100;
      workingPriority+=storylet.specificity;

      if (!priorityAvailable.has(workingPriority))
        priorityAvailable.set(workingPriority, []);
      priorityAvailable.get(workingPriority).push(storylet);
    }

    const sortedPriorities = [...priorityAvailable.keys()].sort((a, b) => b - a);
    for (const priority of sortedPriorities) {
      const bucket = priorityAvailable.get(priority);
      const shuffledBucket = shuffle(bucket);
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


function shuffle(array) {
  for (let i = array.length - 1; i > 0; i--) {
    const j = Math.floor(Math.random() * (i + 1));
    [array[i], array[j]] = [array[j], array[i]];
  }
  return array;
}