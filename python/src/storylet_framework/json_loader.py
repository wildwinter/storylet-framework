# This file is part of an MIT-licensed project: see LICENSE file or README.md for details.
# Copyright (c) 2025 Ian Thomas

from .storylets import Storylet, Deck, REDRAW_ALWAYS, REDRAW_NEVER
from .utils import update_object, copy_object
from .context import init_context

def storylet_from_json(json, defaults):
    if "id" not in json:
        raise ValueError("No 'id' property in the storylet JSON.")

    config = defaults.copy()
    update_object(config, json)

    storylet = Storylet(config["id"])
    if "redraw" in config:
        val = config["redraw"]
        if val == "always":
            storylet.redraw = REDRAW_ALWAYS
        elif val == "never":
            storylet.redraw = REDRAW_NEVER
        else:
            storylet.redraw = int(val)

    if "condition" in config:
        storylet.condition = config["condition"]
    if "priority" in config:
        storylet.priority = config["priority"]
    if "updateOnPlayed" in config:
        storylet.update_on_played = config["updateOnPlayed"]
    if "content" in config:
        storylet.content = config["content"]

    return storylet

def deck_from_json(json, context={}, dump_eval=None):
    deck = Deck(context)
    _read_packet_from_json(deck, json, {}, dump_eval)
    return deck

def _read_packet_from_json(deck, json, defaults, dump_eval=None):
    if "context" in json:
        init_context(deck.context, json["context"], dump_eval)

    if "defaults" in json:
        for var_name, value in json["defaults"].items():
            defaults[var_name] = value

    if "storylets" in json:
        _read_storylets_from_json(deck, json["storylets"], copy_object(defaults), dump_eval)

def _read_storylets_from_json(deck, json, defaults, dump_eval=None):
    for item in json:
        # Is this a storylet? Or is it a packet?
        if "storylets" in item or "defaults" in item or "context" in item:
            _read_packet_from_json(deck, item, defaults, dump_eval)
            continue

        # Read as storylet
        if "id" not in item:
            raise ValueError("Json item is not a storylet or packet")

        storylet = storylet_from_json(item, defaults)
        deck.add_storylet(storylet)

        if dump_eval is not None:
            dump_eval.append(f"Added storylet '{storylet.id}'")