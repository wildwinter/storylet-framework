// This file is part of an MIT-licensed project: see LICENSE file or README.md for details.
// Copyright (c) 2025 Ian Thomas
import { copyObject, updateObject } from "./utils.js";
import { initContext } from "./context.js";
import { Storylet, Deck, REDRAW_ALWAYS, REDRAW_NEVER } from "./storylets.js"

export function storyletFromJson(json, defaults) {
    if (!("id" in json))
      throw new Error("No 'id' property in the storylet JSON.", json);

    let config = copyObject(defaults);
    // Make config into the "import this" version of the storylet - a combination of the current packet defaults
    // and the new material found in the storylet itself.
    updateObject(config, json);

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
    if ("outcomes" in config) {
      storylet.outcomes = config.outcomes;
    }
    if ("content" in config) {
      storylet.content = config.content;
    }
    return storylet;
}


// Parse from json
// dump_eval will fill an array with evaluation debug steps
export function deckFromJson(json, context = {}, dump_eval = null) {
    const deck = new Deck(context);
    _readPacketFromJson(deck, json, {}, dump_eval);
    return deck
}

// Read a packet of storylets, inheriting the given defaults
function _readPacketFromJson(deck, json, defaults, dump_eval = null) {
    if ("context" in json) {
        initContext(deck.context, json.context, dump_eval);
    }

    if ("defaults" in json) {
        for (const [varName, value] of Object.entries(json.defaults)) {
            defaults[varName] = value;
        }
    }

    if ("storylets" in json) {
        _readStoryletsFromJson(deck, json.storylets, copyObject(defaults), dump_eval);
    }
}

// Read an array of storylets, inheriting the given defaults. If any storylets is actually a packet,
//   read that packet.
// dump_eval will fill an array with evaluation debug steps
function _readStoryletsFromJson(deck, json, defaults, dump_eval=null) {

    for (const item of json) {

        // Is this a storylet? Or is it a packet?
        if ("storylets" in item||"defaults" in item||"context" in item) {
            _readPacketFromJson(deck, item, defaults, dump_eval);
            continue;
        }

        // Read as storylet.
        if (!("id" in item)) {
            throw new Error(`Json item is not storylet or packet`, item);
        }

        const storylet = storyletFromJson(item, defaults);
        deck.addStorylet(storylet);
        if (dump_eval) {
            dump_eval.push(`Added storylet '${storylet.id}'`);
        }
    }

}