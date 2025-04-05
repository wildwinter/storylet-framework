// This file is part of an MIT-licensed project: see LICENSE file or README.md for details.
// Copyright (c) 2025 Ian Thomas

import { ExpressionParser } from '../lib/expression-parser/expressionParser.js';
import { shuffleArray } from "./utils.js";
import { updateContext } from "./context.js";

const expressionParser = new ExpressionParser();

// Meta-values in Redraw.
export const REDRAW_ALWAYS = 0;
export const REDRAW_NEVER = -1;

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
    this.updateOnPlayed = null;

    // Precompiled
    this._condition = null;
    // Might be absolute value, might be an expression
    this._priority = 0;
    // The next draw this should be available.
    this._nextPlay = 0;

    // Attached to a deck.
    this.deck = null;
  }

  // Reset the redraw counter
  reset() {
    this._nextPlay = 0;
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
  canDraw(currentPlay) {
    if (this.redraw == REDRAW_NEVER && this._nextPlay<0)
      return false;
    if (this.redraw == REDRAW_ALWAYS)
      return true;
    return currentPlay>=this._nextPlay;
  }

  // Call when actually played - updates the play counter.
  onPlayed(currentPlay, context) {
    if (this.redraw == REDRAW_NEVER) {
      this._nextPlay = -1;
    } else {
      this._nextPlay = currentPlay + this.redraw;
    }
    if (this.updateOnPlayed)
      updateContext(context, this.updateOnPlayed);

  }

  // Convenience. Play the storylet. Updates draw counters and applies any updateOnPlayed
  play() {
    if (!this.deck) {
      throw new Error("Storylet not in deck.");
    }
    this.deck.play(this);
  }

}

// Deck of storylets.
export class Deck {

  constructor(context = {}) {

    // If true, storylet.priority is still used as the base, but
    // within priorities more complex conditions (thus more specific) are
    // treated as higher priority
    this.useSpecificity = false;

    // Complete set of storylets
    this._all = new Map();

    // Currently calculated pile
    this._drawPile = [];

    // Keeps a count of the number of draws. Used to keep redraw rules correct.
    // Call reset() to start from scratch.
    this._currentPlay = 0;

    // Context to be used for all expression evaluations.
    this.context = context;
  }

  // Reset the whole pack, including all redraw counters.
  reset() {
    this.currentDraw = 0;
    for (const storylet of this._all.values()) {
      storylet.reset();
    }
  }

  // Reshuffle the deck, filtering out any storylets whose conditions return false, 
  // and anything which fails the optionally supplied filter.
  // Return "count" storylets, or all of them if count is -1.
  // The draw pile will be sorted by priority (and specificity where relevant)
  // dump_eval will fill an array with evaluation debug steps
  draw(count = -1, filter=null, dump_eval = null) {

    // Temp map to hold lists by priority
    const priorityMap = new Map();

    // All the cards to walk
    const toProcess = [...this._all.values()];

    while (toProcess.length>0) {

      const storylet = toProcess.shift();

      if (!storylet.canDraw(this._currentPlay)) {
        // Storylet fails the draw rules - skip
        continue;
      }
      
      // Apply filter, if available
      if (filter!=null) {
        if (!filter(storylet)) {
          // Storylet fails the filter - skip
          continue;
        }
      }

      if (!storylet.checkCondition(this.context, dump_eval)) {
        // Storylet fails the condition - skip
        continue;
      }

      // Get the current priority for the storylet
      let priority = storylet.calcCurrentPriority(this.context, this.useSpecificity, dump_eval);
      
      if (!priorityMap.has(priority)) {
        // Does a priority list for this priority value exist in the temp map? If not, make it
        priorityMap.set(priority, []);
      }
      // Add our card to this priority list
      priorityMap.get(priority).push(storylet);
    }

    const sortedPriorities = [...priorityMap.keys()].sort((a, b) => b - a);
    const drawPile = [];

    // Shuffle each set of storylets that are the same priority. Then add them all to the master draw pile.
    // Result will be - higher priorities will be in the pile at the front, and we go down from there.
    for (const priority of sortedPriorities) {
      const bucket = priorityMap.get(priority);
      shuffleArray(bucket);
      drawPile.push(...bucket);
      if (count > -1 && drawPile.length > count) {
        break;
      }
    }

    return count > -1 ? drawPile.slice(0, count) : drawPile;
  }

  drawAndPlay(count = -1, filter=null, dump_eval = null) {
    const drawn = this.draw(count, filter, dump_eval);
    for (let i = 0; i < drawn.length; i++) {
      drawn[i].play();
    }
    return drawn;
  }

  drawSingle(filter = null, dump_eval = null) {
    let drawn = this.draw(1, filter, dump_eval);
    if (drawn.length == 0) {
      return null;
    }
    return drawn[0];
  }

  drawAndPlaySingle(filter = null, dump_eval = null) {
    const drawn = this.drawSingle(filter, dump_eval);
    if (drawn) {
      drawn.play();
    }
    return drawn;
  }

  getStorylet(id) {
    return this._all.get(id);
  } 

  addStorylet(storylet) {
    if (this._all.has(storylet.id)) {
      throw new Error(`Storylet with id ${storylet.id} already exists`);
    }
    this._all.set(storylet.id, storylet);
    storylet.deck = this;
  }

  // Mark this storylet as played. If it's got an updateOnPlayed, make that happen.
  play(storylet) {
    this._currentPlay++;
    storylet.onPlayed(this._currentPlay, this.context);
  }
}
