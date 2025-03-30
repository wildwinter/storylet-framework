// This file is part of an MIT-licensed project: see LICENSE file or README.md for details.
// Copyright (c) 2025 Ian Thomas

import { ExpressionParser } from '../lib/expression-parser/expressionParser.js';
import { shuffleArray, copyObject, updateObject } from "./utils.js";
import { initContext, updateContext } from "./context.js";

const expressionParser = new ExpressionParser();

// Meta-values in Redraw.
const REDRAW_ALWAYS = 0;
const REDRAW_NEVER = -1;

export class Storylet {

  constructor(id) {
    // Unique ID of the storylet
    this.id = id;
    
    // Application-defined content.
    this.content = {};

    // Redraw setting:
    //   >0 = how many draws before this is available again, 
    //   REDRAW_ALWAYS = always available, 
    //   REDRAW_NEVER = a one-shot
    this.redraw = REDRAW_ALWAYS;

    // Updates to context, processed when this storylet is drawn.
    // Can use to e.g. set a flag when a storylet is drawn.
    // Only for lightweight behaviours and won't work well for
    // conditionals, might be best to implement
    // something app-specific.
    this.updateOnDrawn = null;

    // Precompiled
    this._condition = null;
    // Might be absolute value, might be an expression
    this._priority = 0;
    // The next draw this should be available.
    this._nextDraw = 0;
  }

  // Reset the redraw counter
  reset() {
    this._nextDraw = 0;
  }

  // Set text of expression. It'll be precompiled.
  set condition(text) {
    this._condition = null;
    if (text) {
      this._condition = expressionParser.parse(text);
    }
  }

  // Evaluate condition using current context. Return boolean. If not condition, returns true.
  checkCondition(context, dump_eval) {
    if (!this._condition)
      return true;
    if (dump_eval) {
      dump_eval.push(`Evaluating condition for ${this.id}`);
    }
    return this._condition.evaluate(context, dump_eval);
  }

  // Set priority to a fixed number, or to an expression which will be precompiled.
  set priority(numOrExpression) { 
    if (typeof numOrExpression==="number") {
      this._priority = numOrExpression;
    } else {
      this._priority = expressionParser.parse(numOrExpression);;
    }
  }  

  // Evaluate priority using current context.
  // If useSpecifity is true, will produce a higher number if the storylet's expression is more complex.
  calcCurrentPriority(context, useSpecificity=true, dump_eval = null) {

    // Internally, if we're using specificity, everything is 100* higher so we can stick with an int.
    let workingPriority = 0;
    if (typeof this._priority==="number") {
      workingPriority = this._priority;
    } else {
      if (dump_eval) {
        dump_eval.push(`Evaluating priority for ${this.id}`);
      }
      workingPriority = +this._priority.evaluate(context, dump_eval);
    }

    if (useSpecificity) {
      workingPriority = workingPriority*100;
      if (this._compiledCondition!=null) {
        workingPriority += this._compiledCondition.specificity;
      }
    }

    return workingPriority;
  }

  // True if the card is available to draw according to its redraw rules.
  canDraw(currentDraw) {
    if (this.redraw == REDRAW_NEVER && this._nextDraw<0)
      return false;
    if (this.redraw == REDRAW_ALWAYS)
      return true;
    return currentDraw>=this._nextDraw;
  }

  // Call when actually drawn - updates the redraw counter.
  drawn(currentDraw) {
    if (this.redraw == REDRAW_NEVER) {
      this._nextDraw = -1;
      return;
    }
    this._nextDraw = currentDraw + this.redraw;
  }

  // Basic parsing
  static fromJson(json, defaults) {

    if (!("id" in json))
      throw new Error("No 'id' property in the storylet JSON.", json);

    let config = defaults;
    // Make config into the "import this" version of the storylet - a combination of the current packet defaults
    // and the new material found in the storylet itself.
    updateObject(config, json);
    //console.log("Parsing:", json.id, config);


    const storylet = new Storylet(config.id);
    if ("redraw" in config) {
      let val = config.redraw;
      if (val=="always")
        storylet.redraw = REDRAW_ALWAYS;
      else if (val=="never")
        storylet.redraw = REDRAW_NEVER;
      else
        storylet.redraw = parseInt(val);
    }

    if ("condition" in config) {
      storylet.condition = config.condition;
    }
    if ("priority" in config) {
      storylet.priority = config.priority;
    }
    if ("updateOnDrawn" in config) {
      storylet.updateOnDrawn = config.updateOnDrawn;
    }
    if ("content" in config) {
      storylet.content = config.content;
    }
    return storylet;
  }

}

// Deck of storylets.
export class Deck {

  constructor(context = {}) {

    // If true, storylet.priority is still used as the base, but
    // within priorities more complex conditions (thus more specific) are
    // treated as higher priority
    this.useSpecificity = false;

    // How many storylets to process on each call to update(), if using async reshuffles?
    this.asyncReshuffleCount = 10;
    
    // Complete set of storylets
    this._all = new Map();

    // Currently calculated pile
    this._drawPile = [];

    // Keeps a count of the number of draws. Used to keep redraw rules correct.
    // Call reset() to start from scratch.
    this._currentDraw = 0;

    // Context to be used for all expression evaluations.
    this._context = context;

    // Used to deal with in-progress reshuffles.
    this._reshuffleState = {callback:null, toProcess:[], filter:null, priorityMap:null, dump_eval:null};
  }

  // Parse from json
  // reshuffle automatically reshuffles before returning.
  // dump_eval will fill an array with evaluation debug steps
  static fromJson(json, context = {}, reshuffle = true, dump_eval = null) {
    const deck = new Deck(context);
    deck.loadJson(json, dump_eval);
    if (reshuffle)
      deck.reshuffle(null, dump_eval);
    return deck
  }

  // dump_eval will fill an array with evaluation debug steps
  loadJson(json, dump_eval = null) {
    this._readPacketFromJson(json, {}, dump_eval);
  }

  // Read a packet of storylets, inheriting the given defaults
  _readPacketFromJson(json, defaults, dump_eval = null) {

    if ("context" in json) {
      initContext(this._context, json.context, dump_eval);
    }

    if ("defaults" in json) {
      for (const [varName, value] of Object.entries(json.defaults)) {
        defaults[varName] = value;
      }
    }

    if ("storylets" in json) {
      this._readStoryletsFromJson(json.storylets, copyObject(defaults), dump_eval);
    }
  }

  // Read an array of storylets, inheriting the given defaults. If any storylets is actually a packet,
  //   read that packet.
  // dump_eval will fill an array with evaluation debug steps
  _readStoryletsFromJson(json, defaults, dump_eval=null) {

    for (const item of json) {
    
      // Is this a storylet? Or is it a packet?
      if ("storylets" in item||"defaults" in item||"context" in item) {
        this._readPacketFromJson(item, defaults, dump_eval);
        continue;
      }

      // Read as storylet.
      if (!("id" in item)) {
        throw new Error(`Json item is not storylet or packet`, item);
      }

      const storylet = Storylet.fromJson(item, defaults);
      if (this._all.has(storylet.id)) {
        throw new Error(`Duplicate storylet id: '${storylet.id}'.`, item);
      }

      this._all.set(storylet.id, storylet);
      if (dump_eval) {
        dump_eval.push(`Added storylet '${storylet.id}'`);
      }
    }

  }

  // Reset the whole pack, including all redraw counters.
  reset() {
    this.currentDraw = 0;
    for (const storylet of this._all) {
      storylet.reset();
    }
  }

  // Reshuffle the deck, filtering out any storylets whose conditions return false, 
  // and anything which fails the optionally supplied filter.
  // The draw pile will be sorted by priority (and specificity where relevant)
  // Might be slow if you have a lot of storylets - consider reshuffleAsync instead.
  // dump_eval will fill an array with evaluation debug steps
  reshuffle(filter=null, dump_eval = null) {

    if (this.asyncReshuffleInProgress()) {
      throw new Error("Async reshuffle in progress, can't call reshuffle()");
    }

    this._reshufflePrep(filter, dump_eval);
    // Reshuffle everything at once.
    this._reshuffleDoChunk(this._reshuffleState.toProcess.length);
    this._reshuffleFinalise();
  }

  // Use this to kick off an async reshuffle. Now you need to call update() periodically (e.g. every frame)
  // When the reshuffle is complete, the callback will be called.
  // dump_eval will fill an array with evaluation debug steps
  reshuffleAsync(callback, filter=null, dump_eval = null) {

    if (this.asyncReshuffleInProgress()) {
      throw new Error("Async reshuffle in progress, can't call reshuffleAsync()");
    }

    this._reshuffleState.callback = callback;
    this._reshufflePrep(filter, dump_eval);

  }

  asyncReshuffleInProgress() {
    return this._reshuffleState.callback!=null;
  }

  // If you are using reshuffleAsync, call this every so often (e.g. every frame) to get the reshuffle to happen.
  update() {

    // If an async reshuffle is in progress
    if (this.asyncReshuffleInProgress()) {
      this._reshuffleDoChunk(this.asyncReshuffleCount);
      if (this._reshuffleState.toProcess.length==0) {
        this._reshuffleFinalise();
      }
    }

  }

  _reshufflePrep(filter, dump_eval=null) {
    // Empty the draw pile
    this._drawPile = [];
    this._reshuffleState.dump_eval = dump_eval;
    this._reshuffleState.filter = filter;
    // Temp map to hold lists by priority
    this._reshuffleState.priorityMap = new Map();
    // All the cards to walk
    this._reshuffleState.toProcess = [...this._all.values()];
  }


  _reshuffleDoChunk(count) {

    let numberToDo = Math.min(count, this._reshuffleState.toProcess.length);

    while (numberToDo>0) {

      numberToDo--;
      
      const storylet = this._reshuffleState.toProcess.shift();

      if (!storylet.canDraw(this._currentDraw)) {
        // Storylet fails the draw rules - skip
        continue;
      }
      
      // Apply filter, if available
      if (this._reshuffleState.filter!=null) {
        if (!this._reshuffleState.filter(storylet)) {
          // Storylet fails the filter - skip
          continue;
        }
      }

      if (!storylet.checkCondition(this._context, this._reshuffleState.dump_eval)) {
        // Storylet fails the condition - skip
        //console.log(`Storylet '${storylet.id}': condition '${storylet.condition}' didn't pass.`);
        continue;
      }

      // Get the current priority for the storylet
      let priority = storylet.calcCurrentPriority(this._context, this.useSpecificity, this._reshuffleState.dump_eval);
      
      if (!this._reshuffleState.priorityMap.has(priority)) {
        // Does a priority list for this priority value exist in the temp map? If not, make it
        this._reshuffleState.priorityMap.set(priority, []);
      }
      // Add our card to this priority list
      this._reshuffleState.priorityMap.get(priority).push(storylet);
    }

  }

  _reshuffleFinalise() {
    // Now sort all the resultant priorities
    const sortedPriorities = [...this._reshuffleState.priorityMap.keys()].sort((a, b) => b - a);

    // Shuffle each set of storylets that are the same priority. Then add them all to the master draw pile.
    // Result will be - higher priorities will be in the pile at the front, and we go down from there.
    for (const priority of sortedPriorities) {
      const bucket = this._reshuffleState.priorityMap.get(priority);
      shuffleArray(bucket);
      this._drawPile.push(...bucket);
    }

    this._reshuffleState.priorityMap = null;
    this._reshuffleState.toProcess = [];
    this._reshuffleState.filter = null;
    const callback = this._reshuffleState.callback;
    this._reshuffleState.callback = null;
    this._reshuffleState.dump_eval = null;
    if (callback) {
      callback();
    }
  }

  // For debugging - simply dump the IDs of the current draw pile storylets.
  dumpDrawPile() {
    if (this.asyncReshuffleInProgress()) {
      throw new Error("Async reshuffle in progress, can't call dumpDrawPile()");
    }

    let ids = [];
    for (const storylet of this._drawPile) {
      ids.push(storylet.id);
    }
    return ids.join(',');
  }

  // Draw the next storylet from the draw pile. If it's got an updateOnDrawn, make that happen.
  draw() {
    
    if (this.asyncReshuffleInProgress()) {
      throw new Error("Async reshuffle in progress, can't call draw()");
    }

    this._currentDraw++;

    if (this._drawPile.length==0)
      return null;

    const storylet = this._drawPile.shift();
    if (storylet.updateOnDrawn)
      updateContext(this._context, storylet.updateOnDrawn);
    storylet.drawn(this._currentDraw);
    return storylet;
  }
}
